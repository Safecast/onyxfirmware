#include "screen_layout.h"
#include "safecast_wirish_types.h"
// types are: ITEM_TYPE_MENU, ITEM_TYPE_LABEL, ITEM_TYPE_SELECTNUM, ITEM_TYPE_SELECTION, ITEM_TYPE_VARLABEL, ITEM_TYPE_GRAPH

__FLASH__ screen screens_layout[SCREEN_COUNT] = {

// Screen 0 - main screen
{
 4,
 {
   { ITEM_TYPE_HEAD       , 0, 0, "CPMDEADINT"      , 255}, 
   { ITEM_TYPE_MENU       , 1, 1, "Current Readings", 4  },
   { ITEM_TYPE_MENU       , 2, 2, "Advanced"        , 30 },
   { ITEM_TYPE_MENU       , 3, 3, "Settings"        , 0  }
 }
 ,7
},

// Screen 1 - Current readings screen
{
  5,
  {
    { ITEM_TYPE_HEAD        , 0 ,  0, "CPMDEADINT", 255}, 
    { ITEM_TYPE_BIGVARLABEL , 0 , 26, "CPMDEAD"   , 255},
    { ITEM_TYPE_VARLABEL    ,104, 60, "CPMSLABEL" , 255},
    { ITEM_TYPE_BIGVARLABEL , 0 , 73, "SVREM"     , 255},
    { ITEM_TYPE_VARLABEL    , 80,104, "SVREMLABEL", 255},
  }
  ,0
},

// Screen 2 - Advanced menu
{
  6,
  {
    { ITEM_TYPE_HEAD       , 0, 0, "CPMDEADINT"     ,255 }, 
    { ITEM_TYPE_MENU       , 4, 1, "Graphs"         ,1   },
    { ITEM_TYPE_MENU       ,15, 2, "Accumulate/Avg" ,6   },
    { ITEM_TYPE_MENU       ,21, 3, "Becquerel"      ,31  },
//    { ITEM_TYPE_MENU_ACTION, 0, 4, "QR Transfer"    ,7   },
    { ITEM_TYPE_MENU_ACTION, 0, 4, "QR Tweet"       ,32  },
    { ITEM_TYPE_MENU_ACTION, 0, 5, "Audio Xfer"     ,255 }
//    { ITEM_TYPE_MENU_ACTION, 0, 7, "Audio Xfer Full",255 }
  }
  ,7
},

// Screen 3 - Settings menu
{
  5,
  {
    { ITEM_TYPE_HEAD       , 0, 0, "CPMDEADINT", 255 }, 
    { ITEM_TYPE_MENU       ,16, 1, "Interface" , 8 },
    { ITEM_TYPE_MENU       ,17, 2, "Geiger"    , 9 },
    { ITEM_TYPE_MENU       , 9, 3, "Time/Date" , 13 },
    { ITEM_TYPE_MENU       ,18, 4, "Version"   , 14 }
  }
  ,7
},


// Screen 4 - Graph display
{
  3,
  {
    { ITEM_TYPE_HEAD    , 0 , 0  , "CPMDEADINT"     , 255 }, 
    { ITEM_TYPE_GRAPH   , 4 , 110, "RECENTDATA"     , 255 },
    { ITEM_TYPE_LABEL   ,255, 112, "CPM last 2 mins", 17  }
  }
  ,3
},

//Screen 5 - Calibration Confirmation
{
  4,
  {
    { ITEM_TYPE_HEAD , 0 , 0  , "CPMDEADINT"       , 255},
    { ITEM_TYPE_MENU , 6 , 3  , "       OK       " , 255},
    { ITEM_TYPE_MENU , 3 , 4  , "     Cancel     " , 18 },
    { ITEM_TYPE_LABEL,255, 16 , "Are you sure?"    , 19 }
  }
  ,4
},

//Screen 6 - Calibration Wait 1
{
  4,
  {
    { ITEM_TYPE_LABEL,255, 32, "Waiting for 15s" , 20  },
    { ITEM_TYPE_LABEL,255, 64, "Please expose"   , 21  },
    { ITEM_TYPE_LABEL,255, 80, "to source."      , 22  },
    { ITEM_TYPE_DELAY, 60,100,"DELAYA\0 16,7"    , 255 } // 16 second delay, then go to screen 7
  } 
  ,255
},

//Screen 7 - Calibration Wait 2
{
  2,
  {
    { ITEM_TYPE_LABEL,255, 32, "Acquiring, 30s" , 23  },
    { ITEM_TYPE_DELAY, 60,100,"DELAYB\0 31,8"   , 255 } // 31 second delay
  } 
  ,255
}, 
    

//Screen 8 - Calibration
{
  8,
  {
    { ITEM_TYPE_ACTION     ,  0,   0, "CALIBRATE" , 255 },
    { ITEM_TYPE_VARNUM     , 20, 50 , "CAL1"      , 255 },
    { ITEM_TYPE_VARNUM     , 56, 50 , "CAL2"      , 255 },
    { ITEM_TYPE_VARNUM     , 74, 50 , "CAL3"      , 255 },
    { ITEM_TYPE_VARNUM     , 92, 50 , "CAL4"      , 255 },
    { ITEM_TYPE_MENU_ACTION,  0,   7, "Save:Calib", 2   },
    { ITEM_TYPE_ACTION     ,  0,   0, "CALIBRATE" , 255 },
    { ITEM_TYPE_VARLABEL   ,  0,   0, "FIXEDSV"   , 255 }
  }
  ,4
},

//Screen 9 - Set time/date
{
  4,
  {
    { ITEM_TYPE_HEAD,             0, 0, "CPMDEADINT"    , 255 }, 
    { ITEM_TYPE_MENU,            10, 1, "Set Time (UTC)", 15  },
    { ITEM_TYPE_MENU,            11, 2, "Set Date (UTC)", 16  },
    { ITEM_TYPE_MENU,            20, 3, "Set UTC Offset", 33  }
  }
  ,7
},

//Screen 10 - Set time
{
  10,
  {
    { ITEM_TYPE_HEAD       , 0 , 0 , "CPMDEADINT" , 255 }, 
    { ITEM_TYPE_VARNUM     , 10, 50, "TIMEHOUR1"  , 255 },
    { ITEM_TYPE_VARNUM     , 32, 50, "TIMEHOUR2"  , 255 },
    { ITEM_TYPE_VARNUM     , 54, 50, "TIMEMIN1"   , 255 },
    { ITEM_TYPE_VARNUM     , 76, 50, "TIMEMIN2"   , 255 },
    { ITEM_TYPE_VARNUM     , 98, 50, "TIMESEC1"   , 255 },
    { ITEM_TYPE_VARNUM     ,118, 50, "TIMESEC2"   , 255 },
    { ITEM_TYPE_MENU_ACTION,  0,  7, "Save:Time"  , 2   },
    { ITEM_TYPE_LABEL      ,  0, 20, "HH:MM:SS"   , 50  },
    { ITEM_TYPE_ACTION     ,  0,  0, "TIMESCREEN" , 255 }
  }
  ,7
},

//Screen 11 - Set date
{
  10,
  {
    { ITEM_TYPE_HEAD       , 0 , 0  , "CPMDEADINT", 255 }, 
    { ITEM_TYPE_VARNUM     , 10, 50 , "DATEMON1"  , 255 },
    { ITEM_TYPE_VARNUM     , 32, 50 , "DATEMON2"  , 255 },
    { ITEM_TYPE_VARNUM     , 54, 50 , "DATEDAY1"  , 255 },
    { ITEM_TYPE_VARNUM     , 76, 50 , "DATEDAY2"  , 255 },
    { ITEM_TYPE_VARNUM     , 98, 50 , "DATEYEAR1" , 255 },
    { ITEM_TYPE_VARNUM     ,118, 50 , "DATEYEAR2" , 255 },
    { ITEM_TYPE_MENU_ACTION,  0,   7, "Save:Date"  , 2  },
    { ITEM_TYPE_LABEL      , 0 ,  20, "MM/DD/YY"  , 24  },
    { ITEM_TYPE_ACTION     ,  0,   0, "DATESCREEN", 255 }
  }
  ,7
},

//Screen 12 - Brightness control
{
  5,
  {
    { ITEM_TYPE_HEAD       , 0 , 0  , "CPMDEADINT"    , 255 }, 
    { ITEM_TYPE_VARNUM     , 64, 64 , "BRIGHTNESS"    , 255 },
    { ITEM_TYPE_MENU_ACTION,  0,   7, "Save:Brightness", 2  },
    { ITEM_TYPE_ACTION     ,  0,   0, "BrightnessSCN" , 255 },
    { ITEM_TYPE_LEAVE_ACTION, 0,   0, "LeftBrightness", 255 }
  }
  ,7
},

//Screen 13 - Warning configuration
{
  9,
  {
    { ITEM_TYPE_HEAD       , 0 , 0  , "CPMDEADINT"  , 255 }, 
    { ITEM_TYPE_VARNUM     , 10, 50 , "WARNCPM1"    , 255 },
    { ITEM_TYPE_VARNUM     , 32, 50 , "WARNCPM2"    , 255 },
    { ITEM_TYPE_VARNUM     , 54, 50 , "WARNCPM3"    , 255 },
    { ITEM_TYPE_VARNUM     , 76, 50 , "WARNCPM4"    , 255 },
    { ITEM_TYPE_VARNUM     , 98, 50 , "WARNCPM5"    , 255 },
    { ITEM_TYPE_MENU_ACTION,  0,   7, "Save:WarnCPM", 2   },
    { ITEM_TYPE_LABEL      , 0 ,  20, "Warning CPM" , 47  },
    { ITEM_TYPE_ACTION     ,  0,   0, "WARNSCREEN"  , 255 }
  }
  ,7
},
  
//Screen 14 - Language Selection Screen
{
  3,
  {
    { ITEM_TYPE_HEAD       , 0, 0, "CPMDEADINT", 255},
    { ITEM_TYPE_MENU_ACTION, 1, 1, "English"   , 255},
    { ITEM_TYPE_MENU_ACTION, 2, 2, "Japanese"  , 3  }
  }
  ,7
},

//Screen 15 - Total/Timer mode
{
  4,
  {
    { ITEM_TYPE_LABEL      ,255,32, "Average CPM", 25 },
    { ITEM_TYPE_VARLABEL   ,255,48, "TTCOUNT"    , 255},
    { ITEM_TYPE_VARLABEL   ,255,90, "TTTIME"     , 255},
    { ITEM_TYPE_ACTION     ,  2, 2, "TOTALTIMER" , 255}
  }
  ,2
},

//Screen 16 - User interface settings
{
  4,
  {
    { ITEM_TYPE_HEAD       , 0, 0, "CPMDEADINT"   , 255}, 
    { ITEM_TYPE_MENU       ,12, 1, "Brightness"   , 10 },
    { ITEM_TYPE_MENU_ACTION, 0, 2, "Geiger Beep"  , 11 },
//    { ITEM_TYPE_MENU       ,24, 3, "Geiger Pulse" , 255},
    { ITEM_TYPE_MENU       ,14, 4, "Language"     , 12 }
  }
  ,7
},

//Screen 17 - Geiger settings
{
  8,
  {
    { ITEM_TYPE_HEAD       , 0, 0, "CPMDEADINT"      , 255},
    { ITEM_TYPE_MENU       ,19, 1, "\x80Sv/\x80R"    , 255},
    { ITEM_TYPE_MENU       , 5, 2, "Calibration"     , 26 },
    { ITEM_TYPE_MENU_ACTION, 0, 3, "Clear Log"       , 27 },
    { ITEM_TYPE_MENU       ,23, 4, "Log Interval"    , 34 },
    { ITEM_TYPE_MENU       ,13, 5, "Warning Levels"  , 28 },
    { ITEM_TYPE_MENU       ,22, 6, "Bq. Eff. Value"  , 35 },
    { ITEM_TYPE_MENU_ACTION, 0, 7, "CPM/CPS Auto"    , 36 }
  }
  ,7
},

//Screen 18 - Version information
{
  3,
  {
    { ITEM_TYPE_HEAD       , 0 , 0, "CPMDEADINT"        , 255}, 
    { ITEM_TYPE_LABEL      , 0  ,32, "Firmware Release" , 37 },
    { ITEM_TYPE_LABEL      , 255,64, OS100VERSION       , 255}
  }
  ,7
},

//Screen 19 - Sieverts or Rems
{
  3,
  {
    { ITEM_TYPE_HEAD       , 0, 0, "CPMDEADINT", 255}, 
    { ITEM_TYPE_MENU_ACTION, 1, 1, "\x80Sv"    , 38 },
    { ITEM_TYPE_MENU_ACTION, 2, 2, "\x80R"     , 39 }
  }
  ,7
},

//Screen 20 - Set UTC offset
{
  9,
  {
    { ITEM_TYPE_HEAD       , 0 , 0 , "CPMDEADINT" , 255 }, 
    { ITEM_TYPE_VARNUM     , 10, 50, "SIGN:-,+,"  , 255 },
    { ITEM_TYPE_VARNUM     , 32, 50, "OFFHOUR1"   , 255 },
    { ITEM_TYPE_VARNUM     , 54, 50, "OFFHOUR2"   , 255 },
    { ITEM_TYPE_VARNUM     , 76, 50, "OFFMIN1"    , 255 },
    { ITEM_TYPE_VARNUM     , 98, 50, "OFFMIN2"    , 255 },
    { ITEM_TYPE_MENU_ACTION,  0,  7, "Save:UTCOff", 2   },
    { ITEM_TYPE_LABEL      , 0 , 20, "HH:MM"      , 40  },
    { ITEM_TYPE_ACTION     ,  0,  0, "UTCSCREEN"  , 255 }

  }
  ,7
},

//Screen 21 - Becquerel
{
  4,
  {
    { ITEM_TYPE_HEAD        , 0  ,  0, "CPMDEADINT", 255}, 
    { ITEM_TYPE_VARLABEL    , 0  , 30, "BECQINFO"  , 255},
    { ITEM_TYPE_BIGVARLABEL , 0  , 43, "BECQ"      , 255},
    { ITEM_TYPE_LABEL        ,81 , 74, "Bq/m2"     , 255}
  }
  ,1
},

//Screen 22 - Set Becquerel conversion value
{
  7,
  {
    { ITEM_TYPE_HEAD       , 0 ,  0 , "CPMDEADINT", 255 }, 
    { ITEM_TYPE_VARNUM     , 38, 50 , "BECQ1"     , 255 },
    { ITEM_TYPE_VARNUM     , 56, 50 , "BECQ2"     , 255 },
    { ITEM_TYPE_VARNUM     , 74, 50 , "BECQ3"     , 255 },
    { ITEM_TYPE_VARNUM     , 92, 50 , "BECQ4"     , 255 },
    { ITEM_TYPE_MENU_ACTION,  0,   7, "Save:Becq" , 2   },
    { ITEM_TYPE_ACTION     ,  0,   0, "BECQSCREEN", 255 },
  }
  ,7
},

//Screen 23 - Set Logging Interval
{
  7,
  {
    { ITEM_TYPE_HEAD       , 0 ,  0 , "CPMDEADINT"    , 255 }, 
    { ITEM_TYPE_VARNUM     , 38, 50 , "LOGINTER1"     , 255 },
    { ITEM_TYPE_VARNUM     , 56, 50 , "LOGINTER2"     , 255 },
    { ITEM_TYPE_VARNUM     , 74, 50 , "LOGINTER3"     , 255 },
    { ITEM_TYPE_MENU_ACTION,  0,   7, "Save:LogInter" , 2   },
    { ITEM_TYPE_LABEL      , 90,  61, "mins"          , 41  },
    { ITEM_TYPE_ACTION     ,  0,   0, "LOGINTERVAL" , 255 }
  }
  ,7
},

//Screen 24 - Geiger Pulse width
{
  3,
  {
    { ITEM_TYPE_HEAD       , 0 ,  0 , "CPMDEADINT"      , 255 }, 
    { ITEM_TYPE_VARNUM     , 38, 50 , "PULSEWIDTH1"     , 255 },
    { ITEM_TYPE_MENU_ACTION,  0,   7, "Save:PulseWidth" ,2    },
  }
  ,7
},

};

