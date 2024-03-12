#include <Nodos/PluginAPI.h>
#include <Nodos/Helpers.hpp>

NOS_INIT()

struct nosVulkanSubsystem* nosVulkan = nullptr;

namespace bf
{

enum class Nodes : int
{
	Channel = 0,
	WaitVBL,
	DMAWrite,
	Count
};

nosResult RegisterChannelNode(nosNodeFunctions*);
nosResult RegisterWaitVBLNode(nosNodeFunctions*);
nosResult RegisterDMAWriteNode(nosNodeFunctions*);

extern "C"
{
/// Nodos calls this function to retrieve the node type count, type names and function pointers.
/// The function first called with outList = nullptr to retrieve the count, then called again with a list of nosNodeFunctions.
NOSAPI_ATTR nosResult NOSAPI_CALL nosExportNodeFunctions(size_t* outCount, nosNodeFunctions** outFunctions) {
	*outCount = static_cast<size_t>(Nodes::Count);
	if (!outFunctions)
		return NOS_RESULT_SUCCESS;

	nosResult result;
	NOS_RETURN_ON_FAILURE(result, RegisterChannelNode(outFunctions[static_cast<int>(Nodes::Channel)]));
	NOS_RETURN_ON_FAILURE(result, RegisterWaitVBLNode(outFunctions[static_cast<int>(Nodes::WaitVBL)]));
	NOS_RETURN_ON_FAILURE(result, RegisterDMAWriteNode(outFunctions[static_cast<int>(Nodes::DMAWrite)]));
    return result;
}

/// Nodos calls this function just before unloading the plugin DLL.
/// After this point, you must have your DLL dependencies unloaded (if any).
NOSAPI_ATTR void NOSAPI_CALL nosUnloadPlugin()
{
	return;
}
}
} // namespace bf
