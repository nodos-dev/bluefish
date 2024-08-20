// Copyright MediaZ Teknoloji A.S. All Rights Reserved.

#include <Nodos/Modules.h>
#include <nosUtil/Stopwatch.hpp>
#include <nosVulkanSubsystem/nosVulkanSubsystem.h>
#include <nosVulkanSubsystem/Helpers.hpp>

#include "DMANodeBase.hpp"
#include "BluefishTypes_generated.h"
#include "Device.hpp"

namespace bf
{
struct DMAReadNodeContext : DMANodeBase
{
	DMAReadNodeContext(const nosFbNode* node) : DMANodeBase(node) {
		AddPinValueWatcher(NOS_NAME("BufferToWrite"), [this](nos::Buffer const& newVal, std::optional<nos::Buffer> oldVal) {
			nosEngine.SetPinValue(PinName2Id[NOS_NAME("Output")], newVal);
		});
	}

	nosResult ExecuteNode(nosNodeExecuteParams* params) override
	{
		nos::bluefish::ChannelInfo* channelInfo = nullptr;
		nosResourceShareInfo outputBuffer{};
		nosUUID outputBufferId{};
		for (size_t i = 0; i < params->PinCount; ++i)
		{
			auto& pin = params->Pins[i];
			if (pin.Name == NOS_NAME("Channel"))
				channelInfo = nos::InterpretPinValue<nos::bluefish::ChannelInfo>(*pin.Data);
			if (pin.Name == NOS_NAME("Output"))
			{
				outputBuffer = nos::vkss::ConvertToResourceInfo(*nos::InterpretPinValue<nos::sys::vulkan::Buffer>(*pin.Data));
				outputBufferId = pin.Id;
			}
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

		if (!outputBuffer.Memory.Handle || outputBuffer.Info.Buffer.Size != bufferSize)
			return NOS_RESULT_FAILED;
		 
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

		outputBuffer.Info.Buffer.FieldType = NOS_TEXTURE_FIELD_TYPE_PROGRESSIVE; // TODO: Interlaced support

		nosEngine.SetPinValue(outputBufferId, nos::Buffer::From(nos::vkss::ConvertBufferInfo(outputBuffer)));

		return NOS_RESULT_SUCCESS;
	}
};

nosResult RegisterDMAReadNode(nosNodeFunctions* outFunctions)
{
	NOS_BIND_NODE_CLASS(NOS_NAME("DMARead"), DMAReadNodeContext, outFunctions)
	return NOS_RESULT_SUCCESS;
}
}