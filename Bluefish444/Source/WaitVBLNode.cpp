#include <Nodos/PluginAPI.h>
#include <Nodos/PluginHelpers.hpp>

#include "Device.hpp"
#include "BluefishTypes_generated.h"

namespace bluefish444
{
struct ChannelInfo;
}

namespace bf
{

struct WaitVBLNodeContext : nos::NodeContext
{
	WaitVBLNodeContext(const nosFbNode* node) : NodeContext(node)
	{
	}

	nosResult ExecuteNode(const nosNodeExecuteArgs* args) override
	{
		bluefish444::ChannelInfo* channelInfo = nullptr;
		for (size_t i = 0; i < args->PinCount; ++i)
		{
			auto& pin = args->Pins[i];
			if (pin.Name == NOS_NAME_STATIC("Channel"))
				channelInfo = nos::InterpretPinValue<bluefish444::ChannelInfo>(*pin.Data);
		}
		
		if (!channelInfo->device() || !channelInfo->channel())
			return NOS_RESULT_FAILED;

		auto device = BluefishDevice::GetDevice(channelInfo->device()->serial()->str());
		if (!device)
			return NOS_RESULT_FAILED;
		auto channel = static_cast<EBlueVideoChannel>(channelInfo->channel()->id());
		device->WaitForOutputVBI(channel, FieldCount);
		return NOS_RESULT_SUCCESS;
	}

	unsigned long FieldCount = 0;
};

nosResult RegisterWaitVBLNode(nosNodeFunctions* outFunctions)
{
	NOS_BIND_NODE_CLASS(NOS_NAME_STATIC("bluefish444.WaitVBL"), WaitVBLNodeContext, outFunctions)
	return NOS_RESULT_SUCCESS;
}

}