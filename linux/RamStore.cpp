#include "RamStore.h"
#include <OpenHome/Av/KvpStore.h>
#include <OpenHome/Private/Printer.h>

#include <map>

using namespace OpenHome;
using namespace OpenHome::Av;

RamStore::RamStore(const Brx& aImageFileName)
    : iImageFileName(aImageFileName)
{
}

typedef struct {
    uint32_t namesz;
    uint32_t descsz;
    uint32_t type;
    uint8_t data[];
} ElfNoteSection_t;

extern const ElfNoteSection_t g_note_build_id;

RamStore::~RamStore() {}

void RamStore::LoadStaticData(IStoreLoaderStatic& aLoader)
{
    aLoader.AddStaticItem(StaticDataKey::kBufManufacturerName, "Delescluze");
    aLoader.AddStaticItem(StaticDataKey::kBufManufacturerInfo, "delescluze.vjf");
    aLoader.AddStaticItem(StaticDataKey::kBufManufacturerUrl, "http://www.openhome.org");
    aLoader.AddStaticItem(StaticDataKey::kBufManufacturerImageUrl, "http://wiki.openhome.org/mediawiki/skins/openhome/images/logo.png");
    aLoader.AddStaticItem(StaticDataKey::kBufModelName, "Raspdac Audiophonics");
    aLoader.AddStaticItem(StaticDataKey::kBufModelInfo, "ohPlayer");
    aLoader.AddStaticItem(StaticDataKey::kBufModelUrl, "https://github.com/jchassin/ohPlayer");
    aLoader.AddStaticItem(StaticDataKey::kBufModelImageUrl, iImageFileName.CString());
    aLoader.AddStaticItem(StaticDataKey::kBufModelImageHiresUrl, iImageFileName.CString());
}
