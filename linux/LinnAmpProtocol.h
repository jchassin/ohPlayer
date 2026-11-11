#pragma once

#include <cctype>
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

} // namespace LinnAmpProtocol
} // namespace Av
} // namespace OpenHome
