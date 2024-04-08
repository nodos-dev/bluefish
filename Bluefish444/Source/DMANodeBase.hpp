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

	static uint32_t GetBufferId(EBlueVideoChannel ch, uint32_t idx)
	{
		return static_cast<int>(ch) * CycledBuffersPerChannel + idx;
	}

	static constexpr uint32_t CycledBuffersPerChannel = 2;
	uint32_t BufferIdx = 0;
};

}