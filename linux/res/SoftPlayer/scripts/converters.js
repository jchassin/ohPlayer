Converters = function()
{

  function RoundToFive(aValue){
      return (aValue % 5) >= 2.5 ? parseInt(aValue / 5,10) * 5 + 5 : parseInt(aValue / 5,10) * 5;
  }

  function MilliDbToDb(aValue){
      aValue = "" + aValue;
      if (aValue.indexOf(".") != -1){
          return aValue; // already Db
      }
      aValue = aValue*1;

      var value = RoundToHalf(aValue/1024);
      if (!/.*\.5$/.test(value)){
          value = value + ".0";
      }
      return value;
  }

  function DbToMilliDb(aValue){
      return RoundToHalf(aValue)*1024;
  }

  function RoundToHalf(aValue) {
     var converted = parseFloat(aValue);
     var decimal = (converted - parseInt(converted, 10));
     decimal = Math.round(decimal * 10);
     if (decimal < 0) {
          if (decimal == -5) {
              return (parseInt(converted, 10) - 0.5);
          }
          else if ((decimal > -3) || (decimal < -7)) {
              return Math.round(converted);
          }
          else {
              return (parseInt(converted, 10) - 0.5);
          }
     }
     else {
          if (decimal == 5) {
              return (parseInt(converted, 10) + 0.5);
          }
          else if ((decimal < 3) || (decimal > 7)) {
              return Math.round(converted);
          }
          else {
              return (parseInt(converted, 10) + 0.5);
          }
     }
  }

  function MilliDbToDbEighth(aValue){
      aValue = "" + aValue;
      if (aValue.indexOf(".") != -1){
          return aValue; // already Db
      }
      aValue = aValue*1;

      var value = RoundToEighth(aValue/1024);

      var converted = parseFloat(value);
      var decimal = (converted - parseInt(converted, 10));
      decimal = Math.round(decimal * 1000);
      if (decimal == 0){
          value = value + ".0";
      }
      return value;
  }

  function DbToMilliDbEighth(aValue){
      return RoundToEighth(aValue)*1024;
  }

  function RoundToEighth(aValue) {
     var converted = parseFloat(aValue);
     var decimal = (converted - parseInt(converted, 10));
     decimal = Math.round(decimal * 1000);

     if (decimal < 63) {                              // 0/8 = 0
        return (parseInt(converted, 10)+0);
     }
     else if ( (decimal >= 63) && (decimal < 187) ) { // 1/8 = 125
        return (parseInt(converted, 10)+0.125);
     }
     else if ( (decimal >= 187) && (decimal < 312) ) { // 2/8 = 250
        return (parseInt(converted, 10)+0.25);
     }
     else if ( (decimal >= 312) && (decimal < 437) ) { // 3/8 = 375
        return (parseInt(converted, 10)+0.375);
     }
     else if ( (decimal >= 437) && (decimal < 562) ) { // 4/8 = 500
        return (parseInt(converted, 10)+0.5);
     }
     else if ( (decimal >= 562) && (decimal < 687) ) { // 5/8 = 625
        return (parseInt(converted, 10)+0.625);
     }
     else if ( (decimal >= 687) && (decimal < 812) ) { // 6/8 = 750
        return (parseInt(converted, 10)+0.750);
     }
     else if ( (decimal >= 812) && (decimal < 937) ) { // 7/8 = 875
        return (parseInt(converted, 10)+0.875);
     }
     else if (decimal >= 937) {                        // 8/8 = 1000
        return (parseInt(converted, 10)+1);
     }
     else {
        return 0;
     }
  }

  function HundredsToHundredths(aValue){
    var hundredsFloat = parseFloat(aValue);
    var hundredths = hundredsFloat / 100;
    hundredths = Math.round(hundredths * 100) / 100; // Round in case a float was passed in as aValue.

    var ones = aValue % 10;
    var tens = aValue % 100;
    if (tens == 0 && ones == 0) {
      hundredths = hundredths + ".00";
    }
    else if (tens > 0 && ones == 0) {
      hundredths = hundredths + "0";
    }

    return hundredths;
  }

  function HundredthsToHundreds(aValue){
    var hundredths = (aValue * 100).toFixed(2);
    hundredths = parseFloat(hundredths);
    hundredths = Math.round(hundredths);
    return hundredths;
  }

  function MultiplyThousand(aValue){
    return aValue * 1000;
  }

  function DivideThousand(aValue){
    return aValue / 1000;
  }

  function MultiplyHundredThousand(aValue){
    return aValue * 1000000;
  }

  function DivideHundredThousand(aValue){
    return aValue / 1000000;
  }

  return {
    MilliDbToDb : function(aValue, aConvertBack){
      if (aConvertBack){
        return DbToMilliDb(aValue);
      }
      return MilliDbToDb(aValue);
    },
    MilliDbToDbEighth : function(aValue, aConvertBack){
      if (aConvertBack){
        return DbToMilliDbEighth(aValue);
      }
      return MilliDbToDbEighth(aValue);
    },
    RoundToFive : function(aValue, aConvertBack){
      if (aConvertBack){
        return RoundToFive(aValue)+"";
      }
      return RoundToFive(aValue*1);
    },
    HundredsToHundredths : function(aValue, aConvertBack){
      if (aConvertBack) {
        return HundredthsToHundreds(aValue);
      }
      return HundredsToHundredths(aValue);
    },
    DivideThousand : function(aValue, aConvertBack){
      if (aConvertBack) {
        return MultiplyThousand(aValue);
      }
      return DivideThousand(aValue);
    },
    DivideHundredThousand : function(aValue, aConvertBack){
      if (aConvertBack) {
        return MultiplyHundredThousand(aValue);
      }
      return DivideHundredThousand(aValue);
    },
  }

}();
