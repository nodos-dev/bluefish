// Copyright MediaZ Teknoloji A.S. All Rights Reserved.

#include <Nodos/PluginAPI.h>
#include <Nodos/PluginHelpers.hpp>

#include "BluefishTypes_generated.h"
#include "Device.hpp"
#include "ChannelNode.hpp"

namespace bf
{

struct InputNode : ChannelNode
{
	InputNode(const nosFbNode* node) : ChannelNode(node)
	{
	}

	uint8_t GetChannelTypeFlags() override { return INPUT; }
};

nosResult RegisterInputNode(nosNodeFunctions* outFunctions)
{
	NOS_BIND_NODE_CLASS(NOS_NAME("Input"), InputNode, outFunctions)
	return NOS_RESULT_SUCCESS;
}

}