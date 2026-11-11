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
    OhLog::Print("OhSerialPort: Opened %s at %u baud\n", iDevicePath.c_str(), iBaudRate);
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

    if (!WriteFramedCommandLocked(aCommand)) {
        return false;
    }

    // Wait for the final response with timeout.
    // Protocol (LR2/Majik spec section 1.4.2): an initial response '!' CRLF is
    // issued on receipt of a valid command, followed by the final response
    // '!$RESULT$' CRLF. Some products answer with the final line only, and an
    // invalid command gets '!$FAIL sc fn$' with no final response: treat any
    // line starting with '!' (longer than the bare initial '!') as the final
    // response, and fail immediately on '!$FAIL'. Any other line (e.g. an
    // unsolicited '$...$' event) is ignored.
    TUint startTime = NowMs();
    std::string line;
    std::string finalLine;
    TBool foundFinal = false;

    while (NowMs() - startTime < aTimeoutMs) {
        unsigned char buf[16];
        ssize_t bytesRead = read(iFd, buf, sizeof(buf));

        if (bytesRead > 0) {
            for (ssize_t i = 0; i < bytesRead && !foundFinal; i++) {
                if (buf[i] == '\r' || buf[i] == '\n') {
                    if (!line.empty()) {
                        if (line.size() > 1 && line[0] == '!') {
                            finalLine = line;
                            foundFinal = true;
                        }
                        line.clear();
                    }
                } else {
                    line += static_cast<char>(buf[i]);
                }
            }
        } else if (bytesRead == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
            OhLog::PrintError("OhSerialPort: read error: %s\n", strerror(errno));
            return false;
        }

        // Small delay to avoid busy waiting
        usleep(10000);
    }

    if (!foundFinal) {
        OhLog::PrintWarning("OhSerialPort: timeout waiting for response\n");
        return false;
    }
    if (finalLine.compare(0, 6, "!$FAIL") == 0) {
        OhLog::PrintWarning("OhSerialPort: command rejected: %s\n", finalLine.c_str());
        return false;
    }

    aResponse.Replace(Brn(reinterpret_cast<const TByte*>(finalLine.c_str()), finalLine.size()));
    return true;
}

TBool OhSerialPort::WriteFramedCommandLocked(const Brx& aCommand) {
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
    return true;
}

TBool OhSerialPort::SendCommandAndReadAllLines(const Brx& aCommand, std::vector<std::string>& aLines, TUint aTimeoutMs) {
    if (!iIsOpen) {
        OhLog::PrintWarning("OhSerialPort: port not open for SendCommandAndReadAllLines\n");
        return false;
    }

    iMutex.Wait();
    struct MutexGuard {
        ~MutexGuard() { iMutex.Signal(); }
        Mutex& iMutex;
    } guard{iMutex};

    if (!WriteFramedCommandLocked(aCommand)) {
        return false;
    }

    // Collect every final '!$...$' line until the line has been quiet for a
    // short period (or the overall timeout expires). A bare '!' initial
    // response and unsolicited '$...$' lines are ignored.
    const TUint kQuietMs = 60;
    TUint startTime = NowMs();
    TUint lastDataTime = startTime;
    std::string line;

    while (NowMs() - startTime < aTimeoutMs) {
        unsigned char buf[16];
        ssize_t bytesRead = read(iFd, buf, sizeof(buf));

        if (bytesRead > 0) {
            lastDataTime = NowMs();
            for (ssize_t i = 0; i < bytesRead; i++) {
                if (buf[i] == '\r' || buf[i] == '\n') {
                    if (!line.empty()) {
                        if (line.size() > 1 && line[0] == '!') {
                            aLines.push_back(line);
                        }
                        line.clear();
                    }
                } else {
                    line += static_cast<char>(buf[i]);
                }
            }
        } else if (bytesRead == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
            OhLog::PrintError("OhSerialPort: read error: %s\n", strerror(errno));
            return false;
        }

        if (!aLines.empty() && NowMs() - lastDataTime >= kQuietMs) {
            break;
        }
        usleep(5000);
    }

    if (aLines.empty()) {
        OhLog::PrintWarning("OhSerialPort: timeout waiting for response\n");
        return false;
    }
    for (const auto& l : aLines) {
        if (l.compare(0, 6, "!$FAIL") == 0) {
            OhLog::PrintWarning("OhSerialPort: command rejected: %s\n", l.c_str());
            return false;
        }
    }
    return true;
}

void OhSerialPort::FlushInputBuffer() {
    if (!iIsOpen) {
        return;
    }

    // Drain stray input (late or unsolicited responses) until the line has
    // been quiet for a short period, not just until the first CRLF: a late
    // '!$...$' final response must not leak into the next command's reply.
    unsigned char buf[64];
    const TUint kMaxFlushMs = 150;
    const TUint kQuietMs = 30;
    TUint startTime = NowMs();
    TUint lastDataTime = startTime;

    while (NowMs() - startTime < kMaxFlushMs) {
        ssize_t bytesRead = read(iFd, buf, sizeof(buf));
        if (bytesRead > 0) {
            lastDataTime = NowMs();
        } else if (bytesRead == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
            break;
        }
        if (NowMs() - lastDataTime >= kQuietMs) {
            break;
        }
        usleep(5000);
    }
}

} // namespace Av
} // namespace OpenHome
