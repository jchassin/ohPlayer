#include "PrefixedFriendlyName.h"
#include <OpenHome/Private/Printer.h>

namespace OpenHome {
namespace Av {

PrefixedFriendlyNameObservable::PrefixedFriendlyNameObservable(IFriendlyNameObservable& aObservable,
                                                               const Brx& aPrefix)
    : iObservable(aObservable)
    , iPrefix(aPrefix)
    , iUpstreamId(kIdInvalid)
    , iNextObserverId(kIdInvalid + 1)
    , iLock("PFNO")
{
    // Functor is called within this call (with the current name).
    iUpstreamId = iObservable.RegisterFriendlyNameObserver(
        MakeFunctorGeneric<const Brx&>(*this, &PrefixedFriendlyNameObservable::FriendlyNameChanged));
}

PrefixedFriendlyNameObservable::~PrefixedFriendlyNameObservable()
{
    iObservable.DeregisterFriendlyNameObserver(iUpstreamId);
}

TUint PrefixedFriendlyNameObservable::RegisterFriendlyNameObserver(FunctorGeneric<const Brx&> aObserver)
{
    AutoMutex a(iLock);
    const TUint id = iNextObserverId++;
    iObservers.insert(std::pair<TUint, FunctorGeneric<const Brx&>>(id, aObserver));
    // Match IFriendlyNameObservable semantics: notify within this call.
    aObserver(iName);
    return id;
}

void PrefixedFriendlyNameObservable::DeregisterFriendlyNameObserver(TUint aId)
{
    AutoMutex a(iLock);
    iObservers.erase(aId);
}

void PrefixedFriendlyNameObservable::FriendlyNameChanged(const Brx& aName)
{
    AutoMutex a(iLock);
    iName.Replace(iPrefix);
    iName.Append(' ');
    iName.Append(aName);
    NotifyLocked(iName);
}

void PrefixedFriendlyNameObservable::NotifyLocked(const Brx& aName)
{
    for (auto it = iObservers.begin(); it != iObservers.end(); ++it) {
        it->second(aName);
    }
}

} // namespace Av
} // namespace OpenHome
