#pragma once

#include <OpenHome/Types.h>
#include <OpenHome/Buffer.h>
#include <OpenHome/Av/Product.h>
#include <OpenHome/Functor.h>
#include <OpenHome/Private/Thread.h>
#include <map>

namespace OpenHome {
namespace Av {

// Wraps an IFriendlyNameObservable, re-notifying observers with a prefix
// prepended to the friendly name ("<prefix> <name>").
//
// Used for the ODP zeroconf announcement so the _odp._tcp instance name can
// never collide with the host name announced by the system mDNS daemon
// (avahi/mdnsd announce "<hostname>.local"; ohNet asserts on name conflicts,
// see MdnsPlatform.cpp StatusCallback).
class PrefixedFriendlyNameObservable : public IFriendlyNameObservable
{
public:
    PrefixedFriendlyNameObservable(IFriendlyNameObservable& aObservable, const Brx& aPrefix);
    ~PrefixedFriendlyNameObservable();
private: // from IFriendlyNameObservable
    TUint RegisterFriendlyNameObserver(FunctorGeneric<const Brx&> aObserver) override;
    void DeregisterFriendlyNameObserver(TUint aId) override;
private:
    void FriendlyNameChanged(const Brx& aName);
    void NotifyLocked(const Brx& aName);
private:
    IFriendlyNameObservable& iObservable;
    const Brn iPrefix;
    Bws<IFriendlyNameObservable::kMaxFriendlyNameBytes + 32> iName;
    TUint iUpstreamId;
    TUint iNextObserverId;
    std::map<TUint, FunctorGeneric<const Brx&>> iObservers;
    Mutex iLock;
};

} // namespace Av
} // namespace OpenHome
