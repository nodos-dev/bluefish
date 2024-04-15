// Copyright MediaZ Teknoloji A.S. All Rights Reserved.

#include <nosUtil/Stopwatch.hpp>

#include <nosVulkanSubsystem/nosVulkanSubsystem.h>

#include "DMANodeBase.hpp"
#include "BluefishTypes_generated.h"
#include "Device.hpp"
#include "nosVulkanSubsystem/Helpers.hpp"

namespace bf
{
struct DMAReadNodeContext : DMANodeBase
{
	using DMANodeBase::DMANodeBase;

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
		
		nosVec2u ycbcrSize(width >> 1, height);
		uint32_t bufferSize = ycbcrSize.x * ycbcrSize.y * 4;

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
					}} };
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

		auto buffer = nosVulkan->Map(&outputBuffer);
		if((uintptr_t)buffer % 64 != 0)
			nosEngine.LogE("DMA write only accepts buffers addresses to be aligned to 64 bytes"); // TODO: Check device. This is only in Khronos range!

		auto startCaptureBufferId = (BufferId + 2) % CycledBuffersPerChannel; // +2: Buffer will be available after two VBIs.
		{
			nos::util::Stopwatch sw;
			device->DMAReadFrame(channel, startCaptureBufferId, BufferId, buffer, outputBuffer.Info.Buffer.Size);
			auto elapsed = sw.Elapsed();
			nosEngine.WatchLog(("Bluefish " + channelStr + " DMA Read").c_str(), nos::util::Stopwatch::ElapsedString(elapsed).c_str());
		}
		BufferId = (BufferId + 1) % CycledBuffersPerChannel;

		return NOS_RESULT_SUCCESS;
	}
};

nosResult RegisterDMAReadNode(nosNodeFunctions* outFunctions)
{
	NOS_BIND_NODE_CLASS(NOS_NAME("DMARead"), DMAReadNodeContext, outFunctions)
	return NOS_RESULT_SUCCESS;
}
}