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
	bluefish444::TChannelInfo ChannelInfo{};
	
	ChannelNode(const nosFbNode* node) : NodeContext(node)
	{
		auto err = BluefishDevice::InitializeDevices();
		if (BERR_NO_ERROR != err)
		{
			SetNodeStatusMessage(std::string("Error during device initialization: ") + bfcUtilsGetStringForBErr(err), nos::fb::NodeStatusMessageType::FAILURE);
			return;
		}
		
	}
	
	void LoadNode(const nosFbNode* node)
	{
		if (auto* pins = node->pins())
		{
			bluefish444::TChannelInfo info;
			for (auto const* pin : *pins)
			{
				auto name = pin->name()->c_str();
				if (0 == strcmp(name, "Channel"))
				{
					ChannelPinId = *pin->id();
					flatbuffers::GetRoot<bluefish444::ChannelInfo>(pin->data()->data())->UnPackTo(&info);
				}
			}
			if (info.device)
				if (auto dev = BluefishDevice::GetDevice(info.device->serial))
					UpdateChannel(std::move(info));
		}
	}

	~ChannelNode() override
	{
	}

	void OnNodeMenuRequested(const nosContextMenuRequest* request) override
	{
		flatbuffers::FlatBufferBuilder fbb;
		
		std::vector<flatbuffers::Offset<nos::ContextMenuItem>> items, devices;
		EnumerateInputChannels(fbb, devices);
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
	    	auto setup = device->GetRecommendedSetupInfoForInput(command.Channel, err);
	    	command.VideoMode = setup.VideoModeExt;
	    	if (setup.SignalLinkType != SIGNAL_LINK_TYPE_SINGLE_LINK)
	    	{
	    		nosEngine.LogE("Unable to open input channel: Only SingleLink is supported for now.");
	    		return;
	    	}
	    }
		std::string pinName = "SingleLink " +  std::to_string(GetChannelNumber(command.Channel));
	    
        bluefish444::TChannelInfo channelPin{};
	    bluefish444::TDeviceId d;
	    d.serial = device->GetSerial();
	    d.name = device->GetName();
	    bluefish444::TChannelId c;
	    c.name = bfcUtilsGetStringForVideoChannel(command.Channel);
	    c.id = static_cast<int>(command.Channel);
        channelPin.device = std::make_unique<bluefish444::TDeviceId>(std::move(d));
        channelPin.channel = std::make_unique<bluefish444::TChannelId>(std::move(c));
        channelPin.video_mode = static_cast<int>(command.VideoMode);
        channelPin.video_mode_name = bfcUtilsGetStringForVideoMode(command.VideoMode);
		nosEngine.SetPinValue(ChannelPinId, nos::Buffer::From(channelPin));
		UpdateChannel(std::move(channelPin));
	}

	void OnNodeUpdated(const nos::fb::Node* updatedNode) override
	{
		LoadNode(updatedNode);
	}

	void OnPinValueChanged(nos::Name pinName, nosUUID pinId, nosBuffer value) override
	{
		if (pinName == NOS_NAME("Channel"))
		{
			bluefish444::TChannelInfo newInfo;
			flatbuffers::GetRoot<bluefish444::ChannelInfo>(value.Data)->UnPackTo(&newInfo);
			UpdateChannel(std::move(newInfo));
		}
	}

	void UpdateChannel(bluefish444::TChannelInfo info)
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
			UpdateStatus(nos::fb::NodeStatusMessageType::FAILURE, "Unable to find bluefish444 device:" + ChannelInfo.device->serial);
			return;
		}
		EVideoModeExt mode = VID_FMT_EXT_INVALID;
		auto channel = static_cast<EBlueVideoChannel>(ChannelInfo.channel->id);
		std::string channelStr = bfcUtilsGetStringForVideoChannel(channel);
		std::string modeStr;
		if (IsInputChannel(channel))
		{
			nosEngine.LogI("Route input %s", channelStr.c_str());
		}
		else
		{
			mode = static_cast<EVideoModeExt>(ChannelInfo.video_mode);
			modeStr = bfcUtilsGetStringForVideoMode(mode);
			nosEngine.LogI("Route output %s with video mode %s", channelStr.c_str(), modeStr.c_str());
		}
		
		if (BERR_NO_ERROR == device->RouteSignal(channel, mode))
			UpdateStatus(nos::fb::NodeStatusMessageType::INFO, channelStr + " " + modeStr);
		else
			UpdateStatus(nos::fb::NodeStatusMessageType::FAILURE, "Unable to open channel " + channelStr);
	}

	void CloseChannel()
	{
		
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

nosResult RegisterChannelNode(nosNodeFunctions* outFunctions)
{
	NOS_BIND_NODE_CLASS(NOS_NAME_STATIC("bluefish444.Channel"), ChannelNode, outFunctions)
	return NOS_RESULT_SUCCESS;
}

}