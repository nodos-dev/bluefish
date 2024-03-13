#include "Device.hpp"

#include <BlueVelvetCUtils.h>

// stl
#include <sstream>

#include "Nodos/Modules.h"

extern nosEngineServices nosEngine;

namespace bf
{
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
}

BluefishDevice::~BluefishDevice()
{
	// TODO: Close channels
	if (auto err = bfcDetach(Instance))
		nosEngine.LogE("Error during detach: %s", bfcUtilsGetStringForBErr(err));
	bfcDestroy(Instance);
}

bool BluefishDevice::CanChannelDoInput(EBlueVideoChannel channel) const
{
	BErr err{};
	GetRecommendedSetupInfoForInput(channel, err);
	return BERR_NO_ERROR == err;
}

blue_setup_info BluefishDevice::GetRecommendedSetupInfoForInput(EBlueVideoChannel channel, BErr& err) const
{
	blue_setup_info setup{.DeviceId = GetId(), .VideoChannelInput = channel};
	err = bfcUtilsGetRecommendedSetupInfoInput(Instance, &setup, UHD_PREFERENCE_DEFAULT);
	return setup;
}

BErr BluefishDevice::RouteSignal(EBlueVideoChannel channel, EVideoModeExt mode)
{
	BErr err{};
	if (IsInputChannel(channel))
	{
		auto setup = GetRecommendedSetupInfoForInput(channel, err);
		if (BERR_NO_ERROR != err)
			return err;
		err = bfcUtilsValidateSetupInfo(&setup);
		if (BERR_NO_ERROR != err)
			return err;
		err = bfcUtilsSetupInput(Instance, &setup);
	}
	else
	{
		auto setup = bfcUtilsGetDefaultSetupInfoOutput(channel, mode);
		setup.MemoryFormat = MEM_FMT_2VUY;
		setup.VideoEngine = VIDEO_ENGINE_FRAMESTORE;
		setup.TransportSampling = Signal_FormatType_422;
		err = bfcUtilsValidateSetupInfo(&setup);
		if (BERR_NO_ERROR != err)
			return err;
		err = bfcUtilsSetupOutput(Instance, &setup);
		bfcSetCardProperty32(Instance, DEFAULT_VIDEO_OUTPUT_CHANNEL, channel);
	}
	return err;
}

bool BluefishDevice::DMAWriteFrame(uint32_t bufferId, uint8_t* buffer, uint32_t size, EBlueVideoChannel channel) const
{
	auto ret = bfcDmaWriteToCardAsync(Instance, buffer, size, nullptr, BlueImage_DMABuffer(bufferId, BLUE_DMA_DATA_TYPE_IMAGE_FRAME), 0);
	if(ret < 0)
	{
		nosEngine.LogE("BluefishDevice: Sync DMA Write returned with error code %d", ret);
		return false;
	}
	
	// Tell the card to playback this frame at the next interrupt - using this macros tells the card to playback, Image, VBI/Vanc and Hanc data.
	auto err = bfcRenderBufferUpdate(Instance, BlueBuffer_Image(bufferId));
	return err == BERR_NO_ERROR;
}

void BluefishDevice::WaitForOutputVBI(unsigned long& fieldCount, EBlueVideoChannel channel) const
{
	bfcWaitVideoOutputSync(Instance, UPD_FMT_FRAME, &fieldCount);
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


}