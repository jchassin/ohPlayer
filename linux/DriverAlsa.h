#ifndef HEADER_PIPELINE_DRIVER_ALSA
#define HEADER_PIPELINE_DRIVER_ALSA

#include <OpenHome/OhNetTypes.h>
#include <OpenHome/Media/Pipeline/Msg.h>
#include <OpenHome/Media/Utils/ProcessorPcmUtils.h>
#include <OpenHome/Private/Thread.h>

namespace OpenHome {
namespace Media {

class PriorityArbitratorDriver : public IPriorityArbitrator, private INonCopyable
{
public:
    PriorityArbitratorDriver(TUint aOpenHomeMax);
private: // from IPriorityArbitrator
    TUint Priority(const TChar* aId, TUint aRequested, TUint aHostMax) override;
    TUint OpenHomeMin() const override;
    TUint OpenHomeMax() const override;
    TUint HostRange() const override;
private:
    const TUint iOpenHomeMax;
};


class DriverAlsa : public Thread, private IMsgProcessor, public IPipelineAnimator
{
public:
    DriverAlsa(IPipeline& aPipeline, TUint aBufferUs);
    ~DriverAlsa();
public: // Thread
    void Run();
private: // from IMsgProcessor
    Msg* ProcessMsg(MsgMode* aMsg) override;
    Msg* ProcessMsg(MsgTrack* aMsg) override;
    Msg* ProcessMsg(MsgChangeInput* aMsg) override;
    Msg* ProcessMsg(MsgDelay* aMsg) override;
    Msg* ProcessMsg(MsgEncodedStream* aMsg) override;
    Msg* ProcessMsg(MsgAudioEncoded* aMsg) override;
    Msg* ProcessMsg(MsgMetaText* aMsg) override;
    Msg* ProcessMsg(MsgStreamInterrupted* aMsg) override;
    Msg* ProcessMsg(MsgHalt* aMsg) override;
    Msg* ProcessMsg(MsgFlush* aMsg) override;
    Msg* ProcessMsg(MsgWait* aMsg) override;
    Msg* ProcessMsg(MsgDecodedStream* aMsg) override;
    Msg* ProcessMsg(MsgAudioPcm* aMsg) override;
    Msg* ProcessMsg(MsgSilence* aMsg) override;
    Msg* ProcessMsg(MsgPlayable* aMsg) override;
    Msg* ProcessMsg(MsgQuit* aMsg) override;
    Msg* ProcessMsg(MsgSession* aMsg) override;
private: // from IPipelineAnimator
    TUint PipelineDriverDelayJiffies(TUint aSampleRateFrom, TUint aSampleRateTo) override;
private:
    class Pimpl;
    Pimpl* iPimpl;
    IPipeline& iPipeline;
    Mutex iMutex;
    TBool iQuit;
};

} // namespace Media
} // namespace OpenHome

#endif // HEADER_PIPELINE_DRIVER_ALSA