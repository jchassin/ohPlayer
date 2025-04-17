#pragma once

#include <OpenHome/Av/RebootHandler.h>
#include <OpenHome/Av/VolumeManager.h>
#include <OpenHome/Media/MuteManager.h>
#include <OpenHome/Private/Thread.h>

#include <alsa/asoundlib.h>

namespace OpenHome {
namespace Av {

class VolumeControl : public IVolume, public IBalance, public IFade
{
public:
    VolumeControl();
    ~VolumeControl();
    TBool IsVolumeSupported();
private:
    snd_mixer_t          *iHandle;    // ALSA mixer handle.
    snd_mixer_elem_t     *iElem;      // PCM mixer element
private: // from IVolume
    void SetVolume(TUint aVolume) override;
private: // from IBalance
    void SetBalance(TInt aBalance) override;
private: // from IFade
    void SetFade(TInt aFade) override;
};

} // namespace Av
} // namespace OpenHome
