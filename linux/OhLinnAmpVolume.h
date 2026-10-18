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
    // aVolumeLimit: max Linn volume (from Volume.Limit), 0-100
    OhLinnAmpVolume(const std::string& aSerialPortPath, TUint aVolumeLimit = 100);
    ~OhLinnAmpVolume();

    TBool IsVolumeSupported() const;
    void SetStandby(TBool aOn);
    void SetBass(TInt aBass);
    void SetTreble(TInt aTreble);

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
    void SetTone(const char* aPrefix, TInt aValue);
    void SelectInput(int aSource);

private:
    OhSerialPort* iSerialPort;
    TUint iCurrentVolume;
    TUint iVolumeLimit;
    Bwh iDeviceId;
    Bwh iDeviceModel;
};

} // namespace Av
} // namespace OpenHome
