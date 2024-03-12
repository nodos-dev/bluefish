#include <Nodos/PluginAPI.h>
#include <Nodos/PluginHelpers.hpp>

namespace bf
{

struct WaitVBLNodeContext : nos::NodeContext
{
	WaitVBLNodeContext(const nosFbNode* node) : NodeContext(node)
	{
		
	}
};

nosResult RegisterWaitVBLNode(nosNodeFunctions* outFunctions)
{
	NOS_BIND_NODE_CLASS(NOS_NAME_STATIC("bluefish444.WaitVBL"), WaitVBLNodeContext, outFunctions)
	return NOS_RESULT_SUCCESS;
}

}