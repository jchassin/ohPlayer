#include "OhLinnAmpVolume.h"
#include "OhSerialPort.h"
#include <OpenHome/Private/Printer.h>
#include <sstream>
#include <unistd.h>

namespace {

const char* const kCmdVolume = "VOL = ";
const char* const kCmdBalance = "BAL = ";
const char* const kCmdVolumeQuery = "VOL ?";
const char* const kCmdIdQuery = "ID ?";
const char* const kCmdListen = "LISTEN ";
const int kDefaultListenSource = 1;
const char* const kCmdStandbyOn = "STANDBY ON";
const char* const kCmdStandbyOff = "STANDBY OFF";
const OpenHome::TUint kResponseTimeoutMs = 500;
// OpenHome volume is in milli-dB: 100 steps of 1024 (see VolumeControl.cpp)
const OpenHome::TUint kMilliDbPerStep = 1024;

std::string BuildCommand(const char* aPrefix, int aValue) {
    std::ostringstream oss;
    oss << aPrefix << aValue;
    return oss.str();
}

}

namespace OpenHome {
namespace Av {

OhLinnAmpVolume::OhLinnAmpVolume(const std::string& aSerialPortPath, TUint aVolumeLimit)
    : iSerialPort(new OhSerialPort(aSerialPortPath, 9600))
    , iCurrentVolume(0)
    , iVolumeLimit(aVolumeLimit)
    , iDeviceId(64)
    , iDeviceModel(16)
{
    if (!iSerialPort->Open()) {
        Log::Print("OhLinnAmpVolume: Failed to open serial port %s\n", aSerialPortPath.c_str());
    } else {
        Log::Print("OhLinnAmpVolume: Connected to %s\n", aSerialPortPath.c_str());
        QueryId();
        // Align amp with OpenHome product initial state (standby at boot)
        SetStandby(true);
        SelectInput(kDefaultListenSource);
        QueryCurrentVolume();
    }
}

OhLinnAmpVolume::~OhLinnAmpVolume() {
    delete iSerialPort;
}

TBool OhLinnAmpVolume::IsVolumeSupported() const {
    return iSerialPort->IsOpen();
}

const Brx& OhLinnAmpVolume::DeviceId() const {
    return iDeviceId;
}

const Brx& OhLinnAmpVolume::DeviceModel() const {
    return iDeviceModel;
}

void OhLinnAmpVolume::SetVolume(TUint aVolume) {
    if (!IsVolumeSupported()) {
        Log::Print("OhLinnAmpVolume: Serial port not open\n");
        return;
    }

    TUint linnVolume = aVolume / kMilliDbPerStep;
    if (linnVolume > iVolumeLimit) {
        linnVolume = iVolumeLimit;
    }

    std::string command = BuildCommand(kCmdVolume, static_cast<int>(linnVolume));
    Brn cmd(reinterpret_cast<const TByte*>(command.c_str()), command.size());
    Bws<32> response;
    if (SendCommandAndWaitResponse(cmd, response)) {
        iCurrentVolume = linnVolume;
        Log::Print("OhLinnAmpVolume: Set volume to %u (Linn: %u)\n", aVolume, linnVolume);
    } else {
        Log::Print("OhLinnAmpVolume: Failed to send volume command\n");
    }
}

void OhLinnAmpVolume::SetBalance(TInt aBalance) {
    if (!IsVolumeSupported()) {
        return;
    }

    std::string command = BuildCommand(kCmdBalance, aBalance);
    Brn cmd(reinterpret_cast<const TByte*>(command.c_str()), command.size());
    Bws<32> response;
    if (!SendCommandAndWaitResponse(cmd, response)) {
        Log::Print("OhLinnAmpVolume: Failed to send balance command\n");
    }
}

void OhLinnAmpVolume::SetFade(TInt /*aFade*/) {
    // FADE is not supported by the LR2 RS232 command set (see LR2 spec section 3.3)
    if (!IsVolumeSupported()) {
        return;
    }
}

TBool OhLinnAmpVolume::SendCommandAndWaitResponse(const Brx& aCommand, Bwx& aResponse) {
    if (iSerialPort->SendCommandAndWaitResponse(aCommand, aResponse, kResponseTimeoutMs)) {
        return true;
    }
    // Single retry: response may be lost if the amp is busy or transitioning
    usleep(100000);
    return iSerialPort->SendCommandAndWaitResponse(aCommand, aResponse, kResponseTimeoutMs);
}

void OhLinnAmpVolume::QueryId() {
    if (!IsVolumeSupported()) {
        return;
    }

    Brn cmd(reinterpret_cast<const TByte*>(kCmdIdQuery), strlen(kCmdIdQuery));
    Bws<64> response;
    if (SendCommandAndWaitResponse(cmd, response)) {
        std::string respStr(reinterpret_cast<const char*>(response.Ptr()), response.Bytes());
        // Response format: !$ID LR2nnnnnnnnnnnn$ (model + 12-digit hex unique ID)
        const std::string prefix = "ID ";
        size_t idPos = respStr.find(prefix);
        if (idPos != std::string::npos) {
            std::string id = respStr.substr(idPos + prefix.length());
            // Strip trailing '$' from response framing
            size_t dollarPos = id.find('$');
            if (dollarPos != std::string::npos) {
                id = id.substr(0, dollarPos);
            }
            std::string model = id.substr(0, 3);
            std::string serial = id.substr(3);
            iDeviceId.Replace(Brn(serial.c_str()));
            iDeviceModel.Replace(Brn(model.c_str()));
            Log::Print("OhLinnAmpVolume: Linn %s detected (serial %s)\n", model.c_str(), serial.c_str());
        } else {
            Log::Print("OhLinnAmpVolume: Device ID: %s\n", respStr.c_str());
        }
    } else {
        Log::Print("OhLinnAmpVolume: Failed to query device ID\n");
    }
}

void OhLinnAmpVolume::SetStandby(TBool aOn) {
    if (!IsVolumeSupported()) {
        return;
    }

    const char* command = aOn ? kCmdStandbyOn : kCmdStandbyOff;
    Brn cmd(reinterpret_cast<const TByte*>(command), strlen(command));
    Bws<32> response;
    if (SendCommandAndWaitResponse(cmd, response)) {
        Log::Print("OhLinnAmpVolume: Standby %s\n", aOn ? "ON" : "OFF");
    } else {
        Log::Print("OhLinnAmpVolume: Failed to set standby %s\n", aOn ? "ON" : "OFF");
    }
}

void OhLinnAmpVolume::SelectInput(int aSource) {
    if (!IsVolumeSupported()) {
        return;
    }

    std::string command = BuildCommand(kCmdListen, aSource);
    Brn cmd(reinterpret_cast<const TByte*>(command.c_str()), command.size());
    Bws<32> response;
    if (SendCommandAndWaitResponse(cmd, response)) {
        std::string respStr(reinterpret_cast<const char*>(response.Ptr()), response.Bytes());
        Log::Print("OhLinnAmpVolume: Selected input %d\n", aSource);
    } else {
        Log::Print("OhLinnAmpVolume: Failed to select input %d\n", aSource);
    }
}

TBool OhLinnAmpVolume::QueryCurrentVolume() {
    if (!IsVolumeSupported()) {
        return false;
    }

    Brn cmd(reinterpret_cast<const TByte*>(kCmdVolumeQuery), strlen(kCmdVolumeQuery));
    Bws<32> response;
    if (SendCommandAndWaitResponse(cmd, response)) {
        std::string respStr(reinterpret_cast<const char*>(response.Ptr()), response.Bytes());
        size_t volumePos = respStr.find(kCmdVolume);
        if (volumePos != std::string::npos) {
            try {
                iCurrentVolume = std::stoi(respStr.substr(volumePos + strlen(kCmdVolume)));
                Log::Print("OhLinnAmpVolume: Current volume: %u\n", iCurrentVolume);
                return true;
            } catch (...) {
                Log::Print("OhLinnAmpVolume: Invalid volume response\n");
            }
        }
    }
    return false;
}

} // namespace Av
} // namespace OpenHome
