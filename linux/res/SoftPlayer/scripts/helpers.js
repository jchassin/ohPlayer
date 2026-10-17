/* device type recognition */
var isIPhone = false;
var isIPad = false;
var isAndroid = false;
if (navigator && navigator.userAgent){
    var agent = navigator.userAgent.toLowerCase();
    isIPhone = (/ipod/.test(agent) ||
               /iphone/.test(agent));
    isIPad = (/ipad/.test(agent));
    isAndroid = (/android/i.test(agent));
}

var hasTouchSupport = (function(){
    if("createTouch" in document){ // True on the iPhone
        return true;
    }
    try{
        var event = document.createEvent("TouchEvent"); // Should throw an error if not supported
        return !!event.initTouchEvent; // Check for existance of initialization method
    }catch(error){
        return false;
    }
}());

function isObject (item) {
  return (typeof item === "object" && !Array.isArray(item) && item !== null);
}

/* array helper */
Array.prototype.remove = function(from, to) {
  var rest = this.slice((to || from) + 1 || this.length);
  this.length = from < 0 ? this.length + from : from;
  return this.push.apply(this, rest);
};

Array.prototype.contains = function(aValue){
    return $.inArray(aValue, this) != -1;
}
/* debug console */

if (typeof console == "undefined") {
    window["console"] = {"log":function(){}};
}

var debugEnabled = true;// /debug=true/i.test(location.search);

var debug = {
    log : function(message){
        if (debugEnabled){
            var debugCallback = window["debugccallback"];
            if (debugCallback){
                debugCallback(message);
            }
        }
        console.log(message);
    },
    stop: function(){
        debugEnabled = false;
    },
    start: function(){
        debugEnabled = true;
    }
}

/* OOP Helper */
Function.prototype.inheritsFrom = function( parentClassOrObject ){
	if ( parentClassOrObject.constructor == Function )
	{
		//Normal Inheritance
		this.prototype = new parentClassOrObject;
		this.prototype.constructor = this;
		this.prototype.superclass = parentClassOrObject.prototype;
	}
	else
	{
		//Pure Virtual Inheritance
		this.prototype = parentClassOrObject;
		this.prototype.constructor = this;
		this.prototype.superclass = parentClassOrObject;
	}
	return this;
}

function StripIllegalCharacters(aString){
    return aString.replace(/[^a-zA-Z0-9]/g, '');
}

function SoftwareVersionPretty(aString) {
    var swSplit = aString.split('.');
    var newVersion = aString;
    if (swSplit.length == 3) {
        switch (swSplit[0]) {
            case "1": {
                newVersion = "Auskerry " + swSplit[1] + " (" + aString + ")";
                break;
            }
            case "2": {
                newVersion = "Bute " + swSplit[1] + " (" + aString + ")";
                break;
            }
            case "3": {
                newVersion = "Cara " + swSplit[1] + " (" + aString + ")";
                break;
            }
            case "4": {
                newVersion = "Davaar " + swSplit[1] + " (" + aString + ")";
                break;
            }
            default: {break;}
        }
    }
    return newVersion;
}

/* base64 decoding */

 var keyStr = "ABCDEFGHIJKLMNOP" +
               "QRSTUVWXYZabcdef" +
               "ghijklmnopqrstuv" +
               "wxyz0123456789+/" +
               "=";

  function decode64(input) {
	 var result = [];
	 if (input && input.length){
		 var output = [];
		 var chr1, chr2, chr3 = "";
		 var enc1, enc2, enc3, enc4 = "";
		 var i = 0;

		 // remove all characters that are not A-Z, a-z, 0-9, +, /, or =
		 var base64test = /[^A-Za-z0-9\+\/\=]/g;
		 if (base64test.exec(input)) {
			alert("There were invalid base64 characters in the input text.\n" +
				  "Valid base64 characters are A-Z, a-z, 0-9, '+', '/',and '='\n" +
				  "Expect errors in decoding.");
		 }
		 input = input.replace(/[^A-Za-z0-9\+\/\=]/g, "");

		 do {
			enc1 = keyStr.indexOf(input.charAt(i++));
			enc2 = keyStr.indexOf(input.charAt(i++));
			enc3 = keyStr.indexOf(input.charAt(i++));
			enc4 = keyStr.indexOf(input.charAt(i++));

			chr1 = (enc1 << 2) | (enc2 >> 4);
			chr2 = ((enc2 & 15) << 4) | (enc3 >> 2);
			chr3 = ((enc3 & 3) << 6) | enc4;

			output[output.length] = chr1;

			if (enc3 != 64) {
			output[output.length] = chr2;
			}
			if (enc4 != 64) {
			output[output.length] = chr3;
			}

			chr1 = chr2 = chr3 = "";
			enc1 = enc2 = enc3 = enc4 = "";

		 } while (i < input.length);

		 var j=0;
		 for (var i = 0; i < output.length; i += 4, ++j){
			result[j] = getUIntAt(output, i);
		 }
	 }
	 return result;
  }

 function getUIntAt ( arr, offs ) {
	return (arr[offs+0] << 24) +
	(arr[offs+1] << 16) +
	(arr[offs+2] << 8) +
	arr[offs+3] >>> 0;
}

function pad(num, size) {
    var s = "000000000" + num;
    return s.substr(s.length-size);
}

function GetQueryString(){return function(a){if(a=="")return{};var b={};for(var i=0;i<a.length;++i){var p=a[i].split('=');b[p[0]]=decodeURIComponent(p[1].replace(/\+/g," "))}return b}(window.location.search.substr(1).split('&'))}
