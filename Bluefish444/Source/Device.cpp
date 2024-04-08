// Copyright MediaZ Teknoloji A.S. All Rights Reserved.

#include "Device.hpp"

#include <BlueVelvetCUtils.h>

// stl
#include <sstream>

#include "Nodos/Modules.h"

extern nosEngineServices nosEngine;

namespace bf
{
SdkInstance::SdkInstance()
{
	Handle = bfcFactory();
}

SdkInstance::~SdkInstance()
{
	Detach();
	bfcDestroy(Handle);
}

void SdkInstance::Attach(BluefishDevice* device)
{
	auto err = bfcAttach(Handle, device->GetId());
	if (BERR_NO_ERROR != err)
		nosEngine.LogE("Error while attaching device %d: %s", device->GetId(), bfcUtilsGetStringForBErr(err));
	else
		AttachedDevice = device->GetId();
}

void SdkInstance::Detach()
{
	if (AttachedDevice)
	{
		auto err = bfcDetach(Handle);
		if (BERR_NO_ERROR != err)
			nosEngine.LogE("Error while detaching device %d: %s", *AttachedDevice, bfcUtilsGetStringForBErr(err));
		AttachedDevice = std::nullopt;
	}
}

BErr BluefishDevice::InitializeDevices()
{
	static bool initialized = false;
	if (initialized)
		return BERR_NO_ERROR;
	auto instance = bfcFactory();
	
	BLUE_S32 deviceCount;
	if (auto err = bfcEnumerate(instance, &deviceCount))
		return err;
	if (deviceCount > 0)
	{
		for (int deviceId = 1; deviceId <= deviceCount; ++deviceId)
		{
			BErr deviceInitErr;
			auto device = std::make_shared<BluefishDevice>(deviceId, deviceInitErr);
			if (BERR_NO_ERROR != deviceInitErr)
			{
				nosEngine.LogE("Error during device initialization: %s", bfcUtilsGetStringForBErr(deviceInitErr));
				continue;
			}
			Devices[device->GetSerial()] = std::move(device);
		}
	}
	
	bfcDestroy(instance);
	initialized = true;
	return BERR_NO_ERROR;
}


BluefishDevice::BluefishDevice(BLUE_S32 deviceId, BErr& error) : Id(deviceId)
{
	Instance = bfcFactory();
	error = bfcAttach(Instance, Id);
	if (BERR_NO_ERROR != error)
		return;
	bfcUtilsGetDeviceInfo(Id, &Info);
	bfcSetCardProperty32(Instance, VIDEO_BLACKGENERATOR, ENUM_BLACKGENERATOR_OFF);
	bfcSetCardProperty32(Instance, VIDEO_IMAGE_ORIENTATION, ImageOrientation_Normal);
	bfcDetach(Instance);
}

BluefishDevice::~BluefishDevice()
{
	Channels.clear();
	bfcDestroy(Instance);
}

bool BluefishDevice::CanChannelDoInput(EBlueVideoChannel channel) const
{
	blue_setup_info setup = bfcUtilsGetDefaultSetupInfoInput(channel);
	setup.DeviceId = GetId();
	SdkInstance instance; // For some reason, we cannot reliably call below function without a fresh instance.
	auto err = bfcUtilsGetSetupInfoForInputSignal(instance, &setup, UHD_PREFERENCE_DEFAULT);
	return BERR_NO_ERROR == err;
}

blue_setup_info BluefishDevice::GetSetupInfoForInput(EBlueVideoChannel channel, BErr& err) const
{
	blue_setup_info setup = bfcUtilsGetDefaultSetupInfoInput(channel);
	err = bfcUtilsGetSetupInfoForInputSignal(Instance, &setup, UHD_PREFERENCE_DEFAULT);
	if (BERR_NO_ERROR != err)
	{
		err = bfcUtilsGetRecommendedSetupInfoInput(Instance, &setup, UHD_PREFERENCE_DEFAULT);
		if (BERR_NO_ERROR != err)
			return setup;
	}
	setup.SignalLinkType = SIGNAL_LINK_TYPE_SINGLE_LINK; // Support only single link for now
	setup.MemoryFormat = MEM_FMT_2VUY;
	setup.VideoEngine = VIDEO_ENGINE_FRAMESTORE;
	setup.TransportSampling = Signal_FormatType_422;
	return setup;
}

BErr BluefishDevice::OpenChannel(EBlueVideoChannel channel, EVideoModeExt mode)
{
	CloseChannel(channel);
	BErr error;
	auto chObject = std::make_unique<Channel>(this, channel, mode, error);
	if (BERR_NO_ERROR != error)
		return error;
	Channels[channel] = std::move(chObject);
	return error;
}

void BluefishDevice::CloseChannel(EBlueVideoChannel channel)
{
	auto it = Channels.find(channel);
	if (it != Channels.end())
		Channels.erase(it);
}

bool BluefishDevice::DMAWriteFrame(EBlueVideoChannel channel, uint32_t bufferId, uint8_t* inBuffer, uint32_t size) const
{
	auto it = Channels.find(channel);
	if (it == Channels.end())
	{
		nosEngine.LogE("DMA Write: Channel %s not open!", bfcUtilsGetStringForVideoChannel(channel));
		return false;
	}
	return it->second->DMAWriteFrame(bufferId, inBuffer, size);
}

bool BluefishDevice::DMAReadFrame(EBlueVideoChannel channel, uint32_t nextCaptureBufferId, uint32_t readBufferId, uint8_t* outBuffer, uint32_t size) const
{
	auto it = Channels.find(channel);
	if (it == Channels.end())
	{
		nosEngine.LogE("DMA Read: Channel %s not open!", bfcUtilsGetStringForVideoChannel(channel));
		return false;
	}
	return it->second->DMAReadFrame(nextCaptureBufferId, readBufferId, outBuffer, size);
}

bool BluefishDevice::WaitVBI(EBlueVideoChannel channel, unsigned long& fieldCount) const
{
	auto it = Channels.find(channel);
	if (it == Channels.end())
	{
		nosEngine.LogE("Wait VBI: Channel %s not open!", bfcUtilsGetStringForVideoChannel(channel));
		return false;
	}
	return it->second->WaitVBI(fieldCount);
}

std::array<uint32_t, 2> BluefishDevice::GetDeltaSeconds(EBlueVideoChannel channel) const
{
	auto it = Channels.find(channel);
	if (it == Channels.end())
		return {0, 0};
	return it->second->GetDeltaSeconds();
}

std::shared_ptr<BluefishDevice> BluefishDevice::GetDevice(std::string const& serial)
{
	auto it = Devices.find(serial);
	if (it == Devices.end())
		return nullptr;
	return it->second;
}

std::shared_ptr<BluefishDevice> BluefishDevice::GetDevice(BLUE_S32 id)
{
	for (auto& [serial, device] : Devices)
	{
		if (device->GetId() == id)
			return device;
	}
	return nullptr;
}

void BluefishDevice::ForEachDevice(std::function<void(BluefishDevice&)>&& func)
{
	for (auto& [serial, device] : Devices)
		func(*device);
}

std::string BluefishDevice::GetSerial() const
{
	std::stringstream ss;
	for (int i = 0; i < 32; ++i)
	{
		auto c = Info.CardSerialNumber[i];
		if (!c)
			break;
		ss << c;
	}
	return ss.str();
}

std::string BluefishDevice::GetName() const
{
	return bfcUtilsGetStringForCardType(Info.CardType);
}

Channel::Channel(BluefishDevice* device, EBlueVideoChannel channel, EVideoModeExt mode, BErr& err)
	: Device(device), VideoChannel(channel)
{
	Instance = bfcFactory();
	err = bfcAttach(Instance, Device->GetId());
	if (BERR_NO_ERROR != err)
		return;
	if (IsInputChannel(channel))
	{
		auto setup = device->GetSetupInfoForInput(channel, err);
		if (BERR_NO_ERROR != err)
			return;
		err = bfcUtilsValidateSetupInfo(&setup);
		if (BERR_NO_ERROR != err)
			return;
		err = bfcUtilsSetupInput(Instance, &setup);
		mode = setup.VideoModeExt;
	}
	else
	{
		auto setup = bfcUtilsGetDefaultSetupInfoOutput(channel, mode);
		setup.MemoryFormat = MEM_FMT_2VUY;
		setup.VideoEngine = VIDEO_ENGINE_FRAMESTORE;
		setup.TransportSampling = Signal_FormatType_422;
		setup.SignalLinkType = SIGNAL_LINK_TYPE_SINGLE_LINK; // Support only single link for now
		if (BERR_NO_ERROR != err)
			return;
		err = bfcUtilsValidateSetupInfo(&setup);
		if (BERR_NO_ERROR != err)
			return;
		err = bfcUtilsSetupOutput(Instance, &setup);
	}
	uint32_t dividend = bfcUtilsGetFpsForVideoMode(mode) * (bfcUtilsIsVideoMode1001Framerate(mode) ? 1000 : 1);
	uint32_t divisor = bfcUtilsIsVideoMode1001Framerate(mode) ? 1001 : 1;
	DeltaSeconds = {divisor, dividend};
}

Channel::~Channel()
{
	if (!Instance)
		return;
	if (auto err = bfcDetach(Instance))
		nosEngine.LogE("Error during detach: %s", bfcUtilsGetStringForBErr(err));
	bfcDestroy(Instance);
}

bool Channel::DMAWriteFrame(uint32_t bufferId, uint8_t* inBuffer, uint32_t size)
{
	auto ret = bfcDmaWriteToCardAsync(Instance, inBuffer, size, nullptr, BlueImage_DMABuffer(bufferId, BLUE_DMA_DATA_TYPE_IMAGE_FRAME), 0);
	if(ret < 0)
	{
		nosEngine.LogE("DMA Write returned with '%s'", bfcUtilsGetStringForBErr(ret));
		return false;
	}
	
	// Tell the card to playback this frame at the next interrupt - using this macros tells the card to playback, Image, VBI/Vanc and Hanc data.
	auto err = bfcRenderBufferUpdate(Instance, BlueBuffer_Image(bufferId));
	return err == BERR_NO_ERROR;
}

bool Channel::DMAReadFrame(uint32_t nextCaptureBufferId, uint32_t readBufferId, uint8_t* outBuffer, uint32_t size)
{
	auto err = bfcRenderBufferCapture(Instance, BlueBuffer_Image(nextCaptureBufferId));
	if (err != BERR_NO_ERROR)
		nosEngine.LogE("DMA Read: Cannot set read buffer to %d", nextCaptureBufferId);
	auto ret = bfcDmaReadFromCardAsync(Instance, outBuffer, size, nullptr, BlueImage_DMABuffer(readBufferId, BLUE_DMA_DATA_TYPE_IMAGE_FRAME), 0);
	if (ret < 0)
	{
		nosEngine.LogE("DMA Read returned with '%s'", bfcUtilsGetStringForBErr(ret));
		return false;
	}
	return err == BERR_NO_ERROR;
}

bool Channel::WaitVBI(unsigned long& fieldCount) const
{
	return BERR_NO_ERROR == (IsInputChannel(VideoChannel)
		                         ? bfcWaitVideoInputSync(Instance, UPD_FMT_FRAME, &fieldCount)
		                         : bfcWaitVideoOutputSync(Instance, UPD_FMT_FRAME, &fieldCount));
}

}