#include "StoreWithOverriddenValue.h"
#include <OpenHome/Private/Stream.h>

namespace OpenHome {
namespace Av {

StoreWithOverriddenValue::StoreWithOverriddenValue(Configuration::IStoreReadWrite& aStore,
                                                   const Brx& aKey, const Brx& aValue)
    : iStore(aStore)
    , iKey(aKey)
    , iValue(aValue)
{
}

StoreWithOverriddenValue::~StoreWithOverriddenValue()
{
}

TBool StoreWithOverriddenValue::KeyMatches(const Brx& aKey) const
{
    return (aKey == iKey);
}

void StoreWithOverriddenValue::Read(const Brx& aKey, Bwx& aDest)
{
    if (KeyMatches(aKey)) {
        if (aDest.MaxBytes() < iValue.Bytes()) {
            THROW(StoreReadBufferUndersized);
        }
        aDest.Replace(iValue);
        return;
    }
    iStore.Read(aKey, aDest);
}

void StoreWithOverriddenValue::Read(const Brx& aKey, IWriter& aWriter)
{
    if (KeyMatches(aKey)) {
        aWriter.Write(iValue);
        return;
    }
    iStore.Read(aKey, aWriter);
}

void StoreWithOverriddenValue::Write(const Brx& aKey, const Brx& aSource)
{
    // Writes always go to the real store, including for the overridden key:
    // changes made at runtime (e.g. from the web ui) must persist.
    iStore.Write(aKey, aSource);
}

void StoreWithOverriddenValue::Delete(const Brx& aKey)
{
    iStore.Delete(aKey);
}

void StoreWithOverriddenValue::ResetToDefaults()
{
    iStore.ResetToDefaults();
}

} // namespace Av
} // namespace OpenHome
