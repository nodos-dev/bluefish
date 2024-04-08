// Copyright MediaZ Teknoloji A.S. All Rights Reserved.

#include <Nodos/PluginAPI.h>
#include <Nodos/PluginHelpers.hpp>

#include "BluefishTypes_generated.h"
#include "Device.hpp"
#include "ChannelNode.hpp"

namespace bf
{

struct OutputNode : ChannelNode
{
	OutputNode(const nosFbNode* node) : ChannelNode(node)
	{
	}

	uint8_t GetChannelTypeFlags() override { return OUTPUT; }
};

nosResult RegisterOutputNode(nosNodeFunctions* outFunctions)
{
	NOS_BIND_NODE_CLASS(NOS_NAME("Output"), OutputNode, outFunctions)
	return NOS_RESULT_SUCCESS;
}

}