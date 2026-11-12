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
const int kDefaultListenSource = 1;
const char* const kCmdStandbyOn = "STANDBY ON";
const char* const kCmdStandbyOff = "STANDBY OFF";
const char* const kCmdVersionHwQuery = "VERSION HARDWARE ?";
const char* const kCmdVersionSwQuery = "VERSION SOFTWARE ?";
const char* const kRespVersionHw = "VERSION HARDWARE ";
const char* const kRespVersionSw = "VERSION SOFTWARE ";
// Some Linn amplifiers (e.g. Majik-I) answer slowly, in particular power
// commands such as STANDBY (several seconds while entering/leaving standby);
// the final response line is detected as soon as it arrives, so this timeout
// only bounds genuinely silent links.
const OpenHome::TUint kResponseTimeoutMs = 3000;
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
    , iStandbyOn(false)
    , iProtocol(std::make_unique<LinnAmpProtocol::Lr2Protocol>())
    , iDeviceId(64)
    , iDeviceModel(32)
    , iDeviceHwVersion(192)
    , iDeviceSwVersion(64)
{
    if (!iSerialPort->Open()) {
        OhLog::PrintError("OhLinnAmpVolume: failed to open serial port %s\n", aSerialPortPath.c_str());
    } else {
        OhLog::Print("OhLinnAmpVolume: Connected to %s\n", aSerialPortPath.c_str());
        QueryId();
        DetectDialect();
        QueryVersions();
        // Query volume and select the input while the amp is still awake
        // (a Majik in standby does not answer queries), then align the amp
        // with the OpenHome product initial state (standby at boot)
        QueryCurrentVolume();
        SelectInput(kDefaultListenSource);
        SetStandby(true);
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

    std::string command = BuildCommand(iProtocol->VolumeSetPrefix(), static_cast<int>(linnVolume));
    Brn cmd(reinterpret_cast<const TByte*>(command.c_str()), command.size());
    // Fire-and-forget: don't block on the amp's answer, ohPipeline ramps the
    // volume with a burst of SetVolume calls and a slow amp would make the
    // slider lag. The response (if any) is drained by the next command.
    if (iSerialPort->SendCommandNoWait(cmd)) {
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

    std::string command = BuildCommand(iProtocol->BalanceSetPrefix(), aBalance);
    Brn cmd(reinterpret_cast<const TByte*>(command.c_str()), command.size());
    if (!iSerialPort->SendCommandNoWait(cmd)) {
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
    // Some Linn amplifiers (e.g. Majik-I) take over a second to answer, in
    // particular power commands while entering/leaving standby: allow a few
    // attempts before giving up.
    for (int attempt = 0; attempt < 3; attempt++) {
        if (iSerialPort->SendCommandAndWaitResponse(aCommand, aResponse, kResponseTimeoutMs)) {
            return true;
        }
        usleep(100000);
    }
    return false;
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

    LinnAmpProtocol::ProbeFn probe = [this](const std::string& aCommand, std::string& aResponse) {
        Brn cmd(reinterpret_cast<const TByte*>(aCommand.c_str()), aCommand.size());
        Bws<32> response;
        if (!iSerialPort->SendCommandAndWaitResponse(cmd, response, kResponseTimeoutMs)) {
            return false;
        }
        aResponse = ToString(response);
        return true;
    };
    iProtocol = LinnAmpProtocol::DetectProtocol(iIdValue, probe);
    OhLog::Print("OhLinnAmpVolume: %s command set detected\n", iProtocol->Name());
}

void OhLinnAmpVolume::QueryVersions() {
    if (!iSerialPort->IsOpen()) {
        return;
    }

    QueryVersion(kCmdVersionHwQuery, kRespVersionHw, iDeviceHwVersion, "HW");
    QueryVersion(kCmdVersionSwQuery, kRespVersionSw, iDeviceSwVersion, "SW");
}

void OhLinnAmpVolume::QueryVersion(const char* aQuery, const char* aRespPrefix, Bwh& aOut, const char* aLabel) {
    Brn cmd(reinterpret_cast<const TByte*>(aQuery), strlen(aQuery));
    std::vector<std::string> lines;
    // Some amplifiers answer a VERSION query with several final lines (e.g.
    // Majik-I $VERSION HARDWARE ?$ returns one line per board): join them all.
    if (!iSerialPort->SendCommandAndReadAllLines(cmd, lines, kResponseTimeoutMs)) {
        return;
    }
    std::string version;
    for (const auto& line : lines) {
        std::string v = LinnAmpProtocol::ParsePrefixedValue(line, aRespPrefix);
        if (!v.empty()) {
            if (!version.empty()) {
                version += "; ";
            }
            version += v;
        }
    }
    if (!version.empty()) {
        aOut.Replace(Brn(version.c_str()));
        OhLog::Print("OhLinnAmpVolume: %s version: %s\n", aLabel, version.c_str());
    }
}

void OhLinnAmpVolume::SetStandby(TBool aOn) {
    if (!IsVolumeSupported()) {
        return;
    }
    // Idempotent: a Majik in standby answers very slowly, so avoid resending
    // STANDBY ON when the amp is already in the requested state (e.g. the
    // renderer's StandbyEnabled right after the boot-time standby).
    if (aOn == iStandbyOn) {
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
    // The amp processes the command even if its answer is lost or very slow
    // (standby), so track the requested state either way.
    iStandbyOn = aOn;
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
    if (iProtocol->SupportsTone()) {
        SetTone(iProtocol->BassSetPrefix(), aBass);
    } else {
        OhLog::PrintDebug("OhLinnAmpVolume: tone control not supported by this amplifier\n");
    }
}

void OhLinnAmpVolume::SetTreble(TInt aTreble) {
    if (iProtocol->SupportsTone()) {
        SetTone(iProtocol->TrebleSetPrefix(), aTreble);
    } else {
        OhLog::PrintDebug("OhLinnAmpVolume: tone control not supported by this amplifier\n");
    }
}

void OhLinnAmpVolume::SelectInput(int aSource) {
    if (!IsVolumeSupported()) {
        return;
    }

    std::string command = iProtocol->InputCommand(aSource);
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

    const char* volumeQuery = iProtocol->VolumeQuery();
    const char* volumeResp = iProtocol->VolumeResponseKeyword();
    Brn cmd(reinterpret_cast<const TByte*>(volumeQuery), strlen(volumeQuery));
    Bws<32> response;
    if (SendCommandAndWaitResponse(cmd, response)) {
        std::string respStr = ToString(response);
        size_t volumePos = respStr.find(volumeResp);
        if (volumePos != std::string::npos) {
            try {
                iCurrentVolume = std::stoi(respStr.substr(volumePos + strlen(volumeResp)));
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
