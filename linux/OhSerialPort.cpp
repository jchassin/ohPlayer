#include "OhSerialPort.h"
#include <OpenHome/Private/Printer.h>
#include <OpenHome/Private/Thread.h>
#include <sys/ioctl.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <ctime>
#include <cstdio>
#include <string>

namespace OpenHome {
namespace Av {

static TUint NowMs() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<TUint>(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

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

TBool OhSerialPort::SendCommandAndWaitResponse(const Brx& aCommand, Bwx& aResponse, TUint aTimeoutMs) {
    if (!iIsOpen) {
        Log::Print("OhSerialPort: Port not open for SendCommandAndWaitResponse\n");
        return false;
    }

    // Flush input buffer by reading until CRLF or timeout
    FlushInputBuffer();

    // Send command with $ prefix and $\r suffix
    Bws<256> commandWithPrefix;
    commandWithPrefix.Append("$");
    commandWithPrefix.Append(aCommand);
    commandWithPrefix.Append("\r");

    {
        std::string dbg(reinterpret_cast<const char*>(commandWithPrefix.Ptr()), commandWithPrefix.Bytes());
        Log::Print("OhSerialPort: Sending command: '%s' (%u bytes)\n", dbg.c_str(), commandWithPrefix.Bytes());
    }

    if (!Write(Brn(commandWithPrefix.Ptr(), commandWithPrefix.Bytes()))) {
        Log::Print("OhSerialPort: Failed to write command\n");
        return false;
    }

    // Wait for response with timeout
    // Response format: !COMMAND result\r\n
    TUint startTime = NowMs();
    Bws<256> responseBuffer;
    TBool foundEnd = false;

    while (NowMs() - startTime < aTimeoutMs) {
        unsigned char buf[16];
        ssize_t bytesRead = read(iFd, buf, sizeof(buf));
        
        if (bytesRead > 0) {
            std::string hex;
            char tmp[8];
            for (ssize_t i = 0; i < bytesRead; i++) {
                snprintf(tmp, sizeof(tmp), "%02X ", buf[i]);
                hex += tmp;
            }
            Log::Print("OhSerialPort: Read %d bytes: %s\n", static_cast<int>(bytesRead), hex.c_str());
            for (ssize_t i = 0; i < bytesRead; i++) {
                if (buf[i] == '\r' || buf[i] == '\n') {
                    // CR or LF terminates the response (accept CR-only, LF-only or CRLF)
                    foundEnd = true;
                    break;
                }
                responseBuffer.Append(buf[i]);
            }
            
            if (foundEnd) {
                break;
            }
        } else if (bytesRead == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
            Log::Print("OhSerialPort: Read error: %s\n", strerror(errno));
            return false;
        }
        
        // Small delay to avoid busy waiting
        usleep(10000);
    }

    if (!foundEnd) {
        Log::Print("OhSerialPort: Timeout waiting for response (received %u bytes)\n", responseBuffer.Bytes());
        return false;
    }

    if (responseBuffer.Bytes() > 0) {
        std::string dbg(reinterpret_cast<const char*>(responseBuffer.Ptr()), responseBuffer.Bytes());
        Log::Print("OhSerialPort: Response: '%s'\n", dbg.c_str());
    } else {
        Log::Print("OhSerialPort: Response: (empty)\n");
    }

    // Remove trailing \r if present
    if (responseBuffer.Bytes() > 0 && responseBuffer[responseBuffer.Bytes() - 1] == '\r') {
        responseBuffer.SetBytes(responseBuffer.Bytes() - 1);
    }
    
    aResponse.Replace(Brn(responseBuffer.Ptr(), responseBuffer.Bytes()));
    return true;
}

void OhSerialPort::FlushInputBuffer() {
    if (!iIsOpen) {
        return;
    }

    // Read and discard all data until CRLF or timeout
    unsigned char buf[64];
    TUint startTime = NowMs();
    TBool foundCrlf = false;

    while (NowMs() - startTime < 100) { // 100ms timeout for flush
        ssize_t bytesRead = read(iFd, buf, sizeof(buf));
        
        if (bytesRead > 0) {
            // Check for CRLF in the buffer
            for (ssize_t i = 0; i < bytesRead; i++) {
                if (buf[i] == '\n' && i > 0 && buf[i-1] == '\r') {
                    foundCrlf = true;
                    break;
                }
            }
            
            if (foundCrlf) {
                break;
            }
        } else if (bytesRead == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
            break;
        }
        
        usleep(10000);
    }
}

} // namespace Av
} // namespace OpenHome
