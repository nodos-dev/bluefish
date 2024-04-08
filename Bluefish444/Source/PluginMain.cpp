// Copyright MediaZ Teknoloji A.S. All Rights Reserved.

#include <Nodos/PluginAPI.h>
#include <Nodos/Helpers.hpp>

#include <nosVulkanSubsystem/nosVulkanSubsystem.h>

NOS_INIT()
NOS_VULKAN_INIT()

namespace bf
{

enum class Nodes : int
{
	Channel = 0,
	WaitVBL,
	DMAWrite,
	OutputNode,
	DMARead,
	InputNode,
	Count
};

nosResult RegisterChannelNode(nosNodeFunctions*);
nosResult RegisterWaitVBLNode(nosNodeFunctions*);
nosResult RegisterDMAWriteNode(nosNodeFunctions*);
nosResult RegisterOutputNode(nosNodeFunctions*);
nosResult RegisterDMAReadNode(nosNodeFunctions*);
nosResult RegisterInputNode(nosNodeFunctions*);

extern "C"
{
/// Nodos calls this function to retrieve the node type count, type names and function pointers.
/// The function first called with outList = nullptr to retrieve the count, then called again with a list of nosNodeFunctions.
NOSAPI_ATTR nosResult NOSAPI_CALL nosExportNodeFunctions(size_t* outCount, nosNodeFunctions** outFunctions) {
	*outCount = static_cast<size_t>(Nodes::Count);
	if (!outFunctions)
		return NOS_RESULT_SUCCESS;

	NOS_RETURN_ON_FAILURE(RequestVulkanSubsystem())
	NOS_RETURN_ON_FAILURE(RegisterChannelNode(outFunctions[static_cast<int>(Nodes::Channel)]))
	NOS_RETURN_ON_FAILURE(RegisterWaitVBLNode(outFunctions[static_cast<int>(Nodes::WaitVBL)]))
	NOS_RETURN_ON_FAILURE(RegisterDMAWriteNode(outFunctions[static_cast<int>(Nodes::DMAWrite)]))
	NOS_RETURN_ON_FAILURE(RegisterOutputNode(outFunctions[static_cast<int>(Nodes::OutputNode)]))
	NOS_RETURN_ON_FAILURE(RegisterDMAReadNode(outFunctions[static_cast<int>(Nodes::DMARead)]))
	NOS_RETURN_ON_FAILURE(RegisterInputNode(outFunctions[static_cast<int>(Nodes::InputNode)]))
    return NOS_RESULT_SUCCESS;
}

/// Nodos calls this function just before unloading the plugin DLL.
/// After this point, you must have your DLL dependencies unloaded (if any).
NOSAPI_ATTR void NOSAPI_CALL nosUnloadPlugin()
{
	return;
}
}
} // namespace bf
