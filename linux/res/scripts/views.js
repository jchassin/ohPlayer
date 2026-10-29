Views = function($){

    // jquery based rendering - todo: to be replaced by ReactJs

    var kHeaderPrefix = "Header_";
    var kBodyPrefix = "Body_";
    var kSelectSelectedAttribute = " selected='selected'";
    var kComboBoxToggleOption = "User-defined...";
    var kNoPreamp = "None";

    var iProxyPreamp;
    var iProxyCd;
    var iModelName;
    var iModelImage;
    var iModelLink;
    var iLogoRefreshRequired;
    var iTitleRefreshRequired;

    var iFieldsLookup = { };
    var iHelptexts = { };
    var iParametersLookup = { };

    function SetLogo(aImageUri, aLinkUri, aTitle)
    {
      $("<a><img src='" + aImageUri + "'/></a>")
          .attr({href: aLinkUri,
                 title: aTitle,
                 target: '_blank'})
          .appendTo($("#logo").empty());
    }

    function CreateRebootButton()
    {
      if ($('#reboot-button').length == 0)
      {
        var rebootButton = $("<button type='button' id='reboot-button' class='ui-button ui-widget ui-state-default ui-corner-all reboot-button'/>");
        rebootButton.append("<span class='RebootIcon'>");
        rebootButton.append("<span class='ui-button-text'>Reboot</span>");
        rebootButton.attr("title", "Reboot");
        rebootButton.click(function(){
          Dialogs.ShowRebootConfirmDialog();
        });
        $("#header .clear").before(rebootButton);
      }
    }

    function SetTitle(aModelName)
    {
        var title = aModelName + " Configuration";
        ($("title").empty()).append(title);
        ($("#headertitle").empty()).append(title);

    }

    function FindKey(aKey, aFieldJson){
      var value = null;
      $(aFieldJson).each(function(aIdx, field){
        if (field.key == aKey){
          value = field.value;
          return false;
        }
        return true;
      });
      return value;
    }

    function CreateContainerHtml(aService) {

        var width = (isIPad ? '1000px' : '800px')
        $("#container").css('max-width', width);

        var lnkTabServiceLength = 0;

        if (lnkTabServiceLength == 0 && $("#tab" + aService).length == 0) {
            var tabName = aService;
            if (aService == "Cd") {
                tabName = "Disc Player";
            }
            var tab = $("<div id='tab" + aService + "'><ul id='header" + aService + "' class='header'></ul><div id='body" + aService + "' class='body'></div></div>");

            $("#tabs").append(tab);
        }
    }

    function PreprocessViewModel(aViewModel){
       var legacyContainerMapping = { };
       var fields = [];
       var topLevelSortOrders = { };
       var topLevelTabIndex = 0;
       $(aViewModel.legacyContainerStructure).each(function(idx, container){
          var topLevelTab = container.mapping.Collection ? container.mapping.Collection : container.mapping.Target;
          var tabIndex;
          if (topLevelTab in topLevelSortOrders) {
            tabIndex = topLevelSortOrders[topLevelTab];
          }else{
            tabIndex = ++topLevelTabIndex;
            topLevelSortOrders[topLevelTab] = tabIndex;
          }
          container.mapping.TabOrder = tabIndex;
          container.mapping.SortOrder = idx;
          legacyContainerMapping[container.key] = container.mapping;
       });

       $(aViewModel.containers).each(function (aContainerIdx, container){
         $(container.fields).each(function(aFieldIdx, field){
           var legacyContainer = legacyContainerMapping[container.containerId];
           field.Container = legacyContainer;
           field.SortOrder = aFieldIdx;
           fields[fields.length] = field;
         });
       });

       var fieldsLookup = { };
       $(fields).each(function(aIdx, container){
           $(container).each(function(aIdxVM, fieldVM){
             fieldsLookup[fieldVM.key] = fieldVM;
           });
       });

       iFieldsLookup = fieldsLookup;
       iHelptexts = aViewModel.helpTexts;
    }

    function MapFieldsToLegacyParameters(aFieldJson, aServiceName, aModelName)
    {
      var parameters = [ ];
      if (iFieldsLookup && iHelptexts){
        $(aFieldJson).each(function (aContainerIdx, fieldData){
             if ("key" in fieldData && fieldData.key in iFieldsLookup) {
               var fieldVM = iFieldsLookup[fieldData.key];
               if (fieldVM.Container){ // ensure we have a container to put it in
                     var parameter = fieldData; // set initial attibutes to be fieldData
                     $.extend(parameter, fieldVM); // and merge in the VM overrides
                     if (!("meta" in parameter)){
                       parameter.meta = { };
                     }
                     // replace by a lookup if appropriate
                     if (parameter.helptext && parameter.helptext in iHelptexts){
                         parameter.helptext = iHelptexts[parameter.helptext]; // lookup helptext
                     }

                     switch (parameter.type.toLowerCase()) {
                       case "numeric":
                         parameter.Type = "INTEGER";
                         if ("min" in parameter.meta){
                           parameter.MinValue = parameter.meta.min;
                         }
                         if ("max" in parameter.meta){
                           parameter.MaxValue = parameter.meta.max;
                         }
                         break;
                       case "bool":
                         parameter.Type = "ENUM";
                         var allowedValues = [];
                         if (!("options" in parameter.meta)){
                           parameter.meta.options = [{id:"1", value: "Yes"},{id:"0", value:"No" }];
                         }

                         $(parameter.meta.options).each(function(idx, option){
                           allowedValues[allowedValues.length] = {text:option.value, value:option.id};
                         });
                         parameter.AllowedValues = allowedValues;
                         break;
                       case "choice":
                       case "combo":
                             parameter.Type = "ENUM";
                             var allowedValues = [];
                             if ("options" in parameter.meta){
                               $(parameter.meta.options).each(function(idx, option){
                                 allowedValues[allowedValues.length] = {text:option.value, value:option.id};
                               });
                             }
                             parameter.AllowedValues = allowedValues;
                             break;
                       case "text":
                         parameter.Type = "STRING";
                         if ("maxlength" in parameter.meta){
                           parameter.MaxLength = parameter.meta.maxlength;
                         }
                         break;
                       case "rebootbutton":
                         parameter.Type = "REBOOTBUTTON";
                         break;
                       case "link":
                         parameter.Type = "LINK";
                         parameter.LinkTitle = parameter.linktitle;
                         parameter.LinkText = parameter.linktext;
                       default:
                        parameter.Type = "type" in parameter ? parameter.type : "LABEL";
                        break;
                     }

                     var name = parameter.key;
                     if ("name" in parameter){
                       name = parameter.name;
                     }else if ("title" in parameter){
                       name = parameter.title;
                     }
                     parameter.Name = name;

                     // append legacy rendering attributes
                     parameter.Target = parameter.Container.Target;
                     if (parameter.Container.Collection){
                       parameter.Collection = parameter.Container.Collection;
                     }
                     parameter.Grouping = "" + parameter.Target;
                     if (parameter.Collection) {
                        parameter.Grouping = parameter.Collection;
                     }

                     parameter.IsComboBox = parameter.type == "combo";
                     parameter.hidden = "hidden" in parameter ? parameter.hidden : false;
                     parameter.DisplayName = "title" in parameter ? parameter.title : parameter.Name;
                     parameter.ForceRefresh = false;
                     parameter.RequiresReboot = "info" in parameter && "reboot-required" in parameter.info && parameter.info["reboot-required"];
                     if ("valueConverter" in parameter){
                       parameter.ForceRefresh = true; //TODO: might be able to do away with this?
                       parameter.CoerceValueFunction = parameter.valueConverter;
                       if (parameter.type.toLowerCase() == "numeric"){
                         parameter.value = parameter.valueConverter(parameter.value, false);
                         if ("MinValue" in parameter){
                           try{
                             parameter.MinValue = parameter.valueConverter(parameter.MinValue, false);
                             parameter.meta.min = parameter.MinValue;
                           }catch (e){
                             console.log("failed to convert MinValue for parameter: " + parameter.key + ", " + parameter.MinValue);
                             console.log(e);
                            }
                         }
                         if ("MaxValue" in parameter){
                           try{
                           parameter.MaxValue = parameter.valueConverter(parameter.MaxValue, false);
                           parameter.meta.max = parameter.MaxValue;
                         }catch(e){
                           console.log("failed to convert MaxValue for parameter: " + parameter.key + ", " + parameter.MaxValue);
                           console.log(e);
                          }
                         }
                         // There is no parameter.DefaultValue, so only need to convert parameter.meta.default.
                         if ("default" in parameter.meta){
                           try{
                             parameter.meta.default = parameter.valueConverter(parameter.meta.default, false);
                           }catch (e){
                             console.log("failed to convert meta.default for parameter: " + parameter.key + ", " + parameter.meta.default);
                             console.log(e);
                           }
                         }
                       }
                       else if (parameter.type.toLowerCase() == "choice"){
                         // "choice" type shouldn't convert parameter.value, so leave it up to individual types to decide what to do, with an "else" block for previous default behaviour.
                         // A choice requiring a valueConverter converts all parameter.options[x].meta.value fields.
                         if ("meta" in parameter) {
                           if ("options" in parameter.meta) {
                             for (var i = 0; i < parameter.meta.options.length; i++){
                               if ("value" in parameter.meta.options[i]){
                                parameter.meta.options[i].value = parameter.valueConverter(parameter.meta.options[i].value, false);
                               }
                             }
                           }
                         }
                         if ("AllowedValues" in parameter) {
                           for (var i = 0; i < parameter.AllowedValues.length; i++){
                            if ("text" in parameter.AllowedValues[i]) {
                              parameter.AllowedValues[i].text = parameter.valueConverter(parameter.AllowedValues[i].text, false);
                            }
                           }
                         }
                         // There is no parameter.DefaultValue, so only need to convert parameter.meta.default.
                         if ("default" in parameter.meta){
                          try{
                            parameter.meta.default = parameter.valueConverter(parameter.meta.default, false);
                          }catch (e){
                            console.log("failed to convert meta.default for parameter: " + parameter.key + ", " + parameter.meta.default);
                            console.log(e);
                          }
                        }
                       }
                       else {
                         parameter.value = parameter.valueConverter(parameter.value, false);
                       }
                     }

                      // append dom ids
                      var sortHeader = pad(parameter.SortOrder, 6);
                      var tabSortHeader = pad(parameter.Container.TabOrder, 6);
                      var containerSortHeader = pad(parameter.Container.SortOrder,6);

                      parameter.HeaderId = kHeaderPrefix + "_" + StripIllegalCharacters(aServiceName) + tabSortHeader + StripIllegalCharacters(parameter.Grouping);
                      parameter.BodyId = kBodyPrefix + StripIllegalCharacters(aServiceName) + tabSortHeader + StripIllegalCharacters(parameter.Grouping);
                      if (parameter.Collection) {
                          parameter.Id = parameter.HeaderId + "_" + containerSortHeader + StripIllegalCharacters(parameter.Target) + "_" + sortHeader + StripIllegalCharacters(parameter.Name);
                          parameter.CollectionId = parameter.HeaderId + "_" + containerSortHeader + StripIllegalCharacters(parameter.Target);
                      }
                      else {
                          parameter.Id = parameter.HeaderId + "_" + containerSortHeader + "_" + sortHeader + StripIllegalCharacters(parameter.Name);
                      }

                      parameter.ServiceName = aServiceName;
                      parameter.ModelName = aModelName;

                     parameters[parameters.length] = parameter;
                  }else{
                    // parameter is there for potential data template lookup but won't have a Container property so cannot be rendered as a field!
                    parameters[parameters.length] = fieldData;
                  }
             }else{
               // parameter is there for potential data template lookup but won't have a Container property so cannot be rendered as a field!
               parameters[parameters.length] = fieldData;
             }
        });
      }
      return parameters;
    }

    function RenderLegacyParameters(aFieldJson, aServiceName, aModelName)
    {
      var parameters = MapFieldsToLegacyParameters(aFieldJson, aServiceName, aModelName);

      // add parameters to a dictionary so that they are available for templating lookup
      $(parameters).each(function(aIdx, parameter){
         iParametersLookup[parameter.key] = parameter;
      });

      // expand templates
      $(parameters).each(function(aIdx, parameter){
           // expand templates and render are the last steps performed once all other parameters are found and added to iParametersLookup

           if ("DisplayName" in parameter)
           {
             parameter.DisplayName = ExpandTemplate(parameter.DisplayName, {field:parameter, device:aModelName, fields:iParametersLookup });
           }

           if ("LinkTitle" in parameter)
           {
             parameter.LinkTitle = ExpandTemplate(parameter.LinkTitle, {field:parameter, device:aModelName, fields:iParametersLookup });
           }

           if ("LinkText" in parameter)
           {
             parameter.LinkText = ExpandTemplate(parameter.LinkText, {field:parameter, device:aModelName, fields:iParametersLookup });
           }
      });

      // render everything
      RenderParameters(parameters, aServiceName);

      UpdateSectionHeadersAndFooters();

      GroupRadioPresets();
    }

    function GetRadioPresetNameInputElement(aPresetIndex){
      var parameter = iParametersLookup["Radio.Preset" + aPresetIndex + ".Name"];
      if (parameter){
        var input = $("#" + parameter.Id).find("input[parameterName='" + parameter.Name + "']");
        if (input.length > 0){
          return input;
        }
      }
      return null;
    }

    function UpdateRadioPresetGroupTitle(aPresetIndex){
      var input = GetRadioPresetNameInputElement(aPresetIndex);
      if (!input){
        return;
      }
      var title = "Preset " + aPresetIndex + " - " + input.val();
      $(".RadioPresetGroupTitle_" + aPresetIndex).each(function(){
        var expanded = $(this).data("expanded");
        $(this).empty();
        $(this).append((expanded ? " &minus; " : " + ") + " &nbsp;&nbsp;" + title);
      });
    }

    function BindRadioPresetNameInput(aPresetIndex){
      var input = GetRadioPresetNameInputElement(aPresetIndex);
      if (!input){
        return;
      }
      if (input.data("presetIndex") == aPresetIndex){
        return; // already bound
      }
      input.data("presetIndex", aPresetIndex);
      input.on("input", function(){
        UpdateRadioPresetGroupTitle(aPresetIndex);
      });
    }

    function GroupRadioPresets(){
      var numPresets = 10;
      for (var presetIdx = 1; presetIdx <= numPresets; presetIdx++){
        (function(presetIndex){
          var fields = ["Name", "Uri", "Image"];
          var params = [];
          for (var i = 0; i < fields.length; i++){
            var parameter = iParametersLookup["Radio.Preset" + presetIndex + "." + fields[i]];
            if (parameter && "Container" in parameter){
              params.push(parameter);
            }
          }
          if (params.length != fields.length){
            return;
          }

          var groupClass = "RadioPresetGroup RadioPresetGroup_" + presetIndex;
          if ($("." + groupClass.split(" ").join(".")).length > 0){
            BindRadioPresetNameInput(presetIndex);
            UpdateRadioPresetGroupTitle(presetIndex);
            return;
          }

          var collectionToggleClass = params[0].CollectionId ? "Toggle_" + params[0].CollectionId : null;
          var groupDiv = $("<div class='" + groupClass + " Expandable" + (collectionToggleClass ? " " + collectionToggleClass : "") + "'/>");
          var groupHeader = $("<h3 class='RadioPresetGroupHeader'/>").appendTo(groupDiv);
          var groupTitleRef = $("<a title='Expand' href='#' class='SectionHeader RadioPresetGroupTitle_" + presetIndex + "'></a>").appendTo(groupHeader);
          var groupBody = $("<div class='RadioPresetGroupBody RadioPresetGroupBody_" + presetIndex + " Expandable'/>").appendTo(groupDiv);

          groupTitleRef.data("presetIndex", presetIndex);
          groupTitleRef.click(function(){
            var expanded = groupTitleRef.data("expanded") ? false : true;
            groupTitleRef.data("expanded", expanded);
            groupTitleRef.attr('title', expanded ? 'Collapse' : 'Expand');
            UpdateRadioPresetGroupTitle(presetIndex);
            groupBody.toggle();
            return false;
          });

          var firstParamElement = $("#" + params[0].Id);
          if (firstParamElement.length == 0){
            return;
          }
          firstParamElement.before(groupDiv);
          for (var i = 0; i < params.length; i++){
            var fieldElement = $("#" + params[i].Id);
            fieldElement.removeClass("Expandable");
            if (collectionToggleClass){
              fieldElement.removeClass(collectionToggleClass);
            }
            groupBody.append(fieldElement);
          }

          BindRadioPresetNameInput(presetIndex);

          UpdateRadioPresetGroupTitle(presetIndex);
        })(presetIdx);
      }
    }

    function RenderParameters(aParameters, aServiceName){
      var createdNew = false;
      $(aParameters).each(function(aIdx, parameter){
        if ("Container" in parameter){ // don't render parameters that are not mapped to a container!
          if (Render(parameter)){
            createdNew = true;
          }
        }
      });

      if (createdNew){
          var header = $("#tab" + aServiceName);
          if (header.data("tabs")){
              header.data("tabs").destroy();
          }
          $("#tab" + aServiceName).tabs()
          .click(function(){
            $('html,body').scrollTop(0);
          });
      }
    }

    function Render(aParameter){
      var createdNew = false;
      var element = GetElement(aParameter)
      if (element.length == 0){
          createdNew = true;
          element = CreateElement(aParameter);
      }else{
          UpdateElement(aParameter, element);
      }
      element.toggleClass('HiddenField', aParameter.hidden);
      return createdNew;
    }

    function ExpandTemplate(aStr, aTemplateParameters)
    {
      return _.template(aStr)(aTemplateParameters);
    }

    function UpdateSectionHeadersAndFooters()
    {
      // remove and recreate headers/footers
      $('.Parameter > .SectionHeader, .Parameter > .SectionFooter').remove();

      $('.Container').each(function(idx, container){
        var currentSection = "";
        var parameters = $(container).find('.Parameter');
        parameters.each(function(idx2, parameter){
          var sectionTitle = $(parameter).data('section');
          var showHeader = $(parameter).data('showsectionheader');
          if (sectionTitle){
            if (!currentSection || currentSection != sectionTitle){
              currentSection = sectionTitle;
              if (showHeader){
                $(parameter).prepend("<div class='SectionHeader'><h3>" +  sectionTitle + "</h3></div>");
              }
            }
            if (idx2 < parameters.length - 1 && $(parameters[idx2 + 1]).data('section') != currentSection){
              $(parameter).append("<div class='SectionFooter'></div>");
            }
          }else{
            currentSection = "";
          }
        });
      });
    }

    function GetElement(aParameter){
        return $("#" + aParameter.Id);
    }

    function CreateElement(aParameter){
        var container = GetContainer(aParameter);
        if (!container){
            container = CreateContainer(aParameter);
        }
        var body = container.body;
        var newElement = CreateElementHtml(aParameter);

        var appendCollection = false;
        var newCollection = [];
        if (aParameter.Collection) {
            // seaprate collection targets
            var collection = $("#" + aParameter.CollectionId);
            if (collection.length == 0){
                newCollection = $("<div id='" + aParameter.CollectionId + "'/>");
                newCollection.append("<hr/>");
                var collectionHeader = $("<h3></h3>").click(function(){
                    var elem = collectionHeader.find("a");
                    if (elem.attr('title') == "Expand") {
                        elem.attr('title', "Collapse");
                        elem.empty();
                        elem.append(" &minus; &nbsp;&nbsp;" + aParameter.Target);
                    }
                    else {
                        elem.attr('title', "Expand");
                        elem.empty();
                        elem.append(" + &nbsp;&nbsp;" + aParameter.Target);
                    }
                    $(".Toggle_" + aParameter.CollectionId).toggle();
                    return false; // prevent the default action, e.g., following a link
                }).appendTo(newCollection);
                $("<a title='Expand' href='#' class='SectionHeader'> + &nbsp;&nbsp;" + aParameter.Target + "</a>").appendTo(collectionHeader);
                appendCollection = true;
            }
        }

        var bodyEntries = body.children();
        var groupListBody = [];
        for (var i=0; i<bodyEntries.length; i++){
            groupListBody[i] = bodyEntries[i].id;
        }
        if (appendCollection) {
            groupListBody[groupListBody.length] = aParameter.CollectionId;
        }
        groupListBody[groupListBody.length] = aParameter.Id;
        groupListBody.sort();

        var loc = $.inArray(aParameter.Id, groupListBody) + 1;
        if ((loc > 0) && (loc < groupListBody.length)) {
            // not the last entry in the array so place it alphapbetically
            $("#" + groupListBody[loc]).before(newElement);
            if (appendCollection) {
                $(newElement).before(newCollection);
            }
        }
        else {
            if (appendCollection) {
                body.append(newCollection);
            }
            body.append(newElement);
        }

        return newElement;
    }

    function CreateElementHtml(aParameter){
      var expandable = (aParameter.Collection ? "class='Expandable Toggle_" + aParameter.CollectionId + "'" : "");
      var placeholder = $("<div id='" + aParameter.Id +"'" + expandable + " class='Parameter'></div>");
      if (!("Container" in aParameter)){
        throw "parameter has no container: " + aParameter.key;
      }
      placeholder.data('showsectionheader', "ShowSectionHeader" in aParameter.Container ? aParameter.Container.ShowSectionHeader : aParameter.Container.Section);
      placeholder.data('section', aParameter.Container.Section);

        /*if (aParameter.type == "rebootbutton")
        {
          var editor = CreateElementEditor(aParameter);
          editor.addClass('ModelIcon');
          placeholder.append(editor);
        }else{*/
          if ("helptext" in aParameter){
            var helpIcon = $("<span class='SmallIcon'></span>")
                .click(function(){
                    Dialogs.ShowHelpDialog(aParameter, GetElementCurrentValue(aParameter));
                    return false; // prevent the default action, e.g., following a link
                }).appendTo(placeholder);
            var helpIconRef = $("<a href='#' title='Help'></a>").appendTo(helpIcon);
            $("<div class='HelpIcon'/>").appendTo(helpIconRef);
          }
          if (aParameter.type == "rebootbutton"){
            var imgContainer = $("<span/>");
            imgContainer.append($('<img>').attr('src', aParameter.value));
            imgContainer.attr("title", aParameter.DisplayName);
            imgContainer.addClass('ModelIcon');

            var elementDescription = $("<span class='ElementDescription'/>");
            elementDescription.append(imgContainer);
            placeholder.append(elementDescription);
            placeholder.append(CreateElementEditor(aParameter));
          }else{
            placeholder.append("<span class='ElementDescription'>" + aParameter.DisplayName + "</span>");
            placeholder.append(CreateElementEditor(aParameter));
          }
          if (aParameter.RequiresReboot){
              var rebootIcon = $("<span class='SmallIcon' title='Reboot Required'></span>")
                  .appendTo(placeholder);
              $("<div class='RebootIcon'/>").appendTo(rebootIcon);
          }
          else if (aParameter.IsComboBox) {
              var toggleIcon = $("<span class='SmallIcon'></span>")
                  .click(function(){
                      // change between drop down list and string editor
                      $(".Toggle_" + aParameter.Id).toggle();
                      return false; // prevent the default action, e.g., following a link
                  }).appendTo(placeholder);
              var toggleIconRef = $("<a href='#' title='Toggle (User-defined | Pre-defined)'></a>").appendTo(toggleIcon);
              $("<div class='ToggleIcon'/>").appendTo(toggleIconRef);
          }
        //}

        return placeholder;
    }


    function CreateElementEditor(aParameter){
        var editor;
        switch(aParameter.Type.toUpperCase()){
            case "REBOOTBUTTON":
              /*editor = $("<button type='button' class='ui-button ui-widget ui-state-default ui-corner-all reboot-button'/>");
              editor.append("<span class='RebootIcon'>");
              editor.append("<span class='ui-button-text'>Reboot</span>");
              editor.attr("title", aParameter.DisplayName);
              editor.click(function(){
                Dialogs.ShowRebootConfirmDialog();
              });*/
              break;
            case "LINK":
              editor = $("<a class='ElementLink' target='_blank'></a>");
              editor.append($("<span/>").html(aParameter.LinkText));
              editor.attr('title', aParameter.LinkTitle);
              editor.attr('href', aParameter.value);
              break;
            case "READ-ONLY-LIST":
              editor = $('<table class=\'ReadOnlyList\'/>')
              if ("listheaders" in aParameter){
                var headerRow = $('<tr/>');
                $(aParameter.listheaders).each(function(idx, header){
                  var col = $('<th/>');
                  col.html(header);
                  headerRow.append(col);
                });
                editor.append(headerRow);
              }
              $(aParameter.value).each(function(idx, row){
                var rowElem = $('<tr/>');
                $(row).each(function(idx2, col){
                  var colElem = $('<td/>');
                  colElem.html(col);
                  rowElem.append(colElem);
                });
                editor.append(rowElem);
              });
              break;
            case "URI":
            case "STRING":
            case "INTEGER":
            case "POSITIVEINTEGER":
            {
                var validation = typeof(aParameter.Validation) != "undefined" ? "validation='" + aParameter.Validation + "' " : "";
                var maxLength = typeof(aParameter.MaxLength) != "undefined" ? "maxLength='" + aParameter.MaxLength + "' " : "";
                var minValue = typeof(aParameter.MinValue) != "undefined" ? "minValue='" + aParameter.MinValue + "' " : "";
                var maxValue = typeof(aParameter.MaxValue) != "undefined" ? "maxValue='" + aParameter.MaxValue + "' " : "";
                var target = "target='" + aParameter.Target + "' ";
                var parameterName = "parameterName='" + aParameter.Name + "' ";
                var parameterId = "parameterId='" + aParameter.Id + "' ";
                var escapedValue = aParameter.value + "";
                escapedValue = escapedValue.replace(/'/g, '&apos;');
                var initialValue = "initialValue='" + escapedValue + "' ";
                var value = "value='" + escapedValue + "' ";
                var className = "class='" + aParameter.Type + (isIPad ? "_iPad" : "") + "' ";
                var serviceName = "serviceName='" + aParameter.ServiceName + "' ";
                editor = $("<input type='text' " + className + maxLength + validation + target + parameterName + parameterId + initialValue + value + minValue + maxValue + serviceName + "/>");
                editor.data('parameterJson', aParameter);
                editor.keyup(InputChanged);
                editor.change(InputUpdated);
                break;
            }
            case "ENUM":{
                editor = CreateSelect(aParameter);
                if (aParameter.IsComboBox) {
                    // enum part
                    var editor1 = $("<span class='ElementEditor' />").append(editor);
                    // string part
                    var visible = (editor.css('display') == 'none'); // enum and string should never be visible at the same time
                    var validation = typeof(aParameter.Validation) != "undefined" ? "validation='" + aParameter.Validation + "' " : "";
                    var maxLength = typeof(aParameter.MaxLength) != "undefined" ? "maxLength='" + aParameter.MaxLength + "' " : "";
                    var minValue = typeof(aParameter.MinValue) != "undefined" ? "minValue='" + aParameter.MinValue + "' " : "";
                    var maxValue = typeof(aParameter.MaxValue) != "undefined" ? "maxValue='" + aParameter.MaxValue + "' " : "";
                    var target = "target='" + aParameter.Target + "' ";
                    var parameterName = "parameterName='" + aParameter.Name + "' ";
                    var parameterId = "parameterId='" + aParameter.Id + "' ";
                    var initialValue = "initialValue='" + aParameter.value + "' ";
                    var value = "value='" + aParameter.value + "' ";
                    var className = "class='string" + (isIPad ? "_iPad" : "") + " Toggle_" + aParameter.Id + "' ";
                    var serviceName = "serviceName='" + aParameter.ServiceName + "' ";
                    editor = $("<input type='text' " + className + maxLength + validation + target + parameterName + parameterId + initialValue + value + minValue + maxValue + serviceName + "/>");
                    editor.data('parameterJson', aParameter);
                    editor.keyup(InputChanged);
                    editor.change(InputUpdated);
                    if (visible) {
                        editor.css('display', 'inline');
                    }
                    else {
                        editor.css('display', 'none');
                    }
                    var editor2 = $("<span class='ElementEditor' />").append(editor);

                    var comboEditor = $("<span class='ElementEditorCombo' />");
                    comboEditor.append(editor2);
                    comboEditor.append(editor1);
                    return comboEditor;
                }
                break;
            }
            default:
            {
                editor = $("<span>" + aParameter.value + "</span>");
                break;
            }
        }

        return $("<span class='ElementEditor' />").append(editor);
    }

    function CreateSelect(aParameter){
        var target = "target='" + aParameter.Target + "' ";
        var parameterName = "parameterName='" + aParameter.Name + "' ";
        var parameterId = "parameterId='" + aParameter.Id + "' ";
        var initialValue = "initialValue='" + aParameter.value + "' ";
        var serviceName = "serviceName='" + aParameter.ServiceName + "' ";
        var className = (aParameter.IsComboBox ? "class='selectBox Toggle_" + aParameter.Id + "'" : "class='selectBox'");
        var select = $("<select " + className + target + parameterName + parameterId + initialValue + serviceName + "/>");
        select.data('parameterJson', aParameter);
        var found = false;
        $(aParameter.AllowedValues).each(function(aIdx, aValue){
            var selected = (aParameter.value == aValue.value ? kSelectSelectedAttribute : "");
            select.append($("<option value='" + aValue.value + "'" + selected + ">" + aValue.text + "</option>"));
            if (aParameter.value == aValue.value){
                found = true;
                if (aParameter.IsComboBox) {
                    if (aValue.value != "") {
                        select.css('display', 'inline');
                    }
                    else {
                        select.css('display', 'none');
                    }
                }
            }
        });
        if (!found){
            select.append($("<option value='" + aParameter.value + "'" + kSelectSelectedAttribute + ">" + (aParameter.IsComboBox ? aParameter.value : "") + "</option>"));
            if (aParameter.IsComboBox) {
                select.css('display', 'none');
            }
        }
        select.change(SelectUpdated);
        return select;
    }

    function OnSelectChanged(elem){
        var value = elem.find("option:selected").attr('value');
        elem.toggleClass("Changed", value != elem.attr('initialValue'));
    }

    function SelectUpdated(){
        OnSelectUpdated($(this), $(this).find("option:selected").attr('value'));
    }

    function OnSelectUpdated(aElem, aNewValue){
        var initialValue = aElem.attr('initialValue');
        var target = aElem.attr('target');
        var parameterName = aElem.attr('parameterName');
        var serviceName = aElem.attr('serviceName');
        var parameterId = aElem.attr('parameterId');

        if (initialValue != aNewValue){
            var parameter = aElem.data('parameterJson');
            parameter.value = aNewValue;
            iParametersLookup[parameter.key] = parameter;
            Controller.CommandSetParameter(parameter.key,
                aNewValue,
                function(){
                    aElem.attr('initialValue', aNewValue);
                    OnSelectChanged(aElem);
                    if (parameter.RequiresReboot) {
                        Dialogs.ShowRebootConfirmDialog(parameter.ServiceName, parameter.DisplayName);
                    }
                },
                function(aSentJson, aMessage, aStatus){
                    if (aStatus && aStatus == 400) {
                        Dialogs.ShowSetParameterErrorMessage(parameter, aMessage, parameter.ModelName, aNewValue);
                    }
                    else {
                        var modelName = parameter.ModelName;
                        Dialogs.ShowCommsErrorMessage(parameter.ModelName);
                    }
                });
        }
    }

    function InputChanged(){
        return Validate($(this));
    }

    function Validate(elem){
        var newValue = elem.val();
        var initialValue = elem.attr('initialValue');
        var validation = elem.attr('validation');
        elem.toggleClass("Changed", newValue != initialValue);
        var valid = true;
        var isNumericElement = (typeof(elem.attr('minValue')) != "undefined" || typeof(elem.attr('maxValue')) != "undefined");
        if (isNumericElement){
            var minValue = elem.attr('minValue');
            var maxValue = elem.attr('maxValue');
            var numeric = newValue != "" && !isNaN(newValue);
            if (!numeric ||
               (typeof(minValue) != "undefined" && newValue * 1 < minValue) ||
               (typeof(maxValue) != "undefined" && newValue * 1 > maxValue)){
                valid = false;
            }
        }else if (typeof(validation) != "undefined"){
            var regex = new RegExp(validation);
            valid = regex.test(newValue);
        }
        elem.toggleClass("Invalid", !valid);
        return valid;
    }


    function InputUpdated(){
        var elem = $(this);
        var valid = Validate(elem);
        var initialValue = elem.attr('initialValue');
        var newValue = elem.val();

        if (initialValue != newValue && valid){
            var parameter = elem.data('parameterJson');
            var coercedValue;
            if (typeof(parameter.CoerceValueFunction) != "undefined"){
                coercedValue = parameter.CoerceValueFunction(newValue, true);
            }else{
                coercedValue = newValue;
            }
            parameter.value = coercedValue;
            iParametersLookup[parameter.key] = parameter; // udpate the lookup value here
            Controller.CommandSetParameter(parameter.key,
                                          coercedValue,
                                          function(){
                                            // might not need all this
                                            elem.attr('initialValue', newValue);
                                            Validate(elem);
                                            if (parameter.RequiresReboot) {
                                                Dialogs.ShowRebootConfirmDialog(parameter.ServiceName, parameter.DisplayName);
                                            }
                                        },
                                        function(aSentJson, aMessage, aStatus){
                                            if (aStatus && aStatus == 400) {
                                                Dialogs.ShowSetParameterErrorMessage(parameter, aMessage, parameter.ModelName, newValue);
                                                elem.toggleClass("Invalid",true);
                                            }
                                            else {
                                                var modelName = parameter.ModelName;
                                                Dialogs.ShowCommsErrorMessage(parameter.ModelName);
                                            }
                                        });
        }
    }

    function GetElementCurrentValue(aParameter, aErrorInfoOnly) {
        var input;
        switch(aParameter.Type.toUpperCase()){
            case "STRING":
            case "URI":
            case "INTEGER":
            case "POSITIVEINTEGER":
            {
                input = $("#" + aParameter.Id).find("span > input");
                break;
            }
            case "ENUM":
            {
                input = $("#" + aParameter.Id).find("select");
                break;
            }
            default: {
                return "";
            }
        }
        var currentValue = input.attr('initialValue');
        var newCurrentValue = currentValue;
        if (aParameter.AllowedValues) {
            var found = false;
            $(aParameter.AllowedValues).each(function(aIdx, aValue){
                if (aValue.value == currentValue) {
                    if (newCurrentValue != aValue.text) {
                        newCurrentValue = aValue.text;
                    }
                    found = true;
                    return false; // Value found; terminate loop.
                }
            });
            if (!found && !aParameter.IsComboBox) {
                newCurrentValue = "";
            }
        }
        if (input.attr('class').indexOf("Invalid") >= 0) {
            return "<span class='HelpTextInvalid'>" + input.val() + "</span> (Invalid)";
        }
        else if (aErrorInfoOnly) {
            return "<span class='HelpTextInvalid'>" + input.val() + "</span>";
        }
        else {
            return newCurrentValue;
        }
    }

    function UpdateElement(aParameter, aElement){
        var input;
        switch(aParameter.Type.toUpperCase()){
            case "STRING":
            case "URI":
            case "INTEGER":
            case "POSITIVEINTEGER":
            {
                input = aElement.find("span > input");
                if (aParameter.value != input.attr('initialValue')){
                    input.attr('value', aParameter.value);
                    input.attr('initialValue', aParameter.value);
                }
                input.data('parameterJson', aParameter);
                break;
            }
            case "ENUM":
            {
                input = aElement.find("select");
                input.data('parameterJson', aParameter);
                var optionsChanged = false;
                var options = input.find("option");
                optionsChanged = aParameter.AllowedValues.length != options.length;
                if (!optionsChanged){
                    for (var i=0;i<aParameter.AllowedValues.length;i++){
                        if ($(options[i]).attr('value') != aParameter.AllowedValues[i].value
                            || $(options[i]).text() != aParameter.AllowedValues[i].text){
                                optionsChanged = true;
                                break;
                            }
                    }
                }

                var newSelect = null;
                if (aParameter.value != input.attr('initialValue') || optionsChanged){
                    var newSelect = CreateSelect(aParameter);
                    input.replaceWith(newSelect);
                }

                if (aParameter.IsComboBox) {
                    // string part
                    input = aElement.find("span > input");
                    input.data('parameterJson', aParameter);
                    if (aParameter.value != input.attr('initialValue')){
                        input.val(aParameter.value);
                        input.attr('initialValue', aParameter.value);
                    }
                    if (newSelect) {
                        if (newSelect.css('display') == 'none') { // if ComboBox, enum and string should never be visible at the same time
                            input.css('display', 'inline');
                        }
                        else {
                            input.css('display', 'none');
                        }
                    }
                }
                break;
            }
            default: {break;}
        }
        if (input){
          Validate(input);
        }
    }

    function GetContainer(aParameter){
        var header = $("#" + aParameter.HeaderId);
        var body = $("#" + aParameter.BodyId);
        if (header.length > 0 && body.length > 0){
            return {header:header, body:body};
        }else{
            return null;
        }
    }

    function CreateContainer(aParameter){
        var newHeader = $("<li id='" + aParameter.HeaderId + "'><a href='#" + aParameter.BodyId + "'>" + aParameter.Grouping + "</a></li>");
        var newBody = $("<div id='" + aParameter.BodyId + "' class='Container'/>");

        var headerEntries = $("#header" + aParameter.ServiceName).children();
        var groupListHeader = [];
        for (var i=0; i<headerEntries.length; i++){
            groupListHeader[i] = headerEntries[i].id;
        }
        groupListHeader[groupListHeader.length] = aParameter.HeaderId;
        groupListHeader.sort();

        var bodyEntries = $("#body" + aParameter.ServiceName).children();
        var groupListBody = [];
        for (var i=0; i<bodyEntries.length; i++){
            groupListBody[i] = bodyEntries[i].id;
        }
        groupListBody[groupListBody.length] = aParameter.BodyId;
        groupListBody.sort();

        var loc = $.inArray(aParameter.HeaderId, groupListHeader) + 1;
        if ((loc > 0) && (loc < groupListHeader.length)) {
            // not the last entry in the array so place it alphapbetically
            $("#" + groupListHeader[loc]).before(newHeader);
            $("#" + groupListBody[loc]).before(newBody);
        }
        else {
            $("#header" + aParameter.ServiceName).append(newHeader);
            $("#body" + aParameter.ServiceName).append(newBody);
        }
        return {header: newHeader, body:newBody};
    }


  return {
    AllParameters : function(){
      return iParametersLookup;
    },
    Init : function(aServiceName, aViewModel){
      CreateContainerHtml(aServiceName);
      PreprocessViewModel(aViewModel);
      CreateRebootButton();
      $("#spinner").show();
      $("#container").hide();
    },
    Render : function(aFieldJson, aServiceName){
          iProxyPreamp = kNoPreamp;
          iProxyCd = kNoPreamp;
          // todo: ensure these keys are correct!
          if (!iModelName || iModelName == "Device"){
            iLogoRefreshRequired = true;
            iTitleRefreshRequired = true;
            iModelName = FindKey("About.ModelName", aFieldJson);
            if (!iModelName){
              iModelName = "Device";
            }
          }
          if (!iModelImage){
            iLogoRefreshRequired = true;
            iModelImage = FindKey("About.ModelIcon", aFieldJson);
          }
          if (!iModelLink){
            iLogoRefreshRequired = true;
            iModelLink = FindKey("About.ModelUrl", aFieldJson);
          }

          if (iModelName && iTitleRefreshRequired){
            SetTitle(iModelName);
            iTitleRefreshRequired = false;
          }
          if (iModelImage && iModelLink && iModelName && iLogoRefreshRequired){
            SetLogo(iModelImage, iModelLink, iModelName);
            iLogoRefreshRequired = false;
          }
          RenderLegacyParameters(aFieldJson, aServiceName, iModelName);
          $("#spinner").hide();
          $("#container").show();
    }
  }

}(jQuery);
