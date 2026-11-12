#pragma once

#include <cctype>
#include <functional>
#include <memory>
#include <string>

namespace OpenHome {
namespace Av {
namespace LinnAmpProtocol {

inline std::string ParsePrefixedValue(const std::string& aResponse, const std::string& aPrefix) {
    size_t pos = aResponse.find(aPrefix);
    if (pos == std::string::npos) {
        return std::string();
    }
    std::string value = aResponse.substr(pos + aPrefix.length());
    size_t end = value.find('$');
    if (end != std::string::npos) {
        value = value.substr(0, end);
    }
    while (!value.empty() && (value.back() == '\r' || value.back() == '\n' || value.back() == ' ')) {
        value.pop_back();
    }
    return value;
}

inline bool IsLr2IdValue(const std::string& aId) {
    if (aId.length() != 15) {
        return false;
    }
    for (size_t i = 3; i < aId.length(); i++) {
        if (!std::isxdigit(static_cast<unsigned char>(aId[i]))) {
            return false;
        }
    }
    return true;
}

inline void SplitIdValue(const std::string& aId, std::string& aModel, std::string& aSerial) {
    if (IsLr2IdValue(aId)) {
        aModel = aId.substr(0, 3);
        aSerial = aId.substr(3);
    } else {
        aModel = aId;
        aSerial.clear();
    }
}

// Strategy for the command set of one family of Linn amplifiers speaking the
// $...$ RS232 protocol (9600 bauds, 7E1). Supporting a new amp family means
// adding a subclass here - no branching on the amp type anywhere else.
class ILinnAmpProtocol {
public:
    virtual ~ILinnAmpProtocol() = default;

    virtual const char* Name() const = 0;

    // Detection: send ProbeCommand(); the dialect matches when the final
    // response contains ProbeKeyword().
    virtual const char* ProbeCommand() const = 0;
    virtual const char* ProbeKeyword() const = 0;

    virtual const char* VolumeSetPrefix() const = 0;        // e.g. "VOL = " / "VOLUME = "
    virtual const char* VolumeQuery() const = 0;            // e.g. "VOL ?" / "VOLUME ?"
    virtual const char* VolumeResponseKeyword() const = 0;  // keyword in the query's final response
    virtual const char* BalanceSetPrefix() const = 0;       // e.g. "BAL = " / "BALANCE = "
    virtual bool SupportsTone() const = 0;
    virtual const char* BassSetPrefix() const = 0;          // nullptr when unsupported
    virtual const char* TrebleSetPrefix() const = 0;        // nullptr when unsupported
    virtual std::string InputCommand(int aSource) const = 0; // e.g. "LISTEN 1" / "INPUT AUDIO INPUT1"
};

// LR2 preamplifiers (e.g. Akurate Kontrol / RA2): VOL / BAL / BAS / TRB / LISTEN
class Lr2Protocol : public ILinnAmpProtocol {
public:
    const char* Name() const override { return "LR2"; }
    const char* ProbeCommand() const override { return "VOL ?"; }
    const char* ProbeKeyword() const override { return "VOL"; }
    const char* VolumeSetPrefix() const override { return "VOL = "; }
    const char* VolumeQuery() const override { return "VOL ?"; }
    const char* VolumeResponseKeyword() const override { return "VOL = "; }
    const char* BalanceSetPrefix() const override { return "BAL = "; }
    bool SupportsTone() const override { return true; }
    const char* BassSetPrefix() const override { return "BAS = "; }
    const char* TrebleSetPrefix() const override { return "TRB = "; }
    std::string InputCommand(int aSource) const override { return "LISTEN " + std::to_string(aSource); }
};

// Majik integrated amplifiers (e.g. Majik-I): VOLUME / BALANCE / INPUT AUDIO,
// no tone commands
class MajikProtocol : public ILinnAmpProtocol {
public:
    const char* Name() const override { return "Majik"; }
    const char* ProbeCommand() const override { return "VOLUME ?"; }
    const char* ProbeKeyword() const override { return "VOLUME"; }
    const char* VolumeSetPrefix() const override { return "VOLUME = "; }
    const char* VolumeQuery() const override { return "VOLUME ?"; }
    const char* VolumeResponseKeyword() const override { return "VOLUME "; }
    const char* BalanceSetPrefix() const override { return "BALANCE = "; }
    bool SupportsTone() const override { return false; }
    const char* BassSetPrefix() const override { return nullptr; }
    const char* TrebleSetPrefix() const override { return nullptr; }
    std::string InputCommand(int aSource) const override { return "INPUT AUDIO INPUT" + std::to_string(aSource); }
};

// Probe callback: sends aCommand, returns true and fills aResponse when the
// amp answered with a final response line.
using ProbeFn = std::function<bool(const std::string& aCommand, std::string& aResponse)>;

// Detects the command dialect by probing the link (Majik first, then LR2);
// when no probe answers, falls back to the $ID ?$ format (LR2 = 3-char code +
// 12 hex digits serial, anything else is treated as Majik-style model name).
inline std::unique_ptr<ILinnAmpProtocol> DetectProtocol(const std::string& aIdValue, const ProbeFn& aProbe) {
    std::unique_ptr<ILinnAmpProtocol> candidates[] = {
        std::make_unique<MajikProtocol>(),
        std::make_unique<Lr2Protocol>(),
    };
    for (auto& candidate : candidates) {
        std::string response;
        if (aProbe(candidate->ProbeCommand(), response)
            && response.find(candidate->ProbeKeyword()) != std::string::npos) {
            return std::move(candidate);
        }
    }
    if (IsLr2IdValue(aIdValue)) {
        return std::make_unique<Lr2Protocol>();
    }
    return std::make_unique<MajikProtocol>();
}

} // namespace LinnAmpProtocol
} // namespace Av
} // namespace OpenHome
