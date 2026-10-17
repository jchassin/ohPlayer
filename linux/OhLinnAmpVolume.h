#pragma once

#include <OpenHome/Av/VolumeManager.h>
#include <OpenHome/Media/MuteManager.h>
#include <string>
#include "OhSerialPort.h"

namespace OpenHome {
namespace Av {

class OhLinnAmpVolume : public IVolume, public IBalance, public IFade {
public:
    OhLinnAmpVolume(const std::string& aSerialPortPath);
    ~OhLinnAmpVolume();

    TBool IsVolumeSupported() const;

private: // from IVolume
    void SetVolume(TUint aVolume) override;

private: // from IBalance
    void SetBalance(TInt aBalance) override;

private: // from IFade
    void SetFade(TInt aFade) override;

private:
    TBool SendCommand(const char* aCommand);
    TBool QueryCurrentVolume();

private:
    OhSerialPort* iSerialPort;
    TUint iCurrentVolume;
};

} // namespace Av
} // namespace OpenHome
