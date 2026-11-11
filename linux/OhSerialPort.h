#pragma once

#include <OpenHome/Types.h>
#include <OpenHome/Buffer.h>
#include <OpenHome/Private/Thread.h>
#include <string>
#include <vector>
#include <termios.h>

namespace OpenHome {
namespace Av {

class OhSerialPort {
public:
    OhSerialPort(const std::string& aDevicePath, TUint aBaudRate = 9600);
    ~OhSerialPort();

    TBool Open();
    void Close();
    TBool IsOpen() const;

    TBool Write(const Brx& aData, TUint aTimeoutMs = 1000);
    TBool Read(Bwx& aBuffer, TUint aTimeoutMs = 1000);
    TBool SendCommandAndWaitResponse(const Brx& aCommand, Bwx& aResponse, TUint aTimeoutMs = 1000);
    // Like SendCommandAndWaitResponse but collects every final '!$...$' line
    // until the line goes quiet: some products answer a query with several
    // final lines (e.g. Majik-I $VERSION HARDWARE ?$ returns one per board).
    TBool SendCommandAndReadAllLines(const Brx& aCommand, std::vector<std::string>& aLines, TUint aTimeoutMs = 1000);

    void SetBaudRate(TUint aBaudRate);

private:
    speed_t GetBaudRateFlag(TUint aBaudRate);
    void FlushInputBuffer();
    // Caller must hold iMutex
    TBool WriteFramedCommandLocked(const Brx& aCommand);

private:
    int iFd;
    std::string iDevicePath;
    TUint iBaudRate;
    struct termios iOldTermios;
    TBool iIsOpen;
    Mutex iMutex;
};

} // namespace Av
} // namespace OpenHome
