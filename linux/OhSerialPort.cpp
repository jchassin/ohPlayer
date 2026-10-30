#include "OhSerialPort.h"
#include <OpenHome/Private/Printer.h>
#include "OhLog.h"
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
    , iMutex("OHSP")
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
        OhLog::PrintError("OhSerialPort: failed to open %s: %s\n", iDevicePath.c_str(), strerror(errno));
        return false;
    }

    if (tcgetattr(iFd, &iOldTermios) != 0) {
        Close();
        return false;
    }

    struct termios newTermios;
    memset(&newTermios, 0, sizeof(newTermios));
    // LR2 RS232 spec (Appendix C): 7 data bits, 1 stop bit, even parity
    newTermios.c_cflag = CREAD | CLOCAL | CS7 | PARENB;
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
        OhLog::PrintError("OhSerialPort: write failed: %s\n", strerror(errno));
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
        OhLog::PrintWarning("OhSerialPort: port not open for SendCommandAndWaitResponse\n");
        return false;
    }

    // Serialize command/response exchanges: flush, write and read must be atomic
    iMutex.Wait();
    struct MutexGuard {
        ~MutexGuard() { iMutex.Signal(); }
        Mutex& iMutex;
    } guard{iMutex};

    // Flush input buffer by reading until CRLF or timeout
    FlushInputBuffer();

    // Send command framed as $COMMAND$\r\n
    Bws<256> commandWithPrefix;
    commandWithPrefix.Append("$");
    commandWithPrefix.Append(aCommand);
    commandWithPrefix.Append("$");
    commandWithPrefix.Append("\r\n");

    if (!Write(Brn(commandWithPrefix.Ptr(), commandWithPrefix.Bytes()))) {
        OhLog::PrintError("OhSerialPort: failed to write command\n");
        return false;
    }

    // Wait for responses with timeout
    // LR2 protocol (spec section 1.4.2): an initial response '!' CRLF is issued
    // on receipt of a valid command, followed by the final response '!$RESULT$' CRLF
    TUint startTime = NowMs();
    Bws<256> responseBuffer;
    TBool foundEnd = false;
    TUint linesSeen = 0;

    while (NowMs() - startTime < aTimeoutMs) {
        unsigned char buf[16];
        ssize_t bytesRead = read(iFd, buf, sizeof(buf));
        
        if (bytesRead > 0) {
            for (ssize_t i = 0; i < bytesRead; i++) {
                if (buf[i] == '\r' || buf[i] == '\n') {
                    // End of a response line
                    if (responseBuffer.Bytes() > 0) {
                        linesSeen++;
                        // First non-empty line is the initial response '!',
                        // the second is the final response '!$RESULT$'
                        if (linesSeen >= 2) {
                            foundEnd = true;
                            break;
                        }
                        responseBuffer.SetBytes(0);
                    }
                } else {
                    responseBuffer.Append(buf[i]);
                }
            }
            
            if (foundEnd) {
                break;
            }
        } else if (bytesRead == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
            OhLog::PrintError("OhSerialPort: read error: %s\n", strerror(errno));
            return false;
        }
        
        // Small delay to avoid busy waiting
        usleep(10000);
    }

    if (!foundEnd) {
        OhLog::PrintWarning("OhSerialPort: timeout waiting for response\n");
        return false;
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
