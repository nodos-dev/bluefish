// Copyright MediaZ Teknoloji A.S. All Rights Reserved.

#include <nosUtil/Stopwatch.hpp>

#include <nosVulkanSubsystem/nosVulkanSubsystem.h>

#include "DMANodeBase.hpp"
#include "BluefishTypes_generated.h"
#include "Device.hpp"
#include "nosVulkanSubsystem/Helpers.hpp"

namespace bf
{
struct DMAWriteNodeContext : DMANodeBase
{
	using DMANodeBase::DMANodeBase;

	nos::Buffer ChannelInfo{};
	BluefishDevice* Device = nullptr;
	EBlueVideoChannel Channel = BLUE_VIDEOCHANNEL_INVALID;

	void OnPinValueChanged(nos::Name pinName, nosUUID pinId, nosBuffer value) override
	{
		if (pinName == NOS_NAME("Channel"))
		{
			if (ChannelInfo.Size() == value.Size && memcmp(ChannelInfo.Data(), value.Data, value.Size) == 0)
				return;
			auto* channelInfo = nos::InterpretPinValue<nos::bluefish::ChannelInfo>(value);
			Device = nullptr;
			ChannelInfo = {};
			if (!channelInfo || !channelInfo->device())
				return;
			Device = BluefishDevice::GetDevice(channelInfo->device()->serial()->str()).get();
			if (!Device || !channelInfo->channel()->name())
				return;
			ChannelInfo = value;
			Channel = static_cast<EBlueVideoChannel>(channelInfo->channel()->id());
			auto dSec = Device->GetDeltaSeconds(Channel);
			DeltaSeconds = {dSec[0], dSec[1]};
			nosEngine.RecompilePath(NodeId);
		}
	}

	nosResult ExecuteNode(const nosNodeExecuteArgs* args) override
	{
		nosResourceShareInfo inputBuffer{};
		for (size_t i = 0; i < args->PinCount; ++i)
		{
			auto& pin = args->Pins[i];
			if (pin.Name == NOS_NAME("Input"))
				inputBuffer = nos::vkss::ConvertToResourceInfo(*nos::InterpretPinValue<nos::sys::vulkan::Buffer>(*pin.Data));
		}
		
		if (!inputBuffer.Memory.Handle)
			return NOS_RESULT_FAILED;
 
		if (!Device || Channel == BLUE_VIDEOCHANNEL_INVALID)
			return NOS_RESULT_FAILED;

		std::string channelStr = bfcUtilsGetStringForVideoChannel(Channel);

		nosCmd cmd;
		nosVulkan->Begin("Flush Before Bluefish DMA Write", &cmd);
		nosGPUEvent event;
		nosCmdEndParams end {.ForceSubmit = NOS_TRUE, .OutGPUEventHandle = &event};
		nosVulkan->End(cmd, &end);
		auto res = nosVulkan->WaitGpuEvent(&event, 10e9);
		if (res != NOS_RESULT_SUCCESS)
			nosEngine.LogE("Error when flush before Bluefish DMA write");

		auto buffer = nosVulkan->Map(&inputBuffer);
		if((uintptr_t)buffer % 64 != 0)
			nosEngine.LogE("DMA write only accepts buffers addresses to be aligned to 64 bytes"); // TODO: Check device. This is only in Khronos range!

		{
			nos::util::Stopwatch sw;
			Device->DMAWriteFrame(Channel, GetBufferId(Channel, BufferIdx), buffer, inputBuffer.Info.Buffer.Size);
			auto elapsed = sw.Elapsed();
			nosEngine.WatchLog(("Bluefish " + channelStr + " DMA Write").c_str(), nos::util::Stopwatch::ElapsedString(elapsed).c_str());
		}

		BufferIdx = (BufferIdx + 1) % CycledBuffersPerChannel;

		nosScheduleNodeParams schedule {
			.NodeId = NodeId,
			.AddScheduleCount = 1
		};
		nosEngine.ScheduleNode(&schedule);

		return NOS_RESULT_SUCCESS;
	}

	void GetScheduleInfo(nosScheduleInfo* out) override
	{
		*out = nosScheduleInfo {
			.Importance = 1,
			.DeltaSeconds = DeltaSeconds,
			.Type = NOS_SCHEDULE_TYPE_ON_DEMAND,
		};
	}
	
	void OnPathStart() override
	{
		nosScheduleNodeParams schedule{.NodeId = NodeId, .AddScheduleCount = 1};
		nosEngine.ScheduleNode(&schedule);
	}

	nosVec2u DeltaSeconds{};
};

nosResult RegisterDMAWriteNode(nosNodeFunctions* outFunctions)
{
	NOS_BIND_NODE_CLASS(NOS_NAME("DMAWrite"), DMAWriteNodeContext, outFunctions)
	return NOS_RESULT_SUCCESS;
}
}