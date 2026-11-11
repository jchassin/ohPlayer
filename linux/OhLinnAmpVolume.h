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
    // Command dialects of the Linn $...$ RS232 protocol
    enum class EDialect {
        kLr2,   // preamplifiers: VOL / BAL / BAS / TRB / LISTEN
        kMajik, // integrated amplifiers: VOLUME / BALANCE / INPUT AUDIO, no tone commands
    };

    TBool SendCommandAndWaitResponse(const Brx& aCommand, Bwx& aResponse);
    TBool ProbeCommand(const char* aCommand, Bwx& aResponse);
    void SetDialect(EDialect aDialect);
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
    EDialect iDialect;
    TBool iStandbyOn;
    const char* iCmdVolumeSet;
    const char* iCmdVolumeQuery;
    const char* iCmdVolumeResp;
    const char* iCmdBalanceSet;
    const char* iCmdBassSet;
    const char* iCmdTrebleSet;
    std::string iIdValue;
    Bwh iDeviceId;
    Bwh iDeviceModel;
    Bwh iDeviceHwVersion;
    Bwh iDeviceSwVersion;
};

} // namespace Av
} // namespace OpenHome
