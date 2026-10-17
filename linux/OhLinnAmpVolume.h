#pragma once

#include <OpenHome/Av/VolumeManager.h>
#include <OpenHome/Media/MuteManager.h>
#include <OpenHome/Buffer.h>
#include <string>
#include "OhSerialPort.h"

namespace OpenHome {
namespace Av {

class OhLinnAmpVolume : public IVolume, public IBalance, public IFade {
public:
    OhLinnAmpVolume(const std::string& aSerialPortPath);
    ~OhLinnAmpVolume();

    TBool IsVolumeSupported() const;
    void SetStandby(TBool aOn);

    // Info from the $ID ?$ query, empty if not detected
    const Brx& DeviceId() const;
    const Brx& DeviceModel() const;

private: // from IVolume
    void SetVolume(TUint aVolume) override;

private: // from IBalance
    void SetBalance(TInt aBalance) override;

private: // from IFade
    void SetFade(TInt aFade) override;

private:
    TBool SendCommandAndWaitResponse(const Brx& aCommand, Bwx& aResponse);
    TBool QueryCurrentVolume();
    void QueryId();
    void SelectInput(int aSource);

private:
    OhSerialPort* iSerialPort;
    TUint iCurrentVolume;
    Bwh iDeviceId;
    Bwh iDeviceModel;
};

} // namespace Av
} // namespace OpenHome
