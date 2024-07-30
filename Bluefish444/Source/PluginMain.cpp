// Copyright MediaZ Teknoloji A.S. All Rights Reserved.

#include <Nodos/PluginAPI.h>
#include <Nodos/Helpers.hpp>

#include <nosVulkanSubsystem/nosVulkanSubsystem.h>

#define IMPLEMENTATION_BLUEVELVETC_FUNC_PTR
#define LOAD_FUNC_PTR_V6_5_3
#include <BlueVelvetCFuncPtr.h>

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

NOSAPI_ATTR nosResult NOSAPI_CALL ExportNodeFunctions(size_t* outCount, nosNodeFunctions** outFunctions)
{
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

NOSAPI_ATTR nosResult NOSAPI_CALL Initialize()
{
	if (!LoadFunctionPointers_BlueVelvetC())
	{
		constexpr auto text = "Failed to load BlueVelvetC function pointers. Make sure you have at Bluefish SDK with version at least 6.5.3.";
		nosModuleStatusMessage message{
			.ModuleId = nosEngine.Context->Id,
		    .UpdateType = NOS_MODULE_STATUS_MESSAGE_UPDATE_TYPE_REPLACE,
			.MessageType = NOS_MODULE_STATUS_MESSAGE_TYPE_ERROR,
			.Message = text
		};
		nosEngine.SendModuleStatusMessageUpdate(&message);
		return NOS_RESULT_FAILED;
	}
	return NOS_RESULT_SUCCESS;
}

extern "C"
{
/// Nodos calls this function to retrieve the plugin functions.
NOSAPI_ATTR nosResult NOSAPI_CALL nosExportPlugin(nosPluginFunctions* out)
{
	out->ExportNodeFunctions = bf::ExportNodeFunctions;
	out->Initialize = bf::Initialize;
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
