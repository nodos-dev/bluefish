#include <Nodos/PluginAPI.h>
#include <Nodos/PluginHelpers.hpp>

#include "BluefishTypes_generated.h"
#include "Device.hpp"
#include "ChannelHelpers.hpp"

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

	void OnNodeMenuRequested(const nosContextMenuRequest* request) override
	{
		flatbuffers::FlatBufferBuilder fbb;
		
		std::vector<flatbuffers::Offset<nos::ContextMenuItem>> items, devices;
		if (INPUT & GetChannelTypeFlags())
			EnumerateInputChannels(fbb, devices);
		if (OUTPUT & GetChannelTypeFlags())
			EnumerateOutputChannels(fbb, devices);
		if (!devices.empty())
			items.push_back(nos::CreateContextMenuItemDirect(fbb, "Open Channel", 0, &devices));

		if (items.empty())
			return;

		HandleEvent(nos::CreateAppEvent(fbb, nos::app::CreateAppContextMenuUpdateDirect(fbb, &NodeId, request->pos(), request->instigator(), &items)));
	}
	
	void OnPinMenuRequested(nos::Name pinName, const nosContextMenuRequest* request) override
	{
	}

	void OnMenuCommand(nosUUID itemID, uint32_t cmd) override
	{
		SelectChannelCommand& command = reinterpret_cast<SelectChannelCommand&>(cmd);
		auto device = BluefishDevice::GetDevice(command.DeviceId);
		if (!device)
		{
			nosEngine.LogE("No such Bluefish444 device found: %d", command.DeviceId);
			return;
		}
		bool input = IsInputChannel(command.Channel);
	    if (input)
	    {
	    	BErr err{};
	    	auto setup = device->GetSetupInfoForInput(command.Channel, err);
	    	command.VideoMode = setup.VideoModeExt;
	    	if (setup.SignalLinkType != SIGNAL_LINK_TYPE_SINGLE_LINK)
	    	{
	    		nosEngine.LogE("Unable to open input channel: Only SingleLink is supported for now.");
	    		return;
	    	}
	    }
		std::string pinName = "SingleLink " +  std::to_string(GetChannelNumber(command.Channel));
	    
        nos::bluefish::TChannelInfo channelPin{};
	    nos::bluefish::TDeviceId d;
	    d.serial = device->GetSerial();
	    d.name = device->GetName();
	    nos::bluefish::TChannelId c;
	    c.name = bfcUtilsGetStringForVideoChannel(command.Channel);
	    c.id = static_cast<int>(command.Channel);
        channelPin.device = std::make_unique<nos::bluefish::TDeviceId>(std::move(d));
        channelPin.channel = std::make_unique<nos::bluefish::TChannelId>(std::move(c));
        channelPin.video_mode = static_cast<int>(command.VideoMode);
        channelPin.video_mode_name = bfcUtilsGetStringForVideoMode(command.VideoMode);
		nosEngine.SetPinValue(ChannelPinId, nos::Buffer::From(channelPin));
		UpdateChannel(std::move(channelPin));
	}

	void OnNodeUpdated(const nos::fb::Node* updatedNode) override
	{
		LoadChannelInfo(updatedNode);
	}

	void OnPinValueChanged(nos::Name pinName, nosUUID pinId, nosBuffer value) override
	{
		if (pinName == NOS_NAME("Channel"))
		{
			nos::bluefish::TChannelInfo newInfo;
			flatbuffers::GetRoot<nos::bluefish::ChannelInfo>(value.Data)->UnPackTo(&newInfo);
			UpdateChannel(std::move(newInfo));
		}
	}

	void UpdateChannel(nos::bluefish::TChannelInfo info)
	{
		if (info == ChannelInfo)
			return;
		CloseChannel();
		ChannelInfo = std::move(info);
		OpenChannel();
	}
	
	void OpenChannel()
	{
		if (!ChannelInfo.device || !ChannelInfo.channel)
			return;
		auto device = BluefishDevice::GetDevice(ChannelInfo.device->serial);
		if (!device)
		{
			UpdateStatus(nos::fb::NodeStatusMessageType::FAILURE, "Unable to find Bluefish444 device:" + ChannelInfo.device->serial);
			return;
		}
		EVideoModeExt mode = static_cast<EVideoModeExt>(ChannelInfo.video_mode);
		auto channel = static_cast<EBlueVideoChannel>(ChannelInfo.channel->id);
		std::string channelStr = bfcUtilsGetStringForVideoChannel(channel);
		std::string modeStr = bfcUtilsGetStringForVideoMode(mode);
		if (IsInputChannel(channel))
			nosEngine.LogI("Route input %s", channelStr.c_str());
		else
			nosEngine.LogI("Route output %s with video mode %s", channelStr.c_str(), modeStr.c_str());

		auto err = device->OpenChannel(channel, mode);
		if (BERR_NO_ERROR == err)
			UpdateStatus(nos::fb::NodeStatusMessageType::INFO, channelStr + " " + modeStr);
		else
		{
			UpdateStatus(nos::fb::NodeStatusMessageType::FAILURE, "Unable to open channel " + channelStr + ": " + bfcUtilsGetStringForBErr(err));
			nosEngine.SetPinValue(ChannelPinId, nos::Buffer::From(nos::bluefish::TChannelInfo{}));
		}
	}

	void CloseChannel()
	{
		if (!ChannelInfo.device || !ChannelInfo.channel)
			return;
		auto device = BluefishDevice::GetDevice(ChannelInfo.device->serial);
		if (!device)
		{
			UpdateStatus(nos::fb::NodeStatusMessageType::FAILURE, "Unable to find Bluefish444 device:" + ChannelInfo.device->serial);
			return;
		}
		device->CloseChannel(static_cast<EBlueVideoChannel>(ChannelInfo.channel->id));
	}

	void UpdateStatus(nos::fb::NodeStatusMessageType type, std::string text)
	{
		auto device = BluefishDevice::GetDevice(ChannelInfo.device->serial);
		std::vector<nos::fb::TNodeStatusMessage> messages;
		if (device)
		{
			messages.push_back(nos::fb::TNodeStatusMessage{{}, device->GetName(), type});
		}
		messages.push_back(nos::fb::TNodeStatusMessage{{}, text, type});
		SetNodeStatusMessages(messages);
	}
};

}