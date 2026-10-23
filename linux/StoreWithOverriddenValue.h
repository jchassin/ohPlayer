#pragma once

#include <OpenHome/Configuration/IStore.h>

namespace OpenHome {
namespace Av {

// Wraps an IStoreReadWrite, overriding the value returned for a single key.
//
// Used to decouple Volume.Enabled as seen by the web config ui (which reflects
// the real store and the user's intent) from the value the MediaPlayer is
// given at construction time: ohPipeline only creates the Volume provider when
// Volume.Enabled is Yes, but with an external Linn amplifier handling volume
// the provider must exist while the store keeps recording the user's No.
//
// Reads of the overridden key return the injected value; every other read and
// all writes/deletes/visits are forwarded to the wrapped store unchanged, so
// changes made at runtime (e.g. from the web ui) persist as usual.
class StoreWithOverriddenValue : public Configuration::IStoreReadWrite
{
public:
    StoreWithOverriddenValue(Configuration::IStoreReadWrite& aStore,
                             const Brx& aKey, const Brx& aValue);
    ~StoreWithOverriddenValue();
public: // from IStoreReadWrite
    void Read(const Brx& aKey, Bwx& aDest) override;
    void Read(const Brx& aKey, IWriter& aWriter) override;
    void Write(const Brx& aKey, const Brx& aSource) override;
    void Delete(const Brx& aKey) override;
    void ResetToDefaults() override;
private:
    TBool KeyMatches(const Brx& aKey) const;
private:
    Configuration::IStoreReadWrite& iStore;
    const Brn iKey;
    const Brn iValue;
};

} // namespace Av
} // namespace OpenHome
