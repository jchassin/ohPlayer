#pragma once

#include <OpenHome/Types.h>
#include <OpenHome/Buffer.h>
#include <string>

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

    void SetBaudRate(TUint aBaudRate);

private:
    speed_t GetBaudRateFlag(TUint aBaudRate);

private:
    int iFd;
    std::string iDevicePath;
    TUint iBaudRate;
    struct termios iOldTermios;
    TBool iIsOpen;
};

} // namespace Av
} // namespace OpenHome
