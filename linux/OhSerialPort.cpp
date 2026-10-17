#include "OhSerialPort.h"
#include <OpenHome/Private/Printer.h>
#include <sys/ioctl.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>

namespace OpenHome {
namespace Av {

OhSerialPort::OhSerialPort(const std::string& aDevicePath, TUint aBaudRate)
    : iFd(-1)
    , iDevicePath(aDevicePath)
    , iBaudRate(aBaudRate)
    , iIsOpen(false)
{
}

OhSerialPort::~OhSerialPort() {
    Close();
}

TBool OhSerialPort::Open() {
    if (iIsOpen) {
        return true;
    }

    iFd = open(iDevicePath.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
    if (iFd == -1) {
        Log::Print("OhSerialPort: Failed to open %s: %s\n", iDevicePath.c_str(), strerror(errno));
        return false;
    }

    if (tcgetattr(iFd, &iOldTermios) != 0) {
        Close();
        return false;
    }

    struct termios newTermios;
    memset(&newTermios, 0, sizeof(newTermios));
    newTermios.c_cflag = CREAD | CLOCAL | CS8;
    newTermios.c_iflag = 0;
    newTermios.c_oflag = 0;
    newTermios.c_lflag = 0;

    cfsetispeed(&newTermios, GetBaudRateFlag(iBaudRate));
    cfsetospeed(&newTermios, GetBaudRateFlag(iBaudRate));

    newTermios.c_cc[VMIN] = 0;
    newTermios.c_cc[VTIME] = 10;

    if (tcsetattr(iFd, TCSANOW, &newTermios) != 0) {
        Close();
        return false;
    }

    iIsOpen = true;
    Log::Print("OhSerialPort: Opened %s at %u baud\n", iDevicePath.c_str(), iBaudRate);
    return true;
}

void OhSerialPort::Close() {
    if (iFd != -1) {
        tcsetattr(iFd, TCSANOW, &iOldTermios);
        close(iFd);
        iFd = -1;
        iIsOpen = false;
    }
}

TBool OhSerialPort::IsOpen() const {
    return iIsOpen;
}

speed_t OhSerialPort::GetBaudRateFlag(TUint aBaudRate) {
    switch (aBaudRate) {
        case 9600:   return B9600;
        case 19200:  return B19200;
        case 38400:  return B38400;
        case 57600:  return B57600;
        case 115200: return B115200;
        default:     return B9600;
    }
}

void OhSerialPort::SetBaudRate(TUint aBaudRate) {
    if (iBaudRate == aBaudRate) {
        return;
    }
    iBaudRate = aBaudRate;
    if (iIsOpen) {
        struct termios newTermios;
        if (tcgetattr(iFd, &newTermios) == 0) {
            cfsetispeed(&newTermios, GetBaudRateFlag(iBaudRate));
            cfsetospeed(&newTermios, GetBaudRateFlag(iBaudRate));
            tcsetattr(iFd, TCSANOW, &newTermios);
        }
    }
}

TBool OhSerialPort::Write(const Brx& aData, TUint aTimeoutMs) {
    if (!iIsOpen) {
        return false;
    }

    ssize_t written = write(iFd, aData.Ptr(), aData.Bytes());
    if (written == -1) {
        Log::Print("OhSerialPort: Write failed: %s\n", strerror(errno));
        return false;
    }
    return (written == (ssize_t)aData.Bytes());
}

TBool OhSerialPort::Read(Bwx& aBuffer, TUint aTimeoutMs) {
    if (!iIsOpen) {
        return false;
    }

    unsigned char buf[256];
    ssize_t bytesRead = read(iFd, buf, sizeof(buf) - 1);
    if (bytesRead > 0) {
        aBuffer.Replace(Brn(buf, static_cast<TUint>(bytesRead)));
        return true;
    }
    return false;
}

} // namespace Av
} // namespace OpenHome
