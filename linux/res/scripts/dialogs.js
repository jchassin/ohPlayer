Dialogs = function($) {

    function GetHelpText(aParameter, aErrorInfoOnly, aCurrentValue)
    {
        var kDescriptionHeader = "<span class='HelpTextDescription'>";
        var kDescriptionFooter = "</span><hr/>";
        var kCurrentValueHeader = "<span class='HelpTextTitle'>Current: </span><span class='HelpTextOption'>";
        var kCurrentValueFooter = "</span><br/>";
        var kOptionsHeader = "<span class='HelpTextTitle'>Options: </span><span class='HelpTextOption'>";
        var kOptionsFooter = "</span><br/>";
        var kDefaultHeader = "<span class='HelpTextTitle'>Default: </span><span class='HelpTextOption'>";
        var kDefaultFooter = "</span><br/>";
        var kRebootHeader = "<hr/>";
        var kRebootFooter = "<br/>";
        var kToggleHeader = "<hr/>";
        var kToggleFooter = "<br/>";

        var helpTextProperties = {
            description : "Help Description Missing",
            currentValue : "",
            optionsValue : "",
            defaultValue: ""
        }

        if ("helptext" in aParameter){
            var templateParameters = { field: aParameter, device: aParameter.ModelName, fields:Views.AllParameters() }; // hack, pass in Views.AllParameters for templating
            var description = aParameter.helptext;
            if (isObject(aParameter.helptext) && "description" in aParameter.helptext){
              description = aParameter.helptext.description;
            }else if ("helptext" in aParameter){
              description = aParameter.helptext;
            }
            helpTextProperties.description = _.template(description)(templateParameters);
            helpTextProperties.optionsValue = GetOptionsValue(aParameter, templateParameters);
            helpTextProperties.defaultValue = GetDefaultValue(aParameter, templateParameters);
        }

        var text = $("<span class='HelpTextComplete'></span>");
        // help description
        if (helpTextProperties.description && !aErrorInfoOnly) {
            text.append(kDescriptionHeader + helpTextProperties.description + kDescriptionFooter);
        }
        // current value (may be blank)
        text.append(kCurrentValueHeader + aCurrentValue + kCurrentValueFooter);
        // options
        if (helpTextProperties.optionsValue) {
            text.append(kOptionsHeader + helpTextProperties.optionsValue + kOptionsFooter);
        }
        // default (may be blank)
        if (!aErrorInfoOnly) {
            text.append(kDefaultHeader + helpTextProperties.defaultValue + kDefaultFooter);
        }
        // reboot message
        if (aParameter.RequiresReboot && !aErrorInfoOnly) {
            text.append(kRebootHeader + "<span class='HelpTextReboot'>A reboot is required for this setting to take effect.</span>" + kRebootFooter);
        }
        // toggle message
        if (aParameter.IsComboBox && !aErrorInfoOnly) {
            var toggleIcon = $("<span class='SmallIcon'></span>");
            var toggleIconRef = $("<a href='#' title='Toggle (User-defined | Pre-defined)'></a>").appendTo(toggleIcon);
            $("<div class='ToggleIconHelp' />").appendTo(toggleIconRef);

            text.append(kToggleHeader + "<span class='HelpTextDescription'>Click the Toggle button </span>");
            text.append(toggleIcon);
            text.append("<span class='HelpTextDescription'> to change between User-defined and Pre-defined.</span>" + kToggleFooter);
        }
        return text;
    }

    function GetOptionsValue(aParameter, aTemplateParameters) {
        if (isObject(aParameter.helptext) && "options" in aParameter.helptext) {
           return _.template(aParameter.helptext.options)(aTemplateParameters);
        }else{
           switch (aParameter.type.toLowerCase()) {
             case "text":
               var maxlength = 20;
               if (aParameter.meta && aParameter.meta.maxlength) {
                 maxlength = aParameter.meta.maxlength;
               }
               return "Maximum of  " + maxlength + " characters, can not be blank.";
             case "numeric":
                 var min = 0, max=0;
                 if (aParameter.meta && aParameter.meta.min){
                   min = aParameter.meta.min;
                 }
                 if (aParameter.meta && aParameter.meta.max){
                   max = aParameter.meta.max;
                 }
                 return "Minimum of " + min + ", Maximum of " + max;
             case "choice":
             case "bool":
             case "combo":
                 var optionsText = "";
                 $(aParameter.meta.options).each(function(idx,option) {
                   if (idx != 0){
                      optionsText += ", ";
                   }
                   optionsText += option.value;
                 });
                 return optionsText;
             default:
                 console.log("unknown parameter type: " + aParameter.type);
           }
        }
    }

    function GetDefaultValue(aParameter, aTemplateParameters){
      if (isObject(aParameter.helptext) && "default" in aParameter.helptext) {
       return _.template(aParameter.helptext.default)(aTemplateParameters);
      }else{
        // todo: check this is the correct attribute returned by the device
        if ("meta" in aParameter && "default" in aParameter.meta){
          var defaultValue = aParameter.meta.default;
          if ("type" in aParameter && aParameter.type == "choice" && "options" in aParameter.meta){
              var valueText = "";
              $(aParameter.meta.options).each(function(idx, option){
                if ("id" in option && option.id == defaultValue){
                  valueText = option.value;
                  return false;
                }
                return true;
              });
              return (valueText ? valueText : defaultValue);
          }
          else {
              // This is not a "choice" type, so defaultValue is the correct value to report and does not need to be mapped to a string.
              return defaultValue;
          }
        }else{
          return defaultValue;
        }
      }
      return "";
    }

    return {


      ShowHelpDialog : function (aParameter, aCurrentValue) {
          var helpDialog = $("<div id='dialogHelpText'></div>");
          helpDialog
              .html(GetHelpText(aParameter, false, aCurrentValue))
              .dialog({
                  buttons: { "Close": function() { $(this).dialog('close'); } },
                  width: 400,
                  resizable: false,
                  modal: true,
                  autoOpen: false,
                  title: aParameter.DisplayName
              });
          helpDialog.dialog('open');
          helpDialog.on('dialogclose', () => { helpDialog.dialog('destroy')});
      },

      ShowSetParameterErrorMessage : function (aParameter, aMessage, aCurrentValue) {
          var desc = aMessage.split("errorDescription: ");
          var message = (desc.length > 1 ? (desc[1] + ".<br>") : "");
          var errorDialog = $('<div id="dialog-parameter-error"></div>')
              .html($("<span class='HelpTextDescription'>" + message + "Please provide a valid value.<br/><br/></span>").append(GetHelpText(aParameter, true, aCurrentValue)))
              .dialog({
                  buttons: { "OK": function() { $(this).dialog('close'); },
                             "Show Help": function() {
                                   $(this).dialog('close');
                                   ShowHelpDialog(aParameter, aCurrentValue);
                               } },
                  width: 400,
                  resizable: false,
                  modal: true,
                  autoOpen: false,
                  title: "<span class='HelpTextInvalid'>Invalid: </span>" + aParameter.DisplayName + ""
              });
          errorDialog.dialog('open');
          errorDialog.on('dialogclose', () => { errorDialog.dialog('destroy')});
      },

      ShowCommsErrorMessage : function (aModelName) {
          if (!aModelName){
            aModelName = 'Device';
          }
          var errorDialog = $('<div id="dialog-comms-error"></div>')
              .html($("<span class='HelpTextDescription'>Could not communicate with your " + aModelName + ".<br/><br/>Please ensure your " + aModelName + " is still connected to the network and has not changed IP address.<br/><br/></span>"))
              .dialog({
                  buttons: { "OK": function() { $(this).dialog('close'); } },
                  width: 400,
                  resizable: false,
                  modal: true,
                  autoOpen: false,
                  title: "Connection Error"
              });
          errorDialog.dialog('open');
          errorDialog.on('dialogclose', () => { errorDialog.dialog('destroy')});
      },

    ShowRebootConfirmDialog : function (aModelName, aParameterName){
        // show dialog
        var dialogTitle = "Reboot";
        if (aParameterName) {
            dialogTitle = "Reboot (Required)"
        }
        var dialogText = "Reboot the device now?<br/><br/>";
        if (aParameterName) {
            dialogText = "A reboot is required for the new <span class='HelpTextOption'>" + aParameterName + "</span> setting to take effect. " + dialogText;
        }
        dialogText = "<span class='HelpTextDescription'>" + dialogText + "</span>";
        var rebootConfirm = $('<div id="dialog-reboot-confirm"></div>')
            .html(dialogText)
            .dialog({
                buttons: { "Yes": function() {
                                 rebootConfirm.dialog('close');
                                 Controller.CommandReboot();
                             },
                           "No": function() {
                              rebootConfirm.dialog('close');
                             } },
                width: 400,
                resizable: false,
                modal: true,
                autoOpen: false,
                title: dialogTitle
            });
        rebootConfirm.dialog('open');
        rebootConfirm.on('dialogclose', () => { rebootConfirm.dialog('destroy')});
    },

    ShowRebootDialog : function()
    {
        if ($("#dialogReboot").length) {
          return;
        }

        var rebootDialog = $("<div id='dialogReboot'></div>");
        rebootDialog
                   .html("Device is Rebooting...")
                   .dialog({
                       buttons: { "OK": function() {
                                      $(this).dialog('close');
                                  }},
                       width: 400,
                       resizable: false,
                       modal: true,
                       autoOpen: false,
                       title: "Reboot"
                   });
        rebootDialog.dialog('open');
        rebootDialog.on('dialogclose', () => { rebootDialog.dialog('destroy')});
    },

    CloseRebootDialog : function()
    {
      if ($("#dialogReboot").length) {
        $("#dialogReboot").dialog('close');
      }
    },

    CloseCommsErrorDialog : function(){
      if ($("#dialog-comms-error").length) {
        $("#dialog-comms-error").dialog('close');
      }
    }

  }

}(jQuery);
