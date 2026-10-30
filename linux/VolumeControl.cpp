#include "OhLog.h"
#include <OpenHome/Media/Pipeline/Msg.h>
#include <OpenHome/Av/VolumeManager.h>
#include <OpenHome/Private/Printer.h>

#include <alsa/asoundlib.h>
#include <math.h>
#include <OpenHome/Private/Printer.h>
#include "VolumeControl.h"

using namespace OpenHome;
using namespace OpenHome::Av;
using namespace OpenHome::Media;

VolumeControl::VolumeControl()
: iHandle(NULL)
, iElem(NULL)
{
    const TChar *CARD          = "default";
    const TChar *SELEM_NAMES[] = {"Digital", "PCM", "Master"};
    // Get the mixer element for the default sound card.
    if (snd_mixer_open(&iHandle, 0) < 0) {
        iHandle = NULL;
        OhLog::PrintError("VolumeControl: snd_mixer_open failed\n");
        return;
    }
    if (snd_mixer_attach(iHandle, CARD) < 0) {
        OhLog::PrintError("VolumeControl: snd_mixer_attach(%s) failed\n", CARD);
        snd_mixer_close(iHandle);
        iHandle = NULL;
        return;
    }
    if (snd_mixer_selem_register(iHandle, NULL, NULL) < 0 ||
        snd_mixer_load(iHandle) < 0) {
        OhLog::PrintError("VolumeControl: failed to load ALSA mixer elements\n");
        snd_mixer_close(iHandle);
        iHandle = NULL;
        return;
    }
    // Get the mixer element for the most relevant control.
    snd_mixer_selem_id_t *iSid;

    snd_mixer_selem_id_alloca(&iSid);
    snd_mixer_selem_id_set_index(iSid, 0);

    int nelems = sizeof(SELEM_NAMES)/sizeof(TChar*);

    for (int i=0; i<nelems; i++)
    {
        snd_mixer_selem_id_set_name(iSid, SELEM_NAMES[i]);

        iElem = snd_mixer_find_selem(iHandle, iSid);

        // Quit the loop if control found.
        if (iElem != NULL)
        {
            break;
        }
    }
    if (iElem != NULL) {
        if (snd_mixer_selem_has_playback_switch(iElem)) {
            OhLog::Print("VolumeControl: unmuting playback switch\n");
            snd_mixer_selem_set_playback_switch_all(iElem, 1);
        }
    } else {
        // No volume control on this sound card: output runs at full scale.
        OhLog::PrintWarning("VolumeControl: no ALSA volume control found - output at full scale\n");
    }

}

VolumeControl::~VolumeControl()
{
    if (iHandle != NULL) {
        snd_mixer_close(iHandle);
    }
}

TBool VolumeControl::IsVolumeSupported()
{
    return (iElem != NULL);
}

void VolumeControl::SetVolumeFixed(TUint aVolume)
{
    SetVolume(aVolume);
}

void VolumeControl::SetVolume(TUint aVolume)
{
    const long  MAX_LINEAR_DB_SCALE = 24;
    const TUint MILLI_DB_PER_STEP   = 1024;
    double      volume;
    double      min_norm;
    long        min, max, value;
    TInt        err;

    // Sanity Check
    if (! IsVolumeSupported())
    {
        return;
    }
    OhLog::Print("VolumeControl: set volume %u\n", aVolume);
    volume = double((aVolume / MILLI_DB_PER_STEP)/100.0f);

    // Use the dB range to map the volume to a scale more in tune
    // with the human ear, if possible.
    err = snd_mixer_selem_get_playback_dB_range(iElem, &min, &max);

    if (err < 0 || min >= max) {
        // dB range not available, use a linear volume mapping.
        err = snd_mixer_selem_get_playback_volume_range(iElem, &min, &max);
        if (err < 0)
        {
            OhLog::PrintError("VolumeControl: failed to get playback volume range\n");
            return;
        }

        value = lrint(floor(volume * (max - min))) + min;
        snd_mixer_selem_set_playback_volume_all(iElem, value);

        return;
    }

    if (max - min <= MAX_LINEAR_DB_SCALE * 100)
    {
        // dB range less than 24 dB, use a linear mapping
        value = lrint(floor(volume * (max - min))) + min;
        snd_mixer_selem_set_playback_dB_all(iElem, value, -1);
        return;
    }

    if (min != SND_CTL_TLV_DB_GAIN_MUTE) {
        min_norm = exp10((min - max) / 6000.0);
        volume = volume * (1 - min_norm) + min_norm;
    }
    value = lrint(floor(6000.0 * log10(volume))) + max;
    snd_mixer_selem_set_playback_switch_all(iElem, 1);
    snd_mixer_selem_set_playback_dB_all(iElem, value, -1);

    return;
}

void VolumeControl::SetBalance(TInt /*aBalance*/)
{
    // Not Implemented
}

void VolumeControl::SetFade(TInt /*aFade*/)
{
    // Not Implemented
}
