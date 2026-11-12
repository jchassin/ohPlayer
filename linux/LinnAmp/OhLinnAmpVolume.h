#pragma once

#include <OpenHome/Av/VolumeManager.h>
#include <OpenHome/Media/MuteManager.h>
#include <OpenHome/Buffer.h>
#include <memory>
#include <string>
#include "OhSerialPort.h"
#include "LinnAmpProtocol.h"

namespace OpenHome {
namespace Av {

class OhLinnAmpVolume : public IVolume, public IBalance, public IFade {
public:
    // aVolumeLimit: max Linn volume (from Volume.Limit), 0-100
    // aListenSource: amplifier input the renderer is connected to (LinnAmp.Source), 1-6
    OhLinnAmpVolume(const std::string& aSerialPortPath, TUint aVolumeLimit = 100, TUint aListenSource = 1);
    ~OhLinnAmpVolume();

    TBool IsVolumeSupported() const;
    void SetStandby(TBool aOn);
    void SetBass(TInt aBass);
    void SetTreble(TInt aTreble);

    // Info from the $ID ?$ query, empty if not detected
    const Brx& DeviceId() const;
    const Brx& DeviceModel() const;
    // Info from the $VERSION HARDWARE ?$ / $VERSION SOFTWARE ?$ queries,
    // empty if not detected or not supported by the amplifier
    const Brx& DeviceHwVersion() const;
    const Brx& DeviceSwVersion() const;

private: // from IVolume
    void SetVolume(TUint aVolume) override;

private: // from IBalance
    void SetBalance(TInt aBalance) override;

private: // from IFade
    void SetFade(TInt aFade) override;

private:
    TBool SendCommandAndWaitResponse(const Brx& aCommand, Bwx& aResponse);
    void QueryId();
    void DetectDialect();
    void QueryVersions();
    void QueryVersion(const char* aQuery, const char* aRespPrefix, Bwh& aOut, const char* aLabel);
    TBool QueryCurrentVolume();
    void SetTone(const char* aPrefix, TInt aValue);
    void SelectInput(int aSource);

private:
    OhSerialPort* iSerialPort;
    TUint iCurrentVolume;
    TUint iVolumeLimit;
    TBool iStandbyOn;
    std::unique_ptr<LinnAmpProtocol::ILinnAmpProtocol> iProtocol;
    int iListenSource;
    std::string iIdValue;
    Bwh iDeviceId;
    Bwh iDeviceModel;
    Bwh iDeviceHwVersion;
    Bwh iDeviceSwVersion;
};

} // namespace Av
} // namespace OpenHome
