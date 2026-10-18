ViewModel = function(){

    function RoomDisplayName(aResponse) {
        for(var i = 0; i < aResponse.length; i++){
            if (aResponse[i].key == 'RoomDisplayName' && aResponse[i].value.length > 0) {
                return aResponse[i].value;
            }
        }
        return "Room";
    }

    function RoomDisplayNameHelpText(aResponse) {
        for(var i = 0; i < aResponse.length; i++){
            if (aResponse[i].key == 'RoomDisplayName' && aResponse[i].value.length > 0) {
                return "Helptext.Product.RoomDisplayName." + aResponse[i].value;
            }
        }
        return "Helptext.Product.RoomDisplayName.Room";
    }

    function InputCollections(aResponse) {
        var fields = [];
        for(var i = 0; i < aResponse.length; i++){
            var collectionKey = aResponse[i].key;
            if (collectionKey.toLowerCase().startsWith('inputcollection.')) {
                // FIXME - this is relying on parsing a key to get a suitable title prefix. Imposes requirement on "InputCollection.X" keys, where X must be a human-readable collection ID that can be displayed in the UI (or must be explicitly mapped to such here).
                var split = collectionKey.split(".");
                if (split.length > 1) {
                    var titlePrefix = split[1];
                    fields.push({ key:collectionKey, helptext:"Helptext.InputCollection", title:titlePrefix + " Input Mode", type:"choice" });
                }
            }
        }
        return fields;
    }

    function CreateContainerMapping(aSources, aResponse) {
        var containerMapping = [
        {
            // TODO: views.js has a some lookup on these keys in Render fn, amend these when the "real" key is agreed and replaced here!
            containerId: "Display.Tabs.About.BasicInfo",
            fields: [
                { key:"About.ModelUrl", type:"link", title:"Model Name", linktext:"<% if ('About.ModelName' in fields)  { %> <%- fields['About.ModelName'].value %> <% } %>", linktitle:"Model Name" },
                { key:"About.ManufacturerUrl", title:"Manufacturer", type:"link", linktext:"<% if ('About.ManufacturerName' in fields)  { %> <%- fields['About.ManufacturerName'].value %> <% } %>", linktitle:"Manufacturer"},
                { key:"About.ProductId", title:"Product Id" },
            ]
        },
        {
            containerId: "Display.Tabs.About.DetailedInfo",
            fields: [
                { key:"About.SwVersion", title:"Software Version" },
                { key:"About.OhNetVersion", title:"ohNet Version" },
                { key:"About.OhPipelineVersion", title:"ohPipeline Version" },
                { key:"About.OhPlayerCommit", title:"ohPlayer Commit" },
                { key:"About.ModelVariant", title:"Software Variant" },
                { key:"About.SwReleaseNotesUrl", type:"link", title:"Software Update Available", linktext:"<% if ('About.SwUpdate' in fields)  { %> <%- fields['About.SwUpdate'].value %> <% } %>", linktitle:"Release Notes" },
                { key:"About.IpAddress", title:"IP Address" },
                { key:"About.MacAddress", title:"MAC Address" },
                { key:"About.ModelInfo", title:"Model Information" },
                { key:"About.BoardInfo", title:"Board Information", listheaders:['Board Type', 'ID', 'Description']}
            ]
        },
        {
            containerId:"Display.Tabs.Device.BasicInfo",
            fields:[
                { key:"Product.Room", helptext:RoomDisplayNameHelpText(aResponse), title:RoomDisplayName(aResponse) },
                { key:"Product.Name", helptext:"Helptext.Product.Name", title:"Name" },
            ]
        },
        ];

        {
            var id = "Display.Tabs.Device.DetailedInfo";
            var fieldList = [
                // TODO: Delay
                { key:"Device.AdminMode", helptext:"Helptext.Device.AdminMode", title:"Admin Mode" },
                { key:"Device.AnalogOutMode", helptext:"Helptext.Device.AnalogOutMode", title:"Analog Output Mode" },
                { key:"Device.AutoPlay", helptext:"Helptext.Device.AutoPlay", title:"Auto Play" },
                { key:"Device.Basik3", helptext:"Helptext.Device.Basik3", title:"Basik 3 Commands Accepted" },
                { key:"Device.SpdifMode", helptext:"Helptext.Device.SpdifMode", title:"SPDIF Mode" },
                { key:"Device.DigitalOutputMode", helptext:"Helptext.Device.DigitalOutputMode", title:"Digital Output Mode" },
                { key:"Device.EthernetLeds", helptext:"Helptext.Device.EthernetLeds", title:"Ethernet LEDs" },
                { key:"Device.ExaktLeds", helptext:"Helptext.Device.ExaktLeds", title:"Exakt LEDs" },
                { key:"Device.Hdmi", helptext:"Helptext.Device.Hdmi", title:"HDMI" },
                { key:"Device.PowerAmp", helptext:"Helptext.Device.PowerAmp", title:"Internal Power Amplifier" },
                { key:"Device.Wifi", helptext:"Helptext.Device.Wifi", title:"Wi-Fi" },
                //{ key:"Device.WifiPowerMode", helptext:"Helptext.Device.WifiPowerMode", title:"Wi-Fi Power Mode" },
                { key:"Device.IPv6", helptext:"Helptext.Device.IPv6", title:"IPv6" },
                { key:"Device.Bluetooth", helptext:"Helptext.Device.Bluetooth", title:"Bluetooth" },
                { key:"Device.AmpModeBridged", helptext:"Helptext.Device.AmpModeBridged", title:"Internal Power Amplifier Mode" },
                { key:"Device.AmpModeBiAmped", helptext:"Helptext.Device.AmpModeBiAmped", title:"Internal Power Amplifier Mode" },
                { key:"Device.PreOutMode", helptext:"Helptext.Device.PreOutMode", title:"Pre Out/Power In Connector Mode" },
                { key:"Device.RcaGroundLift", helptext:"Helptext.Device.RcaGroundLift", title:"RCA Ground Lift" },
                { key:"Cloud.OptOut", helptext:"Helptext.Cloud.OptOut", title:"Send Usage Data" },
                { key:"Device.CrashReports", helptext:"Helptext.Device.CrashReports", title:"Send Crash Data" },
                { key:"Device.StartupMode", helptext:"Helptext.Device.StartupMode", title:"Startup Mode" },
                { key:"Source.StartupName", helptext:"Helptext.Device.StartupSource", title:"Startup Source", type:"choice" },
                { key:"Kustom.AutoStandby", helptext:"Helptext.Kustom.AutoStandby", title:"Auto Standby" },
                { key:"Delay.Value", helptext:"Helptext.Delay.Value", title:"Delay (ms)" },
                { key:"Device.VerboseDeviceLogging", helptext:"Helptext.Device.VerboseDeviceLogging", title:"Verbose Device Logging" },
                { key:"Analogue.Exakt.Dual.Fronts", helptext:"Helptext.Analogue.Exakt.Dual.Fronts", title:"Dual Analogue/Exakt Stereo Output"},
                { key:"Xlr.0.Out", helptext:"Helptext.Device.XlrOutput", title:"XLR Output" },
                { key:"Rca.0.Out", helptext:"Helptext.Device.RcaOutput", title:"RCA Output" },
                { key:"Rca.0.Xfmr", helptext:"Helptext.Device.RcaOutputTransformer", title:"RCA Output Transformer" },
                { key:"Headphone.Mode", helptext:"Helptext.Device.Headphone.Mode", title:"Headphone Mode" },
                { key:"Sub.Enabled", helptext:"Helptext.Device.Sub.Enabled", title:"Subwoofer" },
                { key:"Sub.Filter.Enabled", helptext:"Helptext.Device.Sub.Filter.Enabled", title:"Subwoofer Filter" },
                { key:"Sub.Filter.Frequency", helptext:"Helptext.Device.Sub.Frequency", title:"Subwoofer Crossover Frequency" },
                { key:"Sub.Filter.Mode", helptext:"Helptext.Device.Sub.Filter.Mode", title:"Subwoofer Crossover Type" },
                { key:"Sub.Channels", helptext:"Helptext.Device.Sub.Channels", title:"Subwoofer Channels" },
                { key:"Exakt.Backplane.StandbyPolicy", helptext:"Helptext.Exakt.Backplane.StandbyPolicy", title:"Standby Mode" },
                { key:"CiGateway.Enable", helptext:"Helptext.CiGateway.Enable", title:"CI Gateway" },
            ];
            fieldList = fieldList.concat(InputCollections(aResponse));

            containerMapping = containerMapping.concat([{
                containerId: id,
                fields: fieldList
            }]);
        }

        // append source specific fields
        for(var i = 0; i < aSources.length; i++) {
            var src = aSources[i];
            var fieldList = [
                { key: "Source." + src + ".Name", helptext:"Helptext.Source.Name", title:"Name" },
                { key: "Source." + src + ".Visible", helptext:"Helptext.Source.Visible", title:"Visible" },
                { key: "Source." + src + ".UnityGain", helptext:"Helptext.Source.UnityGain", title:"Unity Gain" },
                { key: "Source." + src + ".InputGain", helptext:"Helptext.Source.InputGain", title:"Input Gain (dB)" },
                { key: "Source." + src + ".VolumeOffset", helptext:"Helptext.Source.VolumeOffset", title:"Volume Offset", valueConverter:Converters.MilliDbToDb },
                { key: "Source." + src + ".InputLevel", helptext:"Helptext.Source.InputLevel", title:"Input Level (Vrms)" },
                { key: "Source." + src + ".InputTransformer", helptext:"Helptext.Source.InputTransformer", title:"Input Transformer" },
                { key: "Source." + src + ".DigitalFeatures", helptext:"Helptext.Source.DigitalFeatures", title:"Digital Features" },
                { key: "Source." + src + ".DelayMode", helptext:"Helptext.Source.DelayMode", title:"Delay Mode" },
            ];

            if (src.includes("Analog") || src.includes("Balanced")) {
                fieldList.push({ key: "Source." + src + ".InputLevel.uV", helptext:"Helptext.Source.InputLevel", title:"Input Level (Vrms)", valueConverter:Converters.DivideHundredThousand });
            }
            else if (src.includes("Phono")) {
                fieldList.push({ key: "Source." + src + ".InputLevel.uV", helptext:"Helptext.Source.InputLevel.mV", title:"Max. Input Level (mVrms)", valueConverter:Converters.DivideThousand });
            }
            else {
                fieldList.push({ key: "Source." + src + ".InputLevel.uV", helptext:"Helptext.Source.InputLevel.mV", title:"Input Level (mVrms)", valueConverter:Converters.DivideThousand });
            }

            if (src == "Playlist") {
                fieldList = fieldList.concat([
                    { key: "Playlist.TracksMax", helptext: "Helptext.Playlist.TracksMax", title: "Maximum Number of Tracks" }
                ]);
            }
            if (src == "Radio") {
                fieldList = fieldList.concat([
                    { key:"Radio.TuneInUserName", helptext:"Helptext.Radio.TuneInUserName", title:"TuneIn Username", type:"combo" }
                ]);
            }
            if (src == "Spotify") {
                fieldList = fieldList.concat([
                    { key:"Spotify.DelayMode", helptext:"Helptext.Spotify.DelayMode", title:"Delay Mode", type:"choice" }
                ]);
            }
            if (src.startsWith("Roon")) {
                fieldList = fieldList.concat([
                    { key: "Roon.Protocol", helptext: "Helptext.Roon.Protocol", title: "Roon protocol", type: "choice" },
                    { key: "Raat.DsdEnable", helptext: "Helptext.Raat.DsdEnable", title: "DSD decode", type: "choice" }
                ]);
            }
            if (src == "Bluetooth") {
                fieldList = fieldList.concat([
                    { key:"Bluetooth.DelayMode", helptext:"Helptext.Bluetooth.DelayMode", title:"Delay Mode", type:"choice" }
                ]);
            }

            containerMapping = containerMapping.concat([{
                containerId: "Display.Tabs.Sources." + src.replace(" ",""),
                fields: fieldList
            }]);
        }

        // and the rest of the fields after sources
        containerMapping = containerMapping.concat([{
            containerId:"Display.Tabs.Display.BasicInfo",
            fields:[
                { key:"Display.AutoBrightness", helptext:"Helptext.Display.AutoBrightness", title:"Auto Brightness" },
                { key:"Display.Brightness", helptext:"Helptext.Display.Brightness", title:"Brightness" },
            ]
        },
        {
            containerId:"Display.Tabs.Display.DetailedInfo",
            fields:[
                { key:"Display.FpButtons", helptext:"Helptext.Display.FpButtons", title:"Front Panel Buttons" },
                { key:"Display.Orientation", helptext:"Helptext.Display.Orientation", title:"Orientation" },
                { key:"Display.ScrollText", helptext:"Helptext.Display.ScrollText", title:"Scroll Text on Track Change" },
                { key:"Display.SleepMode", helptext:"Helptext.Display.SleepMode", title:"Sleep Mode" },
                { key:"Display.ShowUpdates", helptext:"Helptext.Display.ShowUpdates", title:"Update Notifications" },
                { key:"Display.Led", helptext:"Helptext.Display.Led", title:"Power Indicator (Active mode)" },
                { key:"Display.LedOffInSleep", helptext:"Helptext.Display.LedOffInSleep", title:"Power Indicator (Sleep mode)" },
                { key:"Display.Proximity", helptext:"Helptext.Display.Proximity", title:"Wake Display on Proximity Detection" },
                { key:"Display.DiscreteMode", helptext:"Helptext.Display.DiscreteMode", title:"Discrete Mode" },
                { key:"Display.LedColor", helptext:"Helptext.Display.LedColor", title:"Power Indicator" },
                { key:"Display.Timeout", helptext:"Helptext.Display.Timeout", title:"Timeout" },
            ]
        },
        {
            containerId : "Display.Tabs.Volume.BasicInfo",
            fields : [
                { key:"Volume.Enabled", helptext:"Helptext.Volume.Enabled", title:"Internal Volume Control" },
            ]
        },
        {
            containerId:"Display.Tabs.Volume.DetailedInfo",
            fields : [
                { key:"Volume.Balance", helptext:"Helptext.Volume.Balance", title:"Balance" },
                // TODO: output attenuation?
                { key:"Volume.HeadphoneOffset", helptext:"Helptext.Volume.HeadphoneOffset", title:"Headphone Offset", valueConverter:Converters.MilliDbToDb },
                { key:"Volume.OutputAttenuation", helptext:"Helptext.Volume.OutputAttenuation", title:"Output Attenuation" },
                { key:"Volume.StartupValue", helptext:"Helptext.Volume.StartupValue", title:"Startup Volume" },
                { key:"Volume.StartupEnabled", helptext:"Helptext.Volume.StartupEnabled", title:"Startup Volume Enabled" },
                { key:"Volume.Limit", helptext:"Helptext.Volume.Limit", title:"Volume Limit" },
            ]
        },
        {
            containerId : "Display.Tabs.Hdmi.BasicInfo",
            fields : [
                { key:"Hdmi.OffInStandby", helptext:"Helptext.Hdmi.OffInStandby", title:"Turn HDMI Off in Sleep" },
                { key:"Hdmi.CecEnable", helptext:"Helptext.Hdmi.CecEnable", title:"CEC" },
                { key:"Hdmi.CecSourceSelectEnable", helptext:"Helptext.Hdmi.CecSourceSelectEnable", title:"CEC Auto Source / Power Control" },
                { key:"Hdmi.eArcMode", helptext:"Helptext.Hdmi.eArcMode", title:"eARC Mode" },

            ]
        },
        {
            containerId:"Display.Tabs.Handset.BasicInfo",
            fields : [
                { key:"Device.HandsetCommands", helptext:"Helptext.Device.HandsetCommands", title:"Handset Commands Accepted", type:"choice" },
                { key:"Handset.Input1", helptext:"Helptext.Handset.Input", title:"Input 1 Button", type:"choice" },
                { key:"Handset.Input2", helptext:"Helptext.Handset.Input", title:"Input 2 Button", type:"choice" },
                { key:"Handset.Input3", helptext:"Helptext.Handset.Input", title:"Input 3 Button", type:"choice" },
                { key:"Handset.StarKeyHeldMode", helptext:"Helptext.Handset.StarKeyHeldMode", title:"Star Key Held Action", type:"choice" },
            ]
        },
        {
            containerId:"Display.Tabs.RS232.BasicInfo",
            fields : [
                { key:"LinnAmp.VolumeControl", helptext:"Helptext.LinnAmp.VolumeControl", title:"Volume Control", type:"choice" },
                { key:"LinnAmp.SerialPort", helptext:"Helptext.LinnAmp.SerialPort", title:"Serial Port", type:"string" },
                { key:"LinnAmp.Bass", helptext:"Helptext.LinnAmp.Bass", title:"Bass", type:"numeric" },
                { key:"LinnAmp.Treble", helptext:"Helptext.LinnAmp.Treble", title:"Treble", type:"numeric" },
                { key:"LinnAmp.Model", helptext:"Helptext.LinnAmp.Model", title:"Detected Amplifier" },
                { key:"LinnAmp.Serial", helptext:"Helptext.LinnAmp.Serial", title:"Amplifier Serial" },
            ]
        },
        {
            containerId:"Display.Tabs.SongcastSender.BasicInfo",
            fields:[
                { key:"Sender.Enabled", helptext:"Helptext.Sender.Enabled", title:"Enabled" },
            ]
        },
        {
            containerId:"Display.Tabs.SongcastSender.DetailedInfo",
            fields:[
                { key:"Sender.Mode", helptext:"Helptext.Sender.Mode", title:"Output Mode" },
                { key:"Sender.Channel", helptext:"Helptext.Sender.Channel", title:"Multicast Channel" },
                { key:"Sender.Preset", helptext:"Helptext.Sender.Preset", title:"Preset" },
            ]
        },
        {
            containerId : "Display.Tabs.StreamingServices.Tidal",
            fields:[
                { key:"tidalhifi.com.Enabled", helptext:"Helptext.StreamingService.Enabled", title:"Enabled", helptextparams:{service:"TIDAL",servicelinkuri:"http://www.tidalhifi.com/",servicelinktitle:"tidalhifi.com"} },
                { key:"tidalhifi.com.SoundQuality", helptext:"Helptext.StreamingService.SoundQuality", title:"Maximum Audio Quality", helptextparams:{service:"TIDAL"} },
            ]
        },
        {
            containerId : "Display.Tabs.StreamingServices.Qobuz",
            fields : [
                { key:"qobuz.com.Enabled", helptext:"Helptext.StreamingService.Enabled", title:"Enabled", helptextparams:{service:"Qobuz",servicelinkuri:"http://www.qobuz.com/",servicelinktitle:"qobuz.com"} },
                { key:"qobuz.com.SoundQuality", helptext:"Helptext.StreamingService.SoundQuality", title:"Maximum Audio Quality", helptextparams:{service:"Qobuz"} },
            ]
        },
        {
            containerId : "Display.Tabs.StreamingServices.TuneIn",
            fields : [
                { key:"tunein.com.Enabled", helptext:"Helptext.StreamingService.Enabled", title:"Enabled", helptextparams:{service:"TuneIn",servicelinkuri:"http://TuneIn.com/",servicelinktitle:"TuneIn.com"} },
                { key:"tunein.com.UserName", helptext:"Helptext.StreamingService.UserName", title:"Username", helptextparams:{service:"TuneIn",servicelinkuri:"http://TuneIn.com/",servicelinktitle:"TuneIn.com"} },
            ]
        },
        {
            containerId : "Display.Tabs.StreamingServices.CalmRadio",
            fields : [
                { key:"calmradio.com.Enabled", helptext:"Helptext.StreamingService.Enabled", title:"Enabled", helptextparams:{service:"Calm Radio",servicelinkuri:"http://calmradio.com/",servicelinktitle:"CalmRadio.com"} },
            ]
        },
        {
            containerId : "Display.Tabs.StreamingServices.Spotify",
            fields : [
                { key:"Spotify.Bitrate", helptext:"Helptext.StreamingService.SoundQuality", title:"Maximum Audio Quality", helptextparams:{service:"Spotify"} },
            ]
        },
        {
            containerId : "Display.Tabs.StreamingServices.Deezer",
            fields:[
                { key:"deezer.com.SoundQuality", helptext:"Helptext.StreamingService.SoundQuality", title:"Maximum Audio Quality", helptextparams:{service:"Deezer"} },
            ]
        },
        {
            containerId: "Display.Tabs.Dolby.BasicInfo",
            fields: [
                { key:"AudioDsp.Pl2Mode", helptext:"Helptext.Dolby.Pl2Mode", title:"Decode Mode" },
                { key:"AudioDsp.DynamicRange", helptext:"Helptext.Dolby.DynamicRange", title:"Dynamic Range Compression" },
                { key:"AudioDsp.LfeBoost", helptext:"Helptext.Dolby.LfeBoost", title:"LFE Boost" },
                { key:"AudioDsp.DialogNorm", helptext:"Helptext.Dolby.DialogNorm", title:"Dialogue Normalization" },
                { key:"AudioDsp.StereoMode", helptext:"Helptext.Dolby.StereoMode", title:"Stereo Mode" },
                { key:"AudioDsp.SpaceEnable", helptext:"Helptext.Dolby.Space", title:"Space Optimisation" },
                { key:"AudioDsp.SpaceOptEnabledString", title:"Speaker Configuration" }
            ]
        },
        {
            containerId: "Display.Tabs.Dolby.SpkConfig", //disable if Space enabled
            fields: [
                { key:"AudioDsp.SpkConfig", helptext:"Helptext.Dolby.SpkConfig", title:"Speakers" },
                { key:"AudioDsp.ExtSpkConfig", helptext:"Helptext.Dolby.ExtSpkConfig", title:"Extended Speaker Type" },
                { key:"AudioDsp.LfeVolTrim", helptext:"Helptext.Dolby.LfeVolTrim", title:"LFE Volume Trim", valueConverter:Converters.MilliDbToDb },
                { key:"AudioDsp.TestToneEnable", helptext:"Helptext.Dolby.TestToneEnable", title:"Test tone (Select 'Speaker Setup' source)"},
                ]
         },
         {
            containerId: "Display.Tabs.Dolby.Front",
            fields: [
                { key:"AudioDsp.SpkSize.g0", helptext:"Helptext.Dolby.SpkSize", title:"Size" }, // group 0
                { key:"AudioDsp.SpeakerDistance.0", helptext:"Helptext.Dolby.SpeakerDistance", title:"Distance to Front Left (m)", valueConverter:Converters.HundredsToHundredths },
                { key:"AudioDsp.VolTrim.0", helptext:"Helptext.Dolby.VolTrim", title:"Front Left volume Trim", valueConverter:Converters.MilliDbToDb },

                { key:"AudioDsp.SpeakerDistance.1", helptext:"Helptext.Dolby.SpeakerDistance", title:"Distance to Front Right (m)", valueConverter:Converters.HundredsToHundredths },
                { key:"AudioDsp.VolTrim.1", helptext:"Helptext.Dolby.VolTrim", title:"Front Right volume Trim", valueConverter:Converters.MilliDbToDb },
            ]
         },
         {
            containerId: "Display.Tabs.Dolby.Centre",
            fields: [
                { key:"AudioDsp.SpkSize.g1", helptext:"Helptext.Dolby.SpkSize", title:"Size" }, // group 1
                { key:"AudioDsp.SpeakerDistance.2", helptext:"Helptext.Dolby.SpeakerDistance", title:"Distance to Centre (m)", valueConverter:Converters.HundredsToHundredths },
                { key:"AudioDsp.VolTrim.2", helptext:"Helptext.Dolby.VolTrim", title:"Centre volume Trim", valueConverter:Converters.MilliDbToDb },
            ]
         },
         {
            containerId: "Display.Tabs.Dolby.Surround",
            fields: [
                { key:"AudioDsp.SpkSize.g2", helptext:"Helptext.Dolby.SpkSize", title:"Size" }, // group 2
                { key:"AudioDsp.SpeakerDistance.3", helptext:"Helptext.Dolby.SpeakerDistance", title:"Distance to Surround Left (m)", valueConverter:Converters.HundredsToHundredths },
                { key:"AudioDsp.VolTrim.3", helptext:"Helptext.Dolby.VolTrim", title:"Surround Left volume Trim", valueConverter:Converters.MilliDbToDb },

                { key:"AudioDsp.SpeakerDistance.4", helptext:"Helptext.Dolby.SpeakerDistance", title:"Distance to Surround Right (m)", valueConverter:Converters.HundredsToHundredths },
                { key:"AudioDsp.VolTrim.4", helptext:"Helptext.Dolby.VolTrim", title:"Surround Right volume Trim", valueConverter:Converters.MilliDbToDb },
            ]
         },
         {
            containerId: "Display.Tabs.Dolby.Back",
            fields: [
                { key:"AudioDsp.SpkSize.g3", helptext:"Helptext.Dolby.SpkSize", title:"Size" }, // group 3
                { key:"AudioDsp.SpeakerDistance.5", helptext:"Helptext.Dolby.SpeakerDistance", title:"Distance to Back Left (m)", valueConverter:Converters.HundredsToHundredths },
                { key:"AudioDsp.VolTrim.5", helptext:"Helptext.Dolby.VolTrim", title:"Back Left volume Trim", valueConverter:Converters.MilliDbToDb },

                { key:"AudioDsp.SpeakerDistance.6", helptext:"Helptext.Dolby.SpeakerDistance", title:"Distance to Back Right (m)", valueConverter:Converters.HundredsToHundredths },
                { key:"AudioDsp.VolTrim.6", helptext:"Helptext.Dolby.VolTrim", title:"Back Right volume Trim", valueConverter:Converters.MilliDbToDb },
            ]
         },
         {
            containerId: "Display.Tabs.Dolby.Sub",
            fields: [
                { key:"AudioDsp.SpeakerDistance.7", helptext:"Helptext.Dolby.SpeakerDistance", title:"Distance to Sub (m)", valueConverter:Converters.HundredsToHundredths },
                { key:"AudioDsp.VolTrim.7", helptext:"Helptext.Dolby.VolTrim", title:"Sub volume Trim", valueConverter:Converters.MilliDbToDb },
            ]
         },
        ]);

    return containerMapping;
    }

    function CreateLegacyContainerStructure(aSources) {
        // this should eventually disappear but is presently used to generate the structure of the legacy parameter json used by the jquery renderer
        var legacyContainerStructure = [
            { key: "Display.Tabs.About.BasicInfo", mapping : { Target:"About", Collection:"", Section:"Basic", ShowSectionHeader:false }},
            { key: "Display.Tabs.About.DetailedInfo", mapping : { Target:"About", Collection:"", Section:"Advanced" }},
            { key: "Display.Tabs.Device.BasicInfo", mapping : { Target:"Device", Collection:"", Section:"Basic", ShowSectionHeader:false }},
            { key: "Display.Tabs.Device.DetailedInfo", mapping : { Target:"Device", Collection:"", Section:"Advanced" }},
            { key: "Display.Tabs.Display.BasicInfo", mapping : { Target:"Display", Collection:"", Section:"Basic", ShowSectionHeader:false }},
            { key: "Display.Tabs.Display.DetailedInfo", mapping : { Target:"Display", Collection:"", Section:"Advanced" }},
            { key: "Display.Tabs.Volume.BasicInfo", mapping : { Target:"Volume", Collection:"", Section:"Basic", ShowSectionHeader:false }},
            { key: "Display.Tabs.Volume.DetailedInfo", mapping : { Target:"Volume", Collection:"", Section:"Advanced" }}
        ];

        // generate containers for sources
        for(var i = 0; i < aSources.length; i++){
            var src = aSources[i];
            var containerName = "Display.Tabs.Sources." + src.replace(" ", "");
            legacyContainerStructure = legacyContainerStructure.concat([{ key:containerName, mapping : {    Target:src, Collection:"Sources", Section:"" }}]);
        }

        legacyContainerStructure = legacyContainerStructure.concat([
            { key: "Display.Tabs.SongcastSender.BasicInfo", mapping : { Target:"Songcast Sender", Collection:"", Section:"Basic", ShowSectionHeader:false }},
            { key: "Display.Tabs.SongcastSender.DetailedInfo", mapping : { Target:"Songcast Sender", Collection:"", Section:"Advanced" }},
            { key: "Display.Tabs.StreamingServices.Qobuz", mapping : { Target:"Qobuz", Collection:"Streaming Services", Section:"" }},
            { key: "Display.Tabs.StreamingServices.Tidal", mapping : { Target:"TIDAL", Collection:"Streaming Services", Section:"" }},
            { key: "Display.Tabs.StreamingServices.TuneIn", mapping : { Target:"TuneIn", Collection:"Streaming Services", Section:"" }},
            { key: "Display.Tabs.StreamingServices.CalmRadio", mapping : { Target:"Calm Radio", Collection:"Streaming Services", Section:"" }},
            { key: "Display.Tabs.StreamingServices.Spotify", mapping : { Target:"Spotify", Collection:"Streaming Services", Section:"" }},
            { key: "Display.Tabs.StreamingServices.Deezer", mapping : { Target:"Deezer", Collection:"Streaming Services", Section:"" }},
            { key: "Display.Tabs.Hdmi.BasicInfo", mapping : { Target:"HDMI", Collection:"", Section:"Basic", ShowSectionHeader:false }},
            { key: "Display.Tabs.Handset.BasicInfo", mapping : { Target:"Remote Control", Collection:"", Section:"Basic", ShowSectionHeader:false }},
            { key: "Display.Tabs.RS232.BasicInfo", mapping : { Target:"Linn Amp", Collection:"", Section: "Basic", ShowSectionHeader:false }},
            { key: "Display.Tabs.Dolby.BasicInfo", mapping : { Target:"Dolby/DTS", Collection:"", Section: "Basic", ShowSectionHeader:false }},
            { key: "Display.Tabs.Dolby.SpkConfig", mapping : { Target:"Dolby/DTS", Collection:"", Section: "Speaker Setup", ShowSectionHeader:true }},
            { key: "Display.Tabs.Dolby.Front", mapping : { Target:"Dolby/DTS", Collection:"", Section: "Front", ShowSectionHeader:true }},
            { key: "Display.Tabs.Dolby.Centre", mapping : { Target:"Dolby/DTS", Collection:"", Section: "Centre", ShowSectionHeader:true }},
            { key: "Display.Tabs.Dolby.Surround", mapping : { Target:"Dolby/DTS", Collection:"", Section: "Surround", ShowSectionHeader:true }},
            { key: "Display.Tabs.Dolby.Back", mapping : { Target:"Dolby/DTS", Collection:"", Section: "Back", ShowSectionHeader:true }},
            { key: "Display.Tabs.Dolby.Sub", mapping : { Target:"Dolby/DTS", Collection:"", Section: "Sub", ShowSectionHeader:true }},
        ]);

        return legacyContainerStructure;
    }

    var kHelpTexts = {
        "Helptext.Product.RoomDisplayName.Room" : {
            description: "Enter a name for the room the <%- device %> is installed in (e.g. Lounge). This will appear as the room name in control points."
        },
        "Helptext.Product.RoomDisplayName.Name" : {
            description: "Enter a name for the <%- device %>. This will not appear in control points."
        },
        "Helptext.Product.RoomDisplayName.Stream" : {
            description: "Enter a name for the stream provided by the <%- device %>. This will appear in control points."
        },
        "Helptext.Product.RoomDisplayName.Zone" : {
            description: "Enter a name for the zone the <%- device %> is installed in (e.g. Lounge). This will appear as the room name in control points."
        },
        "Helptext.Device.AdminMode" : {
            description: "Select <span class='HelpTextOption'><%- field.meta.options[1].value %></span> to put the device into admin mode. Certain features (i.e. software updates, space opt edit) will be hidden when enabled."
        },
        "Helptext.Device.AnalogOutMode" : {
            description:"Select your preferred analog output mode. This setting should match the output connection used on the <%- device %>."
        },
        "Helptext.Device.RcaGroundLift" : {
            description:"Lifting the ground will break the ground loop in installations with a secondary earth connection."
        },
        "Helptext.Device.AutoPlay" : {
            description:"If you wish the <%- device %> to auto play on source selection (including coming out of sleep), select <span class='HelpTextOption'><%- field.meta.options[1].value %></span>. Otherwise select <span class='HelpTextOption'><%- field.meta.options[0].value %></span>."
        },
        "Helptext.Device.Basik3" : {
            description: "Select which commands received via the Basik 3 in-wall control terminal the <%- device %> will accept. Basik 3 must be connected to the provided socket on the <%- device %>."
        },
        "Helptext.Device.SpdifMode" : {
            description : "Select how you wish to use the Spdif connector: as an output or an input"
        },
        "Helptext.Device.DigitalOutputMode":{
            description:"If your digital audio receiver can process audio at any sample rate, select <span class='HelpTextOption'><%- field.meta.options[0].value %></span>. Otherwise select <span class='HelpTextOption'><%- field.meta.options[1].value %></span> or <span class='HelpTextOption'><%- field.meta.options[2].value %></span> to sample convert all audio to the selected rate. The <span class='HelpTextOption'><%- field.meta.options[3].value %></span> option will output 192kHz audio with Space Optimization applied. Select <span class='HelpTextOption'><%- field.meta.options[4].value %></span> if you are not using the digital output."
        },
        "Helptext.Device.EthernetLeds":{
            description:"To turn off the LEDs on the ethernet socket on the back of your <%- device %> select <span class='HelpTextOption'><%- field.meta.options[1].value %></span>, otherwise select <span class='HelpTextOption'><%- field.meta.options[0].value %></span> to leave the LEDs on for diagnostic purposes.",
        },
        "Helptext.Device.ExaktLeds":{
            description:"To dim the LEDs on the Exakt sockets on the back of your <%- device %> and Exakt devices (where supported) select <span class='HelpTextOption'><%- field.meta.options[1].value %></span>, otherwise select <span class='HelpTextOption'><%- field.meta.options[0].value %></span> to leave the LEDs enabled for diagnostic purposes.",
        },
        "Helptext.Device.HandsetCommands" : {
            description : "Select which commands received via the IR receiver on the front panel the <%- device %> will accept. Please note, the <span class='HelpTextOption'>Slimline Handsets</span> (REM020 Silver or REM022 Black) use <span class='HelpTextOption'><%- field.meta.options[3].value %></span> mode only.",
        },
        "Helptext.Device.PowerAmp" : {
            description : "If you wish to use the <%- device %> with an external power amplifier, select <span class='HelpTextOption'><%- field.meta.options[1].value %></span>. Otherwise select <span class='HelpTextOption'><%- field.meta.options[0].value %></span> to use the internal power amplifier.",
        },
        "Helptext.Device.XlrOutput" : {
            description: "Select <span class='HelpTextOption'><%- field.meta.options[1].value %></span> to enable the XLR output."
        },
        "Helptext.Device.RcaOutput" : {
            description: "Select <span class='HelpTextOption'><%- field.meta.options[1].value %></span> to enable the RCA output."
        },
        "Helptext.Device.RcaOutputTransformer" : {
            description: "Select <span class='HelpTextOption'><%- field.meta.options[1].value %></span> to enable the RCA output transformer."
        },
        "Helptext.Device.Headphone.Mode" : {
            description: "Select <span class='HelpTextOption'><%- field.meta.options[0].value %></span> to dynamically enable and disable headphones based on whether headphones are connected to the output jack. To force the headphone output to always be enabled, select <span class='HelpTextOption'><%- field.meta.options[1].value %></span>. To disable headphone output select <span class='HelpTextOption'><%- field.meta.options[2].value %></span>."
        },
        "Helptext.Device.Sub.Enabled" : {
            description: "Select <span class='HelpTextOption'><%- field.meta.options[1].value %></span> to enable subwoofer output."
        },
        "Helptext.Device.Sub.Filter.Enabled" : {
            description: "Select <span class='HelpTextOption'><%- field.meta.options[1].value %></span> to enable subwoofer crossover filter."
        },
        "Helptext.Device.Sub.Frequency" : {
            description: "Select desired crossover frequencies based on your subwoofer response."
        },
        "Helptext.Device.Sub.Filter.Mode" : {
            description: "Select <span class='HelpTextOption'><%- field.meta.options[0].value %></span> to send low frequencies to both your subwoofer and standard speakers. Otherwise select <span class='HelpTextOption'><%- field.meta.options[1].value %></span> to send low frequencies to your subwoofer and high frequencies to your standard speakers."
        },
        "Helptext.Device.Sub.Channels" : {
            description: "Select <span class='HelpTextOption'><%- field.meta.options[0].value %></span> to enable stereo subwoofer output. Otherwise select <span class='HelpTextOption'><%- field.meta.options[1].value %></span> to enable mono subwoofer output."
        },
        "Helptext.Exakt.Backplane.StandbyPolicy" : {
            description: "Select <span class='HelpTextOption'><%- field.meta.options[1].value %></span> to enable low power mode when in standby. This will result in a delay when taking the device out of standby. Change in power mode will be applied when device next enters standby."
        },
        "Helptext.CiGateway.Enable" : {
            description: "Select <span class='HelpTextOption'><%- field.meta.options[1].value %></span> to enable the custom install gateway."
        },
        "Helptext.Device.Bluetooth" : {
            description : "If you wish to use the <%- device %> as a Bluetooth receiver, select <span class='HelpTextOption'><%- field.meta.options[0].value %></span>. Otherwise select <span class='HelpTextOption'><%- field.meta.options[1].value %></span>.",
        },
        "Helptext.Device.Wifi" : {
            description : "If you wish to connect the <%- device %> to your network using Wi-Fi, select <span class='HelpTextOption'><%- field.meta.options[0].value %></span>. Otherwise select <span class='HelpTextOption'><%- field.meta.options[1].value %></span>.",
        },
        //"Helptext.Device.WifiPowerMode" : {
        //    description : "If the <%- device %> is using Wi-Fi and Roon Sink, selecting <span class='HelpTextOption'><%- field.meta.options[1].value %></span> is recommended. Otherwise select <span class='HelpTextOption'><%- field.meta.options[0].value %></span>.",
        //},
        "Helptext.Device.IPv6" : {
            description : "To enable IPv6 network support on the <%- device %>, select <span class='HelpTextOption'><%- field.meta.options[0].value %></span>. Otherwise select <span class='HelpTextOption'><%- field.meta.options[1].value %></span>.",
        },
        "Helptext.Device.Hdmi" : {
            description : "If you are not using the available HDMI inputs on the <%- device %> select <span class='HelpTextOption'><%- field.meta.options[1].value %></span>, otherwise select <span class='HelpTextOption'><%- field.meta.options[0].value %></span>."
        },
        "Helptext.Hdmi.OffInStandby" : {
            description : "Select <span class='HelpTextOption'><%- field.meta.options[0].value %></span> to turn HDMI pass through off in sleep mode for the purpose of power saving. Select <span class='HelpTextOption'><%- field.meta.options[1].value %></span> to allow HDMI pass through in sleep mode, for the purpose of continued listening to HDMI sources through a downstream device (i.e. your Television)."
        },
        "Helptext.Hdmi.CecEnable" : {
            description : "Select <span class='HelpTextOption'><%- field.meta.options[0].value %></span> to enable ARC audio and CEC volume control."
        },
        "Helptext.Hdmi.CecSourceSelectEnable" : {
            description : "Select <span class='HelpTextOption'><%- field.meta.options[0].value %></span> to enable CEC Auto Source Select and Power Control features."
        },
        "Helptext.Hdmi.eArcMode" : {
            description : "Select <span class='HelpTextOption'><%- field.meta.options[1].value %></span> to attempt an eARC connection before falling back to ARC (requires an eARC capable television). Select <span class='HelpTextOption'><%- field.meta.options[0].value %></span> to connect via ARC only."
        },
        "Helptext.Device.VerboseDeviceLogging" : {
            description : "Set to <span class='HelpTextOption'><%- field.meta.options[1].value %></span> at request of support team to enable verbose device logging. Set to <span class='HelpTextOption'><%- field.meta.options[0].value %></span>  at all other times."
        },
        "Helptext.Device.AmpModeBridged" : {
            description : "If you are using your <%- device %> to drive one pair of speakers, select <span class='HelpTextOption'><%- field.meta.options[0].value %></span>, otherwise select <span class='HelpTextOption'><%- field.meta.options[1].value %></span> if you are driving multiple pairs of speakers."
        },
        "Helptext.Device.AmpModeBiAmped" : {
            description :
            "If you are wiring your speakers for separate amplification channels (4 wires per speaker), select <span class='HelpTextOption'><%- field.meta.options[0].value %></span>, otherwise select <span class='HelpTextOption'><%- field.meta.options[1].value %></span> if you are wiring your speakers for standard stereo amplification (2 wires per speaker)."
        },
        "Helptext.Device.PreOutMode" : {
            description : "Select <span class='HelpTextOption'><%- field.meta.options[0].value %></span> if you are connecting your <%- device %> to an external amplifier. Select <span class='HelpTextOption'><%- field.meta.options[1].value %></span> if you are using the internal amplifier of your <%- device %> as a power amp for an external device."
        },
        "Helptext.Cloud.OptOut" : {
            description : "Select <span class='HelpTextOption'><%- field.meta.options[0].value %></span> to allow your <%- device %> to send usage data to Linn. This will help us decide where to allocate time for new features or improvements.",
        },
        "Helptext.Device.CrashReports" : {
            description : "Select <span class='HelpTextOption'><%- field.meta.options[0].value %></span> to allow your <%- device %> to send crash data to Linn in the event of a failure. This will help us in diagnosing and fixing existing issues.",
        },
        "Helptext.Product.Name" : {
            description : "Enter a name for the <%- device %>. This will appear in the device name shown in services such as Spotify Connect."
        },
        "Helptext.Device.StartupMode" : {
            description : "If you wish the <%- device %> to start in sleep mode when the device powers on, select <span class='HelpTextOption'><%- field.meta.options[0].value %></span>. Otherwise select <span class='HelpTextOption'><%- field.meta.options[1].value %></span>.",
        },
        "Helptext.Device.StartupSource" : {
            description : "Set the specific source the <%- device %> selects when brought out of sleep.</span>",
        },
        "Helptext.Kustom.AutoStandby": {
            description: "Select <span class='HelpTextOption'><%- field.meta.options[0].value %></span> to keep a Streamer active when all its Zones are in Standby. This is intended for use with Control4 systems.",
        },
        "Helptext.Delay.Value" : {
            description : "Delay (in milliseconds) for a source that has Delay Mode set to Video Sync.</span>",
        },
        "Helptext.Analogue.Exakt.Dual.Fronts" : {
            description : "When playing stereo material, enable or disable dual output when a pair of front speakers are connected to, and configured on, both the analogue outputs and Exakt. NOTE: Analogue and Exakt outputs will be out of sync."
        },
        "Helptext.Display.AutoBrightness" : {
            description : "Select <span class='HelpTextOption'><%- field.meta.options[0].value %></span> to enable the brightness of the <%- device %> front panel display to be set automatically based on ambient light levels."
        },
        "Helptext.Display.Brightness" : {
            description : "Enter a number from 0 (Off) to 100 (Full Brightness) for setting the brightness of the <%- device %> front panel display"
        },
        "Helptext.Display.Timeout" : {
            description : "Select how long you would like the <%- device %> front panel display to stay visible before turning off"
        },
        "Helptext.Display.FpButtons" : {
            description : "If you are not using the front panel buttons on the <%- device %> select <span class='HelpTextOption'><%- field.meta.options[1].value %></span>, otherwise select <span class='HelpTextOption'><%- field.meta.options[0].value %></span>"
        },
        "Helptext.Display.Orientation" : {
            description : "<% if (device=='Kiko DSM') { %>" +
            "Select <span class='HelpTextOption'><%- field.meta.options[0].value %></span> to change the orientation of the <%- device %> front panel display to the vertical position. Select <span class='HelpTextOption'><%- field.meta.options[1].value %></span> to change the orientation of the <%- device %> front panel display to the horizontal position. This setting is only required if the display orientation does not change automatically when the device is rotated." +
            "<% } else  { %> " +
            "Select <span class='HelpTextOption'><%- field.meta.options[0].value %></span> to flip the orientation of the <%- device %> front panel display upside down. " +
            "<% } %>"
        },
        "Helptext.Display.ScrollText" : {
            description : "Select <span class='HelpTextOption'><%- field.meta.options[0].value %></span> to enable track metadata text (title, artist, and album) to scroll on the <%- device %> front panel display when the current track changes."
        },
        "Helptext.Display.Proximity" : {
            description : "Select <span class='HelpTextOption'><%- field.meta.options[0].value %></span> to wake the front panel display when you come within close proximity of the <%- device %>."
        },
        "Helptext.Display.DiscreteMode" : {
            description : "Select <span class='HelpTextOption'><%- field.meta.options[0].value %></span> to operate the front panel display on the <%- device %> in discrete mode (limited interruption)."
        },
        "Helptext.Display.LedColor" : {
            description : "Select the desired behavior for the power indicator LED(s)"
        },
        "Helptext.Display.SleepMode" : {
            description : "Select <span class='HelpTextOption'><%- field.meta.options[0].value %></span> if you want the front panel display to sleep after 10 seconds of inactivity (scrolling text will complete if Scroll Text on Track Change is enabled)."
        },
        "Helptext.Display.ShowUpdates" : {
            description : "If you wish to be notified when a firmware update is available, select <span class='HelpTextOption'><%- field.meta.options[0].value %></span>. Otherwise, select <span class='HelpTextOption'><%- field.meta.options[1].value %></span>."
        },
        "Helptext.Display.Led" : {
              description : "Select <span class='HelpTextOption'><%- field.meta.options[0].value %></span> to turn the power indicator LED(s) on when in active mode. Select <span class='HelpTextOption'><%- field.meta.options[1].value %></span> to turn the power indicator LED(s) off when in active mode."
        },
        "Helptext.Display.LedOffInSleep" : {
              description : "Select <span class='HelpTextOption'><%- field.meta.options[0].value %></span> to turn the power indicator LED(s) off when in sleep mode. Select <span class='HelpTextOption'><%- field.meta.options[1].value %></span> to dim the LED when in sleep mode."
        },
        "Helptext.LinnAmp.VolumeControl" : {
            description : "Select whether volume is controlled by the ALSA driver or by an external Linn amplifier connected over RS232. Changing this requires a reboot."
        },
        "Helptext.LinnAmp.SerialPort" : {
            description : "Serial port the Linn amplifier is connected to (e.g. /dev/ttyUSB0). Changing this requires a reboot."
        },
        "Helptext.LinnAmp.Model" : {
            description : "Model of the detected Linn amplifier."
        },
        "Helptext.LinnAmp.Bass" : {
            description : "Bass adjustment of the Linn amplifier, from -7 to +7. Changing this requires a reboot."
        },
        "Helptext.LinnAmp.Treble" : {
            description : "Treble adjustment of the Linn amplifier, from -7 to +7. Changing this requires a reboot."
        },
        "Helptext.LinnAmp.Serial" : {
            description : "Serial number of the detected Linn amplifier."
        },
        "Helptext.Sender.Enabled" : {
            description : "If you wish to allow other Songcast receivers in your home to listen to the <%- device %> select <span class='HelpTextOption'><%- field.meta.options[1].value %></span>. Otherwise select <span class='HelpTextOption'><%- field.meta.options[0].value %></span>.",
        },
        "Helptext.Sender.Mode" : {
            description : "Only select <span class='HelpTextOption'><%- field.meta.options[0].value %></span> if your network is capable of handling multicast."
        },
        "Helptext.Sender.Channel" : {
            description : "Only change this setting if you have a channel conflict between two Songcast sender devices in your home.<br/><br/>This setting is only relevant if <span class='HelpTextOption'>Output Mode</span> is set to <span class='HelpTextOption'>Multicast</span>."
        },
        "Helptext.Sender.Preset" : {
            description : "If you wish the <%- device %> (Songcast sender) to be selectable as a numeric preset from the handset, set the preset number.",
            options : "0 (Disabled), 1 to 9999 (Enabled)",
            "default" : "0 (Disabled)"
        },
        "Helptext.StreamingService.Enabled" : {
            description : "Enables the <%- field.helptextparams.service %> service on your <%- device %>. Go to <a href='<%- field.helptextparams.servicelinkuri %>' target='_blank'><%- field.helptextparams.servicelinktitle %></a> for more information."
        },
        "Helptext.StreamingService.SoundQuality" : {
            description : "Select the maximum streaming audio quality setting for <%- field.helptextparams.service %> playback. The higher the quality selected will require more internet bandwith. Select lower quality options if you are having problems with dropouts during playback."
        },
        "Helptext.StreamingService.UserName" : {
            description : "Enter your <%- field.helptextparams.service %> username for accessing your <%- field.helptextparams.service %> account on your <%- device %>. Go to <a href='<%- field.helptextparams.servicelinkuri %>' target='_blank'><%- field.helptextparams.servicelinktitle %></a> for more information."
        },
        "Helptext.Playlist.TracksMax": {
            description: "Change the maximum number of tracks allowed in a Playlist.",
            options: "<%- field.meta.min %> (min) to <%- field.meta.max %> (max)",
            "default": "1000"
        },
        "Helptext.Radio.TuneInUserName" : {
            description: "Enter your TuneIn account username for accessing your stored radio presets. Go to <a href='http://TuneIn.com/' target='_blank'>TuneIn.com</a> to setup your account. You may also select from the drop down list of Pre-defined accounts.",
        },
        "Helptext.Volume.Enabled" : {
            description : "If you wish to use the <%- device %> with an external pre amplifier, select <span class='HelpTextOption'><%- field.meta.options[1].value %></span>. Otherwise select <span class='HelpTextOption'><%- field.meta.options[0].value %></span>" +
            "<% if  (['Majik DS-I','Akurate Kontrol','Klimax DSM','Akurate DSM','Majik DSM','Kiko DSM','Sekrit DSM','Sneaky DSM','Sekrit DS-I'].indexOf(device) == -1) { %>" +
            " (Selecting <span class='HelpTextOption'><%- field.meta.options[0].value %></span> will automatically disable any <span class='HelpTextOption'>Linn Preamp</span> connection)" +
            "<% } %>."
        },
        "Helptext.Volume.Balance" : {
            description : "Set the left/right balance of the <%- device %>.",
            options: "<%- field.meta.min %> (left) to <%- field.meta.max %> (right)",
            "default" : "0 (middle)"
        },
        "Helptext.Volume.HeadphoneOffset" : {
            description : "Automatic change to volume when you switch to headphones.",
            options : "<%- Converters.MilliDbToDb(field.meta.min) %> to <%- Converters.MilliDbToDb(field.meta.max) %> in 0.5 dB steps"
        },
        "Helptext.Volume.OutputAttenuation" : {
            description : "Set the analog output attenuation level for the <%- device %> in dB. Smaller (more negative) attenuation will make the <%- device %> quieter. 0dB is the loudest setting."
        },
        "Helptext.Volume.StartupEnabled" : {
            description : "If you wish the <%- device %> to select a specific volume in dB when brought out of sleep, select <span class='HelpTextOption'><%- field.meta.options[0].value %></span>. Otherwise the last volume selected will remain selected out of sleep. Must be set in combination with <span class='HelpTextOption'>Startup Volume</span>."
        },
        "Helptext.Volume.StartupValue" : {
            //description : "Set the specific volume in dB the <%- device %> selects when brought out of sleep. Must be set in combination with <span class='HelpTextOption'>Startup Volume Enabled</span>."
            description : "Set the specific volume the <%- device %> selects when brought out of sleep."
        },
        "Helptext.Volume.Limit" : {
            description : "Set the maximum volume level of the <%- device %>."
        },
        "Helptext.Source.Name" : {
            description : "Enter a name for the given source, for display on a Linn control point.<br/><br/><span class='HelpTextTitle'>System Setup: </span>external input names should match the Device <span class='HelpTextOption'>Name</span> connected to that input. All devices in a particular system should have a consistent <span class='HelpTextOption'>Room</span> setting as well."
        },
        "Helptext.Source.Visible" : {
            description : "If you wish the given source to be visible and selectable from a Linn control point, select <span class='HelpTextOption'><%- field.meta.options[1].value %></span>. Otherwise select <span class='HelpTextOption'><%- field.meta.options[0].value %></span>."
        },
        "Helptext.Source.UnityGain" : {
            description : "Select On if this source is already volume controlled. Otherwise select Off."
        },
        "Helptext.Source.InputGain" : {
            description : "Set the input gain in dB to be applied to the selected phono source."
        },
        "Helptext.Source.VolumeOffset" : {
            description : "Automatic change to volume when you switch to this source."
        },
        "Helptext.Source.InputLevel.mV" : {
            description : "Set the analog input level for the given source in mVrms. This value should match the output level of the corresponding external source."
        },
        "Helptext.Source.InputLevel" : {
            description : "Set the analog input level for the given source in Vrms. This value should match the output level of the corresponding external source."
        },
        "Helptext.Source.InputTransformer" : {
            description : "If you wish to enable the source input transformer select On. If you wish to bypass the source input transformer select Off."
        },
        "Helptext.Source.DigitalFeatures" : {
            description : "Selecting On allows SPACE optimisation to be applied, audio to be available to other devices via songcast and video sync delays to be applied. Selecting Off allows an analog bypass audio path."
        },
        "Helptext.Source.DelayMode" : {
            description : "If you require minimal latency for the given source from input to output select Minimal. If you require source synchronisation with attached Songcast receivers when used as a Songcast sender select Songcast Sync. If you require synchronisation with video, select Video Sync. The delay applied for Video Sync mode is set in the Delay field on the Device tab."
        },
        "Helptext.Spotify.DelayMode" : {
            description : "If you require minimal latency for the Spotify source from input to output select <span class='HelpTextOption'><%- field.meta.options[0].value %></span>. If you require source synchronisation with attached Songcast receivers when used as a Songcast sender select <span class='HelpTextOption'><%- field.meta.options[1].value %></span>. Selecting <span class='HelpTextOption'><%- field.meta.options[1].value %></span> will result in the player taking longer to complete Spotify operations, such as play, pause, seek, etc. It may take several seconds for changes in this value to be applied."
        },
        "Helptext.Roon.Protocol": {
            description: "If you require multiroom support with older Linn devices, choose <span class='HelpTextOption'><%- field.meta.options[1].value %></span>. In all other cases, choose <span class='HelpTextOption'><%- field.meta.options[0].value %></span>."
        },
        "Helptext.Raat.DsdEnable": {
            description: "Determines where DSD decode happens. If you are using Exakt, choose <span class='HelpTextOption'><%- field.meta.options[0].value %></span>. In all other cases, choose <span class='HelpTextOption'><%- field.meta.options[1].value %></span>."
        },
        "Helptext.Bluetooth.DelayMode": {
            description : "Select Minimal delay if you are watching video content via the Bluetooth source. Otherwise select Standard. It may take several seconds for changes in this value to be applied."
        },
        "Helptext.Handset.Input" : {
            description : "Select a source to map to the <span class='HelpTextOption'><%- field.title %></span>. Setting only applies to the <span class='HelpTextOption'>Slimline Handset</span> (REM020 Silver)."
        },
        "Helptext.Handset.StarKeyHeldMode" : {
            description : "Assign a configuration action from the list that will be invoked when you press and hold the star key on the slimline handset (REM020 Silver or REM022 Black)"
        },
        "Helptext.InputCollection" : {
            description : "Select the mode of operation for the given input."
        },


        "Helptext.Dolby.Pl2Mode" : {
            description : "Select Decode mode."
        },
        "Helptext.Dolby.DynamicRange" : {
            description : "Select Dynamic Range Compression."
        },
        "Helptext.Dolby.LfeBoost" : {
            description : "Boost LFE channel by 10dB."
        },
        "Helptext.Dolby.DialogNorm" : {
            description : "Select Dialogue Normalization mode."
        },
        "Helptext.Dolby.StereoMode" : {
            description : "Select Stereo mode."
        },
        "Helptext.Dolby.SpaceEnable" : {
            description : "Space optimisation must be disabled to use Dolby Speaker Setup."
        },
        "Helptext.Dolby.SpkConfig" : {
            description : "Select Speaker configuration."
        },
        "Helptext.Dolby.ExtSpkConfig" : {
            description : "Select Extended Speaker type."
        },
        "Helptext.Dolby.SpkSize" : {
            description : "Select Speaker size."
        },
        "Helptext.Dolby.LfeVolTrim" : {
            description : "Enter LFE volume trim."
        },
        "Helptext.Dolby.SpeakerDistance" : {
            description : "Enter distance from speaker to listening position."
        },
        "Helptext.Dolby.VolTrim" : {
            description : "Enter volume trim."
        },
        "Helptext.Dolby.TestToneEnable" : {
            description : "Turn on test tone. (Select 'Speaker Setup' source)"
        },


    };

    return {
        Init : function (aSources, aResponse) {
            return {
                sources : aSources,
                containers : CreateContainerMapping(aSources, aResponse),
                legacyContainerStructure : CreateLegacyContainerStructure(aSources),
                helpTexts : kHelpTexts,
            }
        }
    }
}();
