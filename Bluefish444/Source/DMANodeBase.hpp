/*
 * Copyright MediaZ Teknoloji A.S. All Rights Reserved.
 */

#pragma once
#include <Nodos/PluginHelpers.hpp>
#include "Device.hpp"

namespace bf
{

struct DMANodeBase : nos::NodeContext
{
	DMANodeBase(const nosFbNode* node) : NodeContext(node)
	{
	}

	static constexpr uint32_t CycledBuffersPerChannel = 4;
	uint32_t BufferId = 0;
};

}