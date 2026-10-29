# Radio

The player offers two complementary ways to listen to internet radio: **presets** (quick access, remote-control style) and the **Radio Browser** (full navigation of the community directory). Both play through the same audio pipeline (icecast/AAC, HLS m3u8 streams).

## 1. Radio presets

The Radio source exposes 10 presets, fully configurable in the web configuration ui (Radio tab). Each preset has:

* **Name** — station title shown in control point apps
* **URL** — stream address (icecast/aac, HLS m3u8, ...)
* **Image URL** — optional station logo, shown as preset artwork

Presets with an empty name or URL are unset. Changes take effect without a reboot.

Defaults: the six Radio France hifi AAC stations (France Inter, FIP, France Musique, France Info, France Culture, Mouv') and four RNE Spanish national stations (Radio Nacional, Radio Clasica, Radio 3, Radio 5, HLS streams). Values already saved in the store always override the defaults.

Presets can be pinned from control point apps; pins reference the preset number.

## 2. Radio Browser

A DLNA Media Server device exposing the [radio-browser.info](https://www.radio-browser.info) community radio directory (45,000+ internet radio stations, no account, no key). It appears in UPnP control points as **Radio Browser \<room\>**, alongside other media servers (e.g. MinimServer).

The media server is **disabled by default**; start the player with the `--radiobrowser` command line option to enable it (e.g. add it to `ExecStart` in `ohPlayer.service`).

### Navigation

```
Most popular          top stations by click count
By country           FR ES GB DE IT US (short, configurable in code)
└── <country>
    ├── Hi-Res       >=192 kbps, alphabetical
    ├── Mid-Res      96-191 kbps (+ unknown bitrate, e.g. HLS), alphabetical
    └── Low-Res      <96 kbps, alphabetical
By genre             jazz rock classical news pop electronic
└── <genre>
    └── (same Hi/Mid/Low structure)
By name              A-Z, search-by-navigation
└── <letter>
    └── (same Hi/Mid/Low structure)
```

Selecting a station plays it on the renderer through its usual protocols; the station artwork (favicon) is passed to the control point.

### Search

* **Control points with a generic DLNA search box** (ohSpy, BubbleUPnP, mconnect): the standard ContentDirectory `Search` action is supported (`dc:title contains "..."`).
* **Linn app / Kazoo**: these apps do not send `Search` requests to generic DLNA servers (their search box only covers their native partner services). Use **By name (A-Z)** to look up a station by its first letter instead; use **pins** to keep quick access to favourite stations.

### Quality of life

* Stations known to be down (`lastcheckok=0` at the last automated check) are hidden.
* Duplicate submissions of the same stream are deduplicated (http/https, host case and query strings are ignored when comparing).
* Station lists are cached for 60 seconds to keep paging responsive.

### Details

* The device runs on the player's UPnP port; its UDN is derived deterministically from the renderer's UDN.
* `fetch_deps.sh` compiles the ContentDirectory/ConnectionManager device providers from the ohNetGenerated sources (the prebuilt library does not ship them).
