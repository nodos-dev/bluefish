#include <Nodos/PluginAPI.h>
#include <Nodos/PluginHelpers.hpp>
#include <nosUtil/Stopwatch.hpp>

#include <nosVulkanSubsystem/nosVulkanSubsystem.h>

#include "BluefishTypes_generated.h"
#include "Device.hpp"
#include "nosVulkanSubsystem/Helpers.hpp"

namespace bf
{
// VANC related utils
BLUE_U32 InitClosedCaptionPacketData(unsigned short* pPacketData)
{
	pPacketData[0] = 0;
	pPacketData[1] = 0x3FF;	//Packet marker
	pPacketData[2] = 0x3FF;	//Packet marker
	pPacketData[3] = 0x161;	//DID
	pPacketData[4] = 0x102;	//SDID
	pPacketData[5] = 0x10B;	//data length: 'Bluefish444' = 11 = 0xB
	pPacketData[6] = 0x42;		//'B'	=>	66	=> 0x42	=> parity: 0x42
	pPacketData[7] = 0x6c;		//'l'	=> 108	=> 0x6c	=> parity: 0x6c
	pPacketData[8] = 0x175;		//'u'	=> 117	=> 0x75	=> parity: 0x175
	pPacketData[9] = 0x65;		//'e'	=> 101	=> 0x65	=> parity: 0x65
	pPacketData[10] = 0x66;		//'f'	=> 102	=> 0x66	=> parity: 0x66
	pPacketData[11] = 0x69;		//'i'	=> 105	=> 0x69	=> parity: 0x69
	pPacketData[12] = 0x173;	//'s'	=> 115	=> 0x73	=> parity: 0x173
	pPacketData[13] = 0x168;	//'h'	=> 104	=> 0x68	=> parity: 0x168
	pPacketData[14] = 0x134;	//'4'	=> 52	=> 0x34	=> parity: 0x134
	pPacketData[15] = 0x134;	//'4'	=> 52	=> 0x34	=> parity: 0x134
	pPacketData[16] = 0x134;	//'4'	=> 52	=> 0x34	=> parity: 0x134

	int checksum = 0x13C;
	printf("Vanc Packet checksum: %d\n", checksum);
	pPacketData[17] = (checksum & 0x3FF);

	return 18;
}

void InsertClosedCaptions(BLUE_S32 CardType, BLUE_S32 LineNumber, BLUE_U16* pPacketData, BLUE_U32 DataElements, BLUE_U8* pVancBuffer, BLUE_U32 VancPixelsPerLine)
{
	bfcUtilsVancPktInsert(CardType, BlueVancPktYComp, LineNumber, (BLUE_U32*)pPacketData, DataElements, (BLUE_U32*)pVancBuffer, VancPixelsPerLine);
}

struct DMAWriteNodeContext : nos::NodeContext
{
	DMAWriteNodeContext(const nosFbNode* node) : NodeContext(node)
	{
	}

	nosResult ExecuteNode(const nosNodeExecuteArgs* args) override
	{
		nosScheduleNodeParams schedule{
			.NodeId = NodeId,
			.AddScheduleCount = 1
		};
		nosEngine.ScheduleNode(&schedule);

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
		auto channel = static_cast<EBlueVideoChannel>(channelInfo->channel()->id());
		auto videoMode = static_cast<EVideoModeExt>(channelInfo->video_mode());
		std::string channelStr = bfcUtilsGetStringForVideoChannel(channel);

		BLUE_U32 width, height, bytesPerLine, bytesPerFrame, goldenBytes;
		bfcGetVideoInfo(videoMode, UPD_FMT_FRAME, MEM_FMT_YUVS, &width, &height, &bytesPerLine, &bytesPerFrame, &goldenBytes);

		nosCmd cmd;
		nosVulkan->Begin("Flush Before Bluefish DMA Write", &cmd);
		nosGPUEvent event;
		nosCmdEndParams end {.ForceSubmit = NOS_TRUE, .OutGPUEventHandle = &event};
		nosVulkan->End(cmd, &end);
		auto res = nosVulkan->WaitGpuEvent(&event, 10e9);
		if (res != NOS_RESULT_SUCCESS)
			nosEngine.LogE("Error when flush before Bluefish DMA write");
		auto buffer = nosVulkan->Map(&inputBuffer);

		{
			nos::util::Stopwatch sw;
			device->DMAWriteFrame(BufferId, buffer, inputBuffer.Info.Buffer.Size, channel);
			auto elapsed = sw.Elapsed();
			nosEngine.WatchLog(("Bluefish " + channelStr + " DMA Write").c_str(), nos::util::Stopwatch::ElapsedString(elapsed).c_str());
		}

		BufferId = (BufferId + 1) % 4;
		
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
	void* VideoBuffer = nullptr;
};

nosResult RegisterDMAWriteNode(nosNodeFunctions* outFunctions)
{
	NOS_BIND_NODE_CLASS(NOS_NAME_STATIC("bluefish444.DMAWrite"), DMAWriteNodeContext, outFunctions)
	return NOS_RESULT_SUCCESS;
}

}