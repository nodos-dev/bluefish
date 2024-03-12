#include <Nodos/PluginAPI.h>
#include <Nodos/PluginHelpers.hpp>

#include <nosVulkanSubsystem/nosVulkanSubsystem.h>

#include "BluefishTypes_generated.h"
#include "Device.hpp"
#include "nosVulkanSubsystem/Helpers.hpp"

namespace bf
{

struct DMAWriteNodeContext : nos::NodeContext
{
	DMAWriteNodeContext(const nosFbNode* node) : NodeContext(node)
	{
	}
	
	nosResult ExecuteNode(const nosNodeExecuteArgs* args) override
	{
		bluefish444::ChannelInfo* channelInfo = nullptr;
		nosResourceShareInfo inputBuffer{};
		for (size_t i = 0; i < args->PinCount; ++i)
		{
			auto& pin = args->Pins[i];
			if (pin.Name == NOS_NAME_STATIC("Channel"))
				channelInfo = nos::InterpretPinValue<bluefish444::ChannelInfo>(*pin.Data);
			if (pin.Name == NOS_NAME_STATIC("Input"))
				inputBuffer = nos::vkss::ConvertToResourceInfo(*nos::InterpretPinValue<nos::sys::vulkan::Buffer>(*pin.Data));
		}
		
		if (!inputBuffer.Memory.Handle)
			return NOS_RESULT_FAILED;

		if (!channelInfo->device() || !channelInfo->channel())
			return NOS_RESULT_FAILED;

		auto device = BluefishDevice::GetDevice(channelInfo->device()->serial()->str());
		if (!device)
			return NOS_RESULT_FAILED;
		auto channel = ParseChannelNumber(channelInfo->channel()->name()->str());
		auto format = static_cast<EVideoModeExt>(channelInfo->video_mode());
		
		nosCmd cmd;
		nosVulkan->Begin("Flush Before Bluefish DMA Write", &cmd);
		nosGPUEvent event;
		nosCmdEndParams end {.ForceSubmit = NOS_TRUE, .OutGPUEventHandle = &event};
		nosVulkan->End(cmd, &end);
		nosVulkan->WaitGpuEvent(&event, UINT64_MAX);
		
		auto buffer = nosVulkan->Map(&inputBuffer);
		
		device->DMAWriteFrame(BufferId, buffer, inputBuffer.Info.Buffer.Size);

		BufferId = (BufferId + 1) % 4;
		
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
			.DeltaSeconds = { 1001, 60000 },
			.Type = NOS_SCHEDULE_TYPE_ON_DEMAND,
		};
	}
	
	void OnPathStart() override
	{
		nosScheduleNodeParams schedule{.NodeId = NodeId, .AddScheduleCount = 1};
		nosEngine.ScheduleNode(&schedule);
	}

	BLUE_U32 BufferId = 0;
};

nosResult RegisterDMAWriteNode(nosNodeFunctions* outFunctions)
{
	NOS_BIND_NODE_CLASS(NOS_NAME_STATIC("bluefish444.DMAWrite"), DMAWriteNodeContext, outFunctions)
	return NOS_RESULT_SUCCESS;
}

}