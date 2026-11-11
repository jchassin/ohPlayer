#include "OhLinnAmpVolume.h"
#include "OhSerialPort.h"
#include "LinnAmpProtocol.h"
#include <OpenHome/Private/Printer.h>
#include "OhLog.h"
#include <sstream>
#include <cstring>
#include <unistd.h>

namespace {

const char* const kCmdIdQuery = "ID ?";
const char* const kCmdListen = "LISTEN ";
const int kDefaultListenSource = 1;
const char* const kCmdStandbyOn = "STANDBY ON";
const char* const kCmdStandbyOff = "STANDBY OFF";
const char* const kCmdVersionHwQuery = "VERSION HARDWARE ?";
const char* const kCmdVersionSwQuery = "VERSION SOFTWARE ?";
const char* const kRespVersionHw = "VERSION HARDWARE ";
const char* const kRespVersionSw = "VERSION SOFTWARE ";
// LR2 preamplifier command set
const char* const kLr2VolumeSet = "VOL = ";
const char* const kLr2VolumeQuery = "VOL ?";
const char* const kLr2BalanceSet = "BAL = ";
const char* const kLr2BassSet = "BAS = ";
const char* const kLr2TrebleSet = "TRB = ";
// Majik integrated amplifier command set: VOLUME instead of VOL, BALANCE
// instead of BAL, INPUT AUDIO instead of LISTEN, no tone commands
const char* const kMajikVolumeSet = "VOLUME = ";
const char* const kMajikVolumeQuery = "VOLUME ?";
const char* const kMajikVolumeResp = "VOLUME ";
const char* const kMajikBalanceSet = "BALANCE = ";
const OpenHome::TUint kResponseTimeoutMs = 500;
// OpenHome volume is in milli-dB: 100 steps of 1024 (see VolumeControl.cpp)
const OpenHome::TUint kMilliDbPerStep = 1024;

std::string BuildCommand(const char* aPrefix, int aValue) {
    std::ostringstream oss;
    oss << aPrefix << aValue;
    return oss.str();
}

std::string ToString(const OpenHome::Brx& aBuf) {
    return std::string(reinterpret_cast<const char*>(aBuf.Ptr()), aBuf.Bytes());
}

}

namespace OpenHome {
namespace Av {

OhLinnAmpVolume::OhLinnAmpVolume(const std::string& aSerialPortPath, TUint aVolumeLimit)
    : iSerialPort(new OhSerialPort(aSerialPortPath, 9600))
    , iCurrentVolume(0)
    , iVolumeLimit(aVolumeLimit)
    , iDialect(EDialect::kLr2)
    , iCmdVolumeSet(kLr2VolumeSet)
    , iCmdVolumeQuery(kLr2VolumeQuery)
    , iCmdVolumeResp(kLr2VolumeSet)
    , iCmdBalanceSet(kLr2BalanceSet)
    , iCmdBassSet(kLr2BassSet)
    , iCmdTrebleSet(kLr2TrebleSet)
    , iDeviceId(64)
    , iDeviceModel(32)
    , iDeviceHwVersion(128)
    , iDeviceSwVersion(64)
{
    if (!iSerialPort->Open()) {
        OhLog::PrintError("OhLinnAmpVolume: failed to open serial port %s\n", aSerialPortPath.c_str());
    } else {
        OhLog::Print("OhLinnAmpVolume: Connected to %s\n", aSerialPortPath.c_str());
        QueryId();
        DetectDialect();
        QueryVersions();
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
    return iSerialPort->IsOpen() && iDeviceModel.Bytes() > 0;
}

const Brx& OhLinnAmpVolume::DeviceId() const {
    return iDeviceId;
}

const Brx& OhLinnAmpVolume::DeviceModel() const {
    return iDeviceModel;
}

const Brx& OhLinnAmpVolume::DeviceHwVersion() const {
    return iDeviceHwVersion;
}

const Brx& OhLinnAmpVolume::DeviceSwVersion() const {
    return iDeviceSwVersion;
}

void OhLinnAmpVolume::SetVolume(TUint aVolume) {
    if (!IsVolumeSupported()) {
        OhLog::PrintWarning("OhLinnAmpVolume: serial port not open\n");
        return;
    }

    TUint linnVolume = aVolume / kMilliDbPerStep;
    if (linnVolume > iVolumeLimit) {
        linnVolume = iVolumeLimit;
    }

    std::string command = BuildCommand(iCmdVolumeSet, static_cast<int>(linnVolume));
    Brn cmd(reinterpret_cast<const TByte*>(command.c_str()), command.size());
    Bws<32> response;
    if (SendCommandAndWaitResponse(cmd, response)) {
        iCurrentVolume = linnVolume;
        OhLog::PrintDebug("OhLinnAmpVolume: Set volume to %u (Linn: %u)\n", aVolume, linnVolume);
    } else {
        OhLog::PrintError("OhLinnAmpVolume: failed to send volume command\n");
    }
}

void OhLinnAmpVolume::SetBalance(TInt aBalance) {
    if (!IsVolumeSupported()) {
        return;
    }

    std::string command = BuildCommand(iCmdBalanceSet, aBalance);
    Brn cmd(reinterpret_cast<const TByte*>(command.c_str()), command.size());
    Bws<32> response;
    if (!SendCommandAndWaitResponse(cmd, response)) {
        OhLog::PrintError("OhLinnAmpVolume: failed to send balance command\n");
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

TBool OhLinnAmpVolume::ProbeCommand(const char* aCommand, Bwx& aResponse) {
    if (!iSerialPort->IsOpen()) {
        return false;
    }
    Brn cmd(reinterpret_cast<const TByte*>(aCommand), strlen(aCommand));
    return iSerialPort->SendCommandAndWaitResponse(cmd, aResponse, kResponseTimeoutMs);
}

void OhLinnAmpVolume::SetDialect(EDialect aDialect) {
    iDialect = aDialect;
    if (aDialect == EDialect::kMajik) {
        iCmdVolumeSet = kMajikVolumeSet;
        iCmdVolumeQuery = kMajikVolumeQuery;
        iCmdVolumeResp = kMajikVolumeResp;
        iCmdBalanceSet = kMajikBalanceSet;
        iCmdBassSet = nullptr;
        iCmdTrebleSet = nullptr;
    } else {
        iCmdVolumeSet = kLr2VolumeSet;
        iCmdVolumeQuery = kLr2VolumeQuery;
        iCmdVolumeResp = kLr2VolumeSet;
        iCmdBalanceSet = kLr2BalanceSet;
        iCmdBassSet = kLr2BassSet;
        iCmdTrebleSet = kLr2TrebleSet;
    }
}

void OhLinnAmpVolume::QueryId() {
    if (!iSerialPort->IsOpen()) {
        return;
    }

    Brn cmd(reinterpret_cast<const TByte*>(kCmdIdQuery), strlen(kCmdIdQuery));
    Bws<64> response;
    if (SendCommandAndWaitResponse(cmd, response)) {
        std::string respStr = ToString(response);
        // LR2 preamplifiers: !$ID LR2nnnnnnnnnnnn$ (model code + 12-digit hex serial)
        // Majik amplifiers: !$ID MAJIK_I$ (model name as product identifier)
        std::string id = LinnAmpProtocol::ParsePrefixedValue(respStr, "ID ");
        if (!id.empty()) {
            iIdValue = id;
            std::string model, serial;
            LinnAmpProtocol::SplitIdValue(id, model, serial);
            iDeviceId.Replace(Brn(serial.c_str()));
            iDeviceModel.Replace(Brn(model.c_str()));
            if (serial.empty()) {
                OhLog::Print("OhLinnAmpVolume: Linn %s detected\n", model.c_str());
            } else {
                OhLog::Print("OhLinnAmpVolume: Linn %s detected (serial %s)\n", model.c_str(), serial.c_str());
            }
        } else {
            OhLog::Print("OhLinnAmpVolume: Device ID: %s\n", respStr.c_str());
        }
    } else {
        OhLog::PrintError("OhLinnAmpVolume: failed to query device ID\n");
    }
}

void OhLinnAmpVolume::DetectDialect() {
    if (!iSerialPort->IsOpen()) {
        return;
    }

    Bws<32> response;
    if (ProbeCommand(kMajikVolumeQuery, response)
        && ToString(response).find("VOLUME") != std::string::npos) {
        SetDialect(EDialect::kMajik);
        OhLog::Print("OhLinnAmpVolume: Majik command set detected (VOLUME)\n");
        return;
    }
    if (ProbeCommand(kLr2VolumeQuery, response)
        && ToString(response).find("VOL") != std::string::npos) {
        SetDialect(EDialect::kLr2);
        OhLog::Print("OhLinnAmpVolume: LR2 command set detected (VOL)\n");
        return;
    }
    SetDialect(LinnAmpProtocol::IsLr2IdValue(iIdValue) ? EDialect::kLr2 : EDialect::kMajik);
    OhLog::PrintWarning("OhLinnAmpVolume: no volume query answered, assuming %s command set from ID\n",
                        iDialect == EDialect::kLr2 ? "LR2" : "Majik");
}

void OhLinnAmpVolume::QueryVersions() {
    if (!iSerialPort->IsOpen()) {
        return;
    }

    Bws<128> response;
    if (ProbeCommand(kCmdVersionHwQuery, response)) {
        std::string version = LinnAmpProtocol::ParsePrefixedValue(ToString(response), kRespVersionHw);
        if (!version.empty()) {
            iDeviceHwVersion.Replace(Brn(version.c_str()));
            OhLog::Print("OhLinnAmpVolume: HW version: %s\n", version.c_str());
        }
    }
    if (ProbeCommand(kCmdVersionSwQuery, response)) {
        std::string version = LinnAmpProtocol::ParsePrefixedValue(ToString(response), kRespVersionSw);
        if (!version.empty()) {
            iDeviceSwVersion.Replace(Brn(version.c_str()));
            OhLog::Print("OhLinnAmpVolume: SW version: %s\n", version.c_str());
        }
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
        OhLog::Print("OhLinnAmpVolume: Standby %s\n", aOn ? "ON" : "OFF");
    } else {
        OhLog::PrintError("OhLinnAmpVolume: failed to set standby %s\n", aOn ? "ON" : "OFF");
    }
}

void OhLinnAmpVolume::SetTone(const char* aPrefix, TInt aValue) {
    if (!IsVolumeSupported()) {
        return;
    }
    if (aPrefix == nullptr) {
        OhLog::PrintDebug("OhLinnAmpVolume: tone control not supported by this amplifier\n");
        return;
    }

    std::string command = BuildCommand(aPrefix, static_cast<int>(aValue));
    Brn cmd(reinterpret_cast<const TByte*>(command.c_str()), command.size());
    Bws<32> response;
    if (SendCommandAndWaitResponse(cmd, response)) {
        OhLog::Print("OhLinnAmpVolume: Set %s to %d\n", aPrefix, aValue);
    } else {
        OhLog::PrintError("OhLinnAmpVolume: failed to set %s\n", aPrefix);
    }
}

void OhLinnAmpVolume::SetBass(TInt aBass) {
    SetTone(iCmdBassSet, aBass);
}

void OhLinnAmpVolume::SetTreble(TInt aTreble) {
    SetTone(iCmdTrebleSet, aTreble);
}

void OhLinnAmpVolume::SelectInput(int aSource) {
    if (!IsVolumeSupported()) {
        return;
    }

    std::ostringstream oss;
    if (iDialect == EDialect::kMajik) {
        oss << "INPUT AUDIO INPUT" << aSource;
    } else {
        oss << kCmdListen << aSource;
    }
    std::string command = oss.str();
    Brn cmd(reinterpret_cast<const TByte*>(command.c_str()), command.size());
    Bws<32> response;
    if (SendCommandAndWaitResponse(cmd, response)) {
        OhLog::Print("OhLinnAmpVolume: Selected input %d\n", aSource);
    } else {
        OhLog::PrintError("OhLinnAmpVolume: failed to select input %d\n", aSource);
    }
}

TBool OhLinnAmpVolume::QueryCurrentVolume() {
    if (!IsVolumeSupported()) {
        return false;
    }

    Brn cmd(reinterpret_cast<const TByte*>(iCmdVolumeQuery), strlen(iCmdVolumeQuery));
    Bws<32> response;
    if (SendCommandAndWaitResponse(cmd, response)) {
        std::string respStr = ToString(response);
        size_t volumePos = respStr.find(iCmdVolumeResp);
        if (volumePos != std::string::npos) {
            try {
                iCurrentVolume = std::stoi(respStr.substr(volumePos + strlen(iCmdVolumeResp)));
                OhLog::Print("OhLinnAmpVolume: Current volume: %u\n", iCurrentVolume);
                return true;
            } catch (...) {
                OhLog::PrintError("OhLinnAmpVolume: invalid volume response\n");
            }
        }
    }
    return false;
}

} // namespace Av
} // namespace OpenHome
