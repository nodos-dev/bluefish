#include <Nodos/PluginAPI.h>
#include <Nodos/PluginHelpers.hpp>
#include <nosUtil/Stopwatch.hpp>

#include <nosVulkanSubsystem/nosVulkanSubsystem.h>

#include "BluefishTypes_generated.h"
#include "Device.hpp"
#include "nosVulkanSubsystem/Helpers.hpp"

namespace bf
{
struct DMAReadNodeContext : nos::NodeContext
{
	DMAReadNodeContext(const nosFbNode* node) : NodeContext(node)
	{
	}

	nosResult ExecuteNode(const nosNodeExecuteArgs* args) override
	{
		nos::bluefish::ChannelInfo* channelInfo = nullptr;
		nosResourceShareInfo outputBuffer{};
		for (size_t i = 0; i < args->PinCount; ++i)
		{
			auto& pin = args->Pins[i];
			if (pin.Name == NOS_NAME_STATIC("Channel"))
				channelInfo = nos::InterpretPinValue<nos::bluefish::ChannelInfo>(*pin.Data);
			if (pin.Name == NOS_NAME_STATIC("Output"))
				outputBuffer = nos::vkss::ConvertToResourceInfo(*nos::InterpretPinValue<nos::sys::vulkan::Buffer>(*pin.Data));
		}
 
		if (!channelInfo->device() || !channelInfo->channel())
			return NOS_RESULT_FAILED;

		auto device = BluefishDevice::GetDevice(channelInfo->device()->serial()->str());
		if (!device)
			return NOS_RESULT_FAILED;
		auto channel = static_cast<EBlueVideoChannel>(channelInfo->channel()->id());
		auto videoMode = static_cast<EVideoModeExt>(channelInfo->video_mode());

		uint32_t width, height;
		bfcGetVideoWidth(videoMode, &width);
		bfcGetVideoHeight(videoMode, UPD_FMT_FRAME, &height);
		
		nosVec2u compressedExt(width >> 1, height);
		uint32_t bufferSize = compressedExt.x * compressedExt.y * 4;

		constexpr nosMemoryFlags memoryFlags = nosMemoryFlags(NOS_MEMORY_FLAGS_HOST_VISIBLE);
		if (outputBuffer.Memory.Size != bufferSize || outputBuffer.Info.Buffer.MemoryFlags != memoryFlags)
		{
			nosResourceShareInfo bufInfo = {
				.Info = {
					.Type = NOS_RESOURCE_TYPE_BUFFER,
					.Buffer = nosBufferInfo{
						.Size = (uint32_t)bufferSize,
						.Alignment = 64,
						.Usage = nosBufferUsage(NOS_BUFFER_USAGE_STORAGE_BUFFER | NOS_BUFFER_USAGE_TRANSFER_SRC),
						.MemoryFlags = memoryFlags
					}}};
			auto bufferDesc = nos::vkss::ConvertBufferInfo(bufInfo);
			nosEngine.SetPinValueByName(NodeId, NOS_NAME_STATIC("Output"), nos::Buffer::From(bufferDesc));
			for (size_t i = 0; i < args->PinCount; ++i)
			{
				auto& pin = args->Pins[i];
				if (pin.Name == NOS_NAME_STATIC("Channel"))
					channelInfo = nos::InterpretPinValue<nos::bluefish::ChannelInfo>(*pin.Data);
				if (pin.Name == NOS_NAME_STATIC("Output"))
					outputBuffer = nos::vkss::ConvertToResourceInfo(*nos::InterpretPinValue<nos::sys::vulkan::Buffer>(*pin.Data));
			}
		}

		if (!outputBuffer.Memory.Handle)
			return NOS_RESULT_SUCCESS;

		std::string channelStr = bfcUtilsGetStringForVideoChannel(channel);
		auto d = device->GetDeltaSeconds(channel);
		nosVec2u newDeltaSeconds = { d[0], d[1] };
		if (memcmp(&newDeltaSeconds, &DeltaSeconds, sizeof(nosVec2u)) != 0)
		{
			// TODO: Recompilation needed. Nodos should call our GetScheduleInfo function.
			DeltaSeconds = newDeltaSeconds;
		}

		auto buffer = nosVulkan->Map(&outputBuffer);
		if((uintptr_t)buffer % 64 != 0)
			nosEngine.LogE("DMA write only accepts buffers addresses to be aligned to 64 bytes"); // TODO: Check device. This is only in Khronos range!

		auto nextBufferId = (BufferId + 1) % 2;
		{
			nos::util::Stopwatch sw;
			std::vector<uint8_t> data(outputBuffer.Info.Buffer.Size, 255);
			device->DMAReadFrame(channel, nextBufferId, BufferId, data.data(), outputBuffer.Info.Buffer.Size);
			auto elapsed = sw.Elapsed();
			nosEngine.WatchLog(("Bluefish " + channelStr + " DMA Read").c_str(), nos::util::Stopwatch::ElapsedString(elapsed).c_str());
		}
		BufferId = nextBufferId;

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
	BLUE_U32 BufferId = 0;
};

nosResult RegisterDMAReadNode(nosNodeFunctions* outFunctions)
{
	NOS_BIND_NODE_CLASS(NOS_NAME("DMARead"), DMAReadNodeContext, outFunctions)
	return NOS_RESULT_SUCCESS;
}
}