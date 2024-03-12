#pragma once

#include <Nodos/PluginHelpers.hpp>

#include "Device.hpp"

namespace bf
{

struct SelectChannelCommand
{
	BLUE_S32 DeviceId : 4;
	EBlueVideoChannel Channel : 5;
	EVideoModeExt VideoMode : 12;
	operator uint32_t() const { return *(uint32_t*)this; }
};
static_assert(sizeof(SelectChannelCommand) == sizeof(uint32_t));

struct FormatDescriptor
{
	EVideoModeExt Format;
	u32 Width, Height;
	u8 Interlaced : 1;
};

inline nos::fb::vec2u UnpackU64(uint64_t val)
{
	return nos::fb::vec2u((val >> 32) & 0xFFFFFFFF, val & 0xFFFFFFFF);
}

inline uint64_t PackU64(nos::fb::vec2u val)
{
	return static_cast<uint64_t>(val.x()) << 32 | val.y();
}

inline auto EnumerateFormats()
{
	struct FormatDescriptor
	{
		EVideoModeExt Format;
		nos::fb::vec2u DeltaSeconds;
		u32 Width, Height;
		u8 Interlaced : 1;
	};

	std::unordered_map<uint64_t, std::unordered_map<uint64_t, std::vector<FormatDescriptor>>> extDeltaSecDescMap;
	for (auto fmt = VID_FMT_EXT_1080I_5000; fmt < VID_FMT_EXT_LAST_ENTRY_V1; fmt = EVideoModeExt(fmt + 1))
	{
		if (bfcUtilsIsVideoModePsF(fmt))
			continue;
		bool interlaced = !bfcUtilsIsVideoModeProgressive(fmt);
		BLUE_U32 w, h;
		bfcGetVideoWidth(fmt, &w);
		bfcGetVideoHeight(fmt, UPD_FMT_FRAME, &h);
		uint32_t dividend = bfcUtilsGetFpsForVideoMode(fmt) * (bfcUtilsIsVideoMode1001Framerate(fmt) ? 1000 : 1);
		uint32_t divisor = bfcUtilsIsVideoMode1001Framerate(fmt) ? 1001 : 1;
		auto desc = FormatDescriptor{
			.Format = fmt,
			.DeltaSeconds = {dividend, divisor},
			.Width = w,
			.Height = h,
			.Interlaced = interlaced,
		};
		extDeltaSecDescMap[PackU64({w, h})][PackU64(desc.DeltaSeconds)].push_back(desc);
	}
	return extDeltaSecDescMap;
}

inline void EnumerateInputChannels(flatbuffers::FlatBufferBuilder& fbb, std::vector<flatbuffers::Offset<nos::ContextMenuItem>>& devices)
{
	BluefishDevice::ForEachDevice([&](BluefishDevice& device) {
		std::vector<flatbuffers::Offset<nos::ContextMenuItem>> inChannels;
		for (EBlueVideoChannel ch = BLUE_VIDEO_OUTPUT_CHANNEL_1; ch < BLUE_VIDEO_INPUT_CHANNEL_8;
		     ch = static_cast<EBlueVideoChannel>(static_cast<int>(ch) + 1))
		{
			SelectChannelCommand command = {
				.DeviceId = device.GetId(),
				.Channel = ch
			};
			if (!IsInputChannel(ch))
				continue;
			std::string channelName = bfcUtilsGetStringForVideoChannel(ch);
			ReplaceString(channelName, "Input ", "");
			if (device.CanChannelDoInput(ch))
				inChannels.push_back(nos::CreateContextMenuItemDirect(fbb, channelName.c_str(), command));
			if (!inChannels.empty())
				devices.push_back(nos::CreateContextMenuItemDirect(fbb, "Input", 0, &inChannels));
		}
	});
}

inline void EnumerateOutputChannels(flatbuffers::FlatBufferBuilder& fbb, std::vector<flatbuffers::Offset<nos::ContextMenuItem>>& devices)
{
	BluefishDevice::ForEachDevice([&](BluefishDevice& device) {
		std::vector<flatbuffers::Offset<nos::ContextMenuItem>> outChannels;
		for (EBlueVideoChannel ch = BLUE_VIDEO_OUTPUT_CHANNEL_1; ch < BLUE_VIDEO_INPUT_CHANNEL_8; ch = static_cast<EBlueVideoChannel>(static_cast<int>(ch) + 1))
		{
			SelectChannelCommand command = {
				.DeviceId = device.GetId(),
				.Channel = ch
			};
			if (IsInputChannel(ch))
				continue;
			std::string channelName = bfcUtilsGetStringForVideoChannel(ch);
			ReplaceString(channelName, "Output ", "");
			std::vector<flatbuffers::Offset<nos::ContextMenuItem>> extents;
			static auto descriptors = EnumerateFormats();
			for (auto& [extPacked, deltaSecDescMap] : descriptors)
			{
				auto extent = UnpackU64(extPacked);
				std::vector<flatbuffers::Offset<nos::ContextMenuItem>> frameRates;
				for (auto& [deltaSecPacked, descs] : deltaSecDescMap)
				{
					auto deltaSec = UnpackU64(deltaSecPacked);
					std::vector<flatbuffers::Offset<nos::ContextMenuItem>> formats;
					for (auto& desc : descs)
					{
						command.VideoMode = desc.Format;
						auto name = desc.Interlaced ? "Interlaced" : "Progressive";
						formats.push_back(nos::CreateContextMenuItemDirect(fbb, name, command));
					}
					if (!formats.empty())
					{
						float frameRate = static_cast<float>(deltaSec.x()) / static_cast<float>(deltaSec.y());
						char buf[16] = {};
						std::sprintf(buf, "%.2f", frameRate);
						frameRates.push_back(nos::CreateContextMenuItemDirect(fbb, buf, 0, &formats));
					}
				}
				if (!frameRates.empty())
				{
					char buf[32] = {};
					std::sprintf(buf, "%dx%d", extent.x(), extent.y());
					extents.push_back(nos::CreateContextMenuItemDirect(fbb, buf, 0, &frameRates));
				}
			}
			if (!extents.empty())
				outChannels.push_back(nos::CreateContextMenuItemDirect(fbb, channelName.c_str(), 0, &extents));
		}
		if (!outChannels.empty())
			devices.push_back(nos::CreateContextMenuItemDirect(fbb, "Output", 0, &outChannels));
	});
}

}
