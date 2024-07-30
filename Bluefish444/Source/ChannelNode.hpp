/*
 * Copyright MediaZ Teknoloji A.S. All Rights Reserved.
 */

#include <Nodos/PluginAPI.h>
#include <Nodos/PluginHelpers.hpp>

#include "BluefishTypes_generated.h"

namespace bf
{

struct ChannelNode : nos::NodeContext
{
	nosUUID ChannelPinId{};
	nos::bluefish::TChannelInfo ChannelInfo{};

	ChannelNode(const nosFbNode* node);

	void LoadChannelInfo(const nosFbNode* node);

	~ChannelNode() override;

	enum ChannelType : uint8_t
	{
		INPUT = 1,
		OUTPUT = 2,
	};

	virtual uint8_t GetChannelTypeFlags()
	{
		return INPUT | OUTPUT;
	}

	void OnNodeMenuRequested(const nosContextMenuRequest* request) override;
	void OnMenuCommand(nosUUID itemID, uint32_t cmd) override;
	void OnNodeUpdated(const nos::fb::Node* updatedNode) override;
	void OnPinValueChanged(nos::Name pinName, nosUUID pinId, nosBuffer value) override;
	
	void UpdateChannel(nos::bluefish::TChannelInfo info);
	void OpenChannel();
	void CloseChannel();

	void UpdateStatus(nos::fb::NodeStatusMessageType type, std::string text);
};

}