#include "screen_layout.h"
#include "safecast_wirish_types.h"
// types are: ITEM_TYPE_MENU, ITEM_TYPE_LABEL, ITEM_TYPE_SELECTNUM, ITEM_TYPE_SELECTION, ITEM_TYPE_VARLABEL, ITEM_TYPE_GRAPH

__FLASH__ screen screens_layout[SCREEN_COUNT] = {

// Screen 0 - main screen
{
 4,
 {
   { ITEM_TYPE_HEAD       , 0, 0, "CPM" }, 
   { ITEM_TYPE_MENU       , 1, 1, "Current Readings" },
   { ITEM_TYPE_MENU       , 2, 2, "Reports"  },
   { ITEM_TYPE_MENU       , 3, 3, "Settings" }
//   { ITEM_TYPE_MENU_ACTION, 0, 4, "Sleep" }
 }
},

// Screen 1 - Current readings screen
{
  5,
  {
    { ITEM_TYPE_HEAD        , 0,  0, "CPM"      }, 
    { ITEM_TYPE_BIGVARLABEL , 0, 26, "CPMDEAD"  },
    { ITEM_TYPE_LABEL       ,89, 60, " CPM"     },
    { ITEM_TYPE_BIGVARLABEL , 0, 70, "SIEVERTS" },
    { ITEM_TYPE_LABEL       ,79,104, "\x80Sv/hr"  }
  }
},

// Screen 2 - Reports menu
{
  4,
  {
    { ITEM_TYPE_HEAD       , 0, 0, "CPM" }, 
    { ITEM_TYPE_MENU       , 4, 1, "Graphs" },
    { ITEM_TYPE_MENU       ,15, 2, "Total/Timer" },
    { ITEM_TYPE_MENU_ACTION, 0, 3, "Data Transfer"}
  }
},

// Screen 3 - Settings menu
{
  8,
  {
    { ITEM_TYPE_HEAD       , 0, 0, "CPM"            }, 
    { ITEM_TYPE_MENU       , 9, 1, "Time/Date"      },
    { ITEM_TYPE_MENU       ,12, 2, "Brightness"     },
    { ITEM_TYPE_MENU       ,13, 3, "Warning Levels" },
    { ITEM_TYPE_MENU       , 5, 4, "Calibration"    },
    { ITEM_TYPE_MENU_ACTION, 0, 5, "Geiger Beep"    },
    { ITEM_TYPE_MENU_ACTION, 0, 6, "Clear Log"      },
    { ITEM_TYPE_MENU       ,14, 7, "Language"       }
//    { ITEM_TYPE_MENU,INVALID_SCREEN, 2, "Averaging Period" },
  }
},

// Screen 4 - Graph display
{
  3,
  {
    { ITEM_TYPE_HEAD    , 0 , 0  , "CPM"             }, 
    { ITEM_TYPE_GRAPH   , 34, 110, "RECENTDATA"      },
    { ITEM_TYPE_LABEL   , 4 , 112, "Last 30 seconds" }
  }
},

//Screen 5 - Calibration Confirmation
{
  4,
  {
    { ITEM_TYPE_HEAD , 0 , 0  , "CPM"              },
    { ITEM_TYPE_MENU , 6 , 3  , "       OK       " },
    { ITEM_TYPE_MENU , 3 , 4  , "     Cancel     " },
    { ITEM_TYPE_LABEL,255, 16 , "Are you sure?"    }
  }
},

//Screen 6 - Calibration Wait 1
{
  4,
  {
    { ITEM_TYPE_LABEL,255, 32, "Waiting for 15s" },
    { ITEM_TYPE_LABEL,255, 64, "Please expose"   },
    { ITEM_TYPE_LABEL,255, 80, "to source."      },
    { ITEM_TYPE_DELAY, 60,100,"DELAYA\0 16,7"   } // 16 second delay
  } 
},

//Screen 7 - Calibration Wait 2
{
  2,
  {
    { ITEM_TYPE_LABEL,255, 32, "Acquiring, 30s" },
    { ITEM_TYPE_DELAY, 60,100,"DELAYB\0 31,8"  } // 31 second delay
  } 
}, 
    

//Screen 8 - Calibration
{
  8,
  {
    { ITEM_TYPE_VARLABEL   , 0 , 0  , "FIXEDSV"   },
    { ITEM_TYPE_VARNUM     , 20, 50 , "CAL1"      },
    { ITEM_TYPE_VARNUM     , 56, 50 , "CAL2"      },
    { ITEM_TYPE_VARNUM     , 74, 50 , "CAL3"      },
    { ITEM_TYPE_VARNUM     , 92, 50 , "CAL4"      },
    { ITEM_TYPE_MENU_ACTION,  0,   7, "Save"      },
    { ITEM_TYPE_ACTION     ,  0,   0, "CALIBRATE" }
  }
},

//Screen 9 - Set time/date
{
  3,
  {
    { ITEM_TYPE_HEAD,             0, 0, "CPM"           }, 
    { ITEM_TYPE_MENU,            10, 1, "Set Time"      },
    { ITEM_TYPE_MENU,            11, 2, "Set Date/Zone" }
  }
},

//Screen 10 - Set time
{
  8,
  {
    { ITEM_TYPE_HEAD       , 0 , 0 , "CPM"       }, 
    { ITEM_TYPE_VARNUM     , 10, 50, "TIMEHOUR1" },
    { ITEM_TYPE_VARNUM     , 32, 50, "TIMEHOUR2" },
    { ITEM_TYPE_VARNUM     , 54, 50, "TIMEMIN1"  },
    { ITEM_TYPE_VARNUM     , 76, 50, "TIMEMIN2"  },
    { ITEM_TYPE_VARNUM     , 98, 50, "TIMESEC1"  },
    { ITEM_TYPE_VARNUM     ,118, 50, "TIMESEC2"  },
    { ITEM_TYPE_MENU_ACTION,  0,  7, "SaveTime"  }
  }
},

//Screen 11 - Set date
{
  10,
  {
    { ITEM_TYPE_HEAD       , 0 , 0  , "CPM"       }, 
    { ITEM_TYPE_VARNUM     , 10, 50 , "DATEMON1"  },
    { ITEM_TYPE_VARNUM     , 32, 50 , "DATEMON2"  },
    { ITEM_TYPE_VARNUM     , 54, 50 , "DATEDAY1"  },
    { ITEM_TYPE_VARNUM     , 76, 50 , "DATEDAY2"  },
    { ITEM_TYPE_VARNUM     , 98, 50 , "DATEYEAR1" },
    { ITEM_TYPE_VARNUM     ,118, 50 , "DATEYEAR2" },
    { ITEM_TYPE_MENU_ACTION,  0,   7, "SaveDate"  },
    { ITEM_TYPE_LABEL      , 0 ,  20, "MM/DD/YY"  },
    { ITEM_TYPE_ACTION     ,  0,   0, "DATESCREEN"}
  }
},

//Screen 12 - Brightness control
{
  3,
  {
    { ITEM_TYPE_HEAD       , 0 , 0  , "CPM"             }, 
    { ITEM_TYPE_VARNUM     , 64, 64 , "BRIGHTNESS"      },
    { ITEM_TYPE_MENU_ACTION,  0,   7, "SaveBrightness"  }
  }
},

//Screen 13 - Warning configuration
{
  8,
  {
    { ITEM_TYPE_HEAD       , 0 , 0  , "CPM"        }, 
    { ITEM_TYPE_VARNUM     , 10, 50 , "WARNCPM1"   },
    { ITEM_TYPE_VARNUM     , 32, 50 , "WARNCPM2"   },
    { ITEM_TYPE_VARNUM     , 54, 50 , "WARNCPM3"   },
    { ITEM_TYPE_VARNUM     , 76, 50 , "WARNCPM4"   },
    { ITEM_TYPE_VARNUM     , 98, 50 , "WARNCPM5"   },
    { ITEM_TYPE_MENU_ACTION,  0,   7, "SaveWarnCPM"},
    { ITEM_TYPE_LABEL      , 0 ,  20, "Warning CPM"}
  }
},
  
//Screen 14 - Language Selection Screen
{
  3,
  {
    { ITEM_TYPE_HEAD       , 0, 0, "CPM"     }, 
    { ITEM_TYPE_MENU_ACTION, 1, 1, "English" },
    { ITEM_TYPE_MENU_ACTION, 2, 2, "Japanese"}
  }
},

//Screen 15 - Total/Timer mode
{
  4,
  {
    { ITEM_TYPE_LABEL      ,255,32, "Total Count"},
    { ITEM_TYPE_VARLABEL   ,255,48, "TTCOUNT"    },
    { ITEM_TYPE_VARLABEL   ,255,90, "TTTIME"     },
    { ITEM_TYPE_ACTION     ,  2, 2, "TOTALTIMER" }
  }
}

};

