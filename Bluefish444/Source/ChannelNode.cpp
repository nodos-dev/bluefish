#include <Nodos/PluginAPI.h>
#include <Nodos/PluginHelpers.hpp>

#include "BluefishTypes_generated.h"
#include "Device.hpp"
#include "ChannelNode.hpp"

namespace bf
{

ChannelNode::ChannelNode(const nosFbNode* node): NodeContext(node)
{
	auto err = BluefishDevice::InitializeDevices();
	if (BERR_NO_ERROR != err)
	{
		SetNodeStatusMessage(std::string("Error during device initialization: ") + bfcUtilsGetStringForBErr(err), nos::fb::NodeStatusMessageType::FAILURE);
		return;
	}
	LoadChannelInfo(node);
}

void ChannelNode::LoadChannelInfo(const nosFbNode* node)
{
	if (auto* pins = node->pins())
	{
		nos::bluefish::TChannelInfo info;
		for (auto const* pin : *pins)
		{
			auto name = pin->name()->c_str();
			if (0 == strcmp(name, "Channel"))
			{
				ChannelPinId = *pin->id();
				flatbuffers::GetRoot<nos::bluefish::ChannelInfo>(pin->data()->data())->UnPackTo(&info);
			}
		}
		UpdateChannel(std::move(info));
	}
}

ChannelNode::~ChannelNode()
{
	CloseChannel();
}

nosResult RegisterChannelNode(nosNodeFunctions* outFunctions)
{
	NOS_BIND_NODE_CLASS(NOS_NAME("Channel"), ChannelNode, outFunctions)
	return NOS_RESULT_SUCCESS;
}

}