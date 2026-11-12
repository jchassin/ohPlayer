Controller = function() {

    var kServiceName = 'Softplayer'; //todo: support legacy tabs for proxy devices?
    var iInitialUpdateExpected = false;

    return {

        StartLongPolling: function ()
        {
            var LongPollStarted = function()
            {
                // clear reboot dialog if there
                Dialogs.CloseRebootDialog();

                // clear comms error dialog if there
                Dialogs.CloseCommsErrorDialog();

                iInitialUpdateExpected = true;
            }

            var UpdateCallback = function(aResponse)
            {
                if (aResponse) {
                    if (iInitialUpdateExpected) {

                        // Retrieve source names from JSON so that ViewModel can be
                        // initialised.
                        var sources = [];
                        for(var i = 0; i < aResponse.length; i++){
                            var key = aResponse[i].key;
                            if (/^Source\./.test(key) && /\.Name$/.test(key)) {
                                var keySplit = aResponse[i].key.split('.');
                                var srcName = keySplit[1];
                                sources.push(srcName);
                            }
                        }

                        Views.Init(kServiceName, ViewModel.Init(sources, aResponse));
                        iInitialUpdateExpected = false;
                    }

                    Views.Render(aResponse, kServiceName);
                }
            }

            function CallbackFailure()
            {
                Dialogs.ShowCommsErrorMessage();
            }

            Config.StartLongPolling(LongPollStarted, UpdateCallback, CallbackFailure);
        },

        EndLongPolling: function ()
        {
            Config.EndLongPolling();
        },

        CommandReboot: function()
        {
            Config.Reboot(function(aStringSent, aStringReceived){
              Dialogs.ShowRebootDialog();
            }, function(aStringSent){
              Dialogs.ShowCommsErrorMessage();
            });
        },

        CommandSetParameter: function(aKey, aValue, aCallbackResponse, aCallbackError)
        {
            Config.SendUpdateToServer(aKey, aValue, aCallbackResponse, aCallbackError);
        }
    }
}();
