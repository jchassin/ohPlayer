#include "OhLinnAmpVolume.h"
#include "OhSerialPort.h"
#include <OpenHome/Private/Printer.h>
#include <sstream>

namespace OpenHome {
namespace Av {

OhLinnAmpVolume::OhLinnAmpVolume(const std::string& aSerialPortPath)
    : iSerialPort(new OhSerialPort(aSerialPortPath, 9600))
    , iCurrentVolume(0)
{
    if (!iSerialPort->Open()) {
        Log::Print("OhLinnAmpVolume: Failed to open serial port %s\n", aSerialPortPath.c_str());
    } else {
        Log::Print("OhLinnAmpVolume: Connected to %s\n", aSerialPortPath.c_str());
        QueryCurrentVolume();
    }
}

OhLinnAmpVolume::~OhLinnAmpVolume() {
    delete iSerialPort;
}

TBool OhLinnAmpVolume::IsVolumeSupported() const {
    return iSerialPort->IsOpen();
}

void OhLinnAmpVolume::SetVolume(TUint aVolume) {
    if (!IsVolumeSupported()) {
        Log::Print("OhLinnAmpVolume: Serial port not open\n");
        return;
    }

    TUint linnVolume = (aVolume * 100) / 10000;
    if (linnVolume > 100) {
        linnVolume = 100;
    }

    std::string command = "VOLUME " + std::to_string(linnVolume) + "\r";
    if (SendCommand(command.c_str())) {
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

    std::string command = "BALANCE " + std::to_string(aBalance) + "\r";
    if (!SendCommand(command.c_str())) {
        Log::Print("OhLinnAmpVolume: Failed to send balance command\n");
    }
}

void OhLinnAmpVolume::SetFade(TInt aFade) {
    if (!IsVolumeSupported()) {
        return;
    }

    std::string command = "FADE " + std::to_string(aFade) + "\r";
    if (!SendCommand(command.c_str())) {
        Log::Print("OhLinnAmpVolume: Failed to send fade command\n");
    }
}

TBool OhLinnAmpVolume::SendCommand(const char* aCommand) {
    Brn cmd(reinterpret_cast<const TByte*>(aCommand), strlen(aCommand));
    if (!iSerialPort->Write(cmd)) {
        return false;
    }
    return true;
}

TBool OhLinnAmpVolume::QueryCurrentVolume() {
    if (!IsVolumeSupported()) {
        return false;
    }

    const char* command = "VOLUME?\r";
    if (iSerialPort->Write(Brn(reinterpret_cast<const TByte*>(command), strlen(command)))) {
        Bws<32> response;
        if (iSerialPort->Read(response, 500)) {
            // Convert TByte* to char* for string operations
            std::string respStr(reinterpret_cast<const char*>(response.Ptr()), response.Bytes());
            if (respStr.find("VOLUME ") == 0) {
                try {
                    iCurrentVolume = std::stoi(respStr.substr(7));
                    Log::Print("OhLinnAmpVolume: Current volume: %u\n", iCurrentVolume);
                    return true;
                } catch (...) {
                    Log::Print("OhLinnAmpVolume: Invalid volume response\n");
                }
            }
        }
    }
    return false;
}

} // namespace Av
} // namespace OpenHome
