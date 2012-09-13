#include "screen_layout.h"
#include "safecast_wirish_types.h"
// types are: ITEM_TYPE_MENU, ITEM_TYPE_LABEL, ITEM_TYPE_SELECTNUM, ITEM_TYPE_SELECTION, ITEM_TYPE_VARLABEL, ITEM_TYPE_GRAPH

__FLASH__ screen screens_layout[SCREEN_COUNT] = {

// Screen 0 - main screen
{
 5,
 {
   { ITEM_TYPE_HEAD       , 0, 0, "CPM" }, 
   { ITEM_TYPE_MENU       , 1, 1, "Current Readings" },
   { ITEM_TYPE_MENU       , 2, 2, "Reports"  },
   { ITEM_TYPE_MENU       , 3, 3, "Settings" },
   { ITEM_TYPE_MENU_ACTION, 0, 4, "Sleep" }
 }
},

// Screen 1 - Current readings screen
{
  8,
  {
    { ITEM_TYPE_HEAD        , 0,  0, "CPM"      }, 
    { ITEM_TYPE_VARLABEL    , 0, 30, "TIME"     },
    { ITEM_TYPE_VARLABEL    , 0, 46, "DATE"     },
    { ITEM_TYPE_VARLABEL    , 0, 62, "CPM"      },
    { ITEM_TYPE_VARLABEL    , 0, 76, "CPMDEAD"  },
    { ITEM_TYPE_VARLABEL    , 0, 92, "SIEVERTS" }
  }
},

// Screen 2 - Reports menu
{
  5,
  {
    { ITEM_TYPE_HEAD       ,             0, 0, "CPM" }, 
    { ITEM_TYPE_MENU       ,             4, 1, "Graphs" },
    { ITEM_TYPE_MENU       ,INVALID_SCREEN, 2, "Visualisation" },
    { ITEM_TYPE_MENU       ,INVALID_SCREEN, 3, "Total/Timer" },
    { ITEM_TYPE_MENU_ACTION,             0, 4, "Data Transfer"}
  }
},

// Screen 3 - Settings menu
{
  8,
  {
    { ITEM_TYPE_HEAD,             0, 0, "CPM" }, 
    { ITEM_TYPE_MENU,             9, 1, "Time/Date" },
    { ITEM_TYPE_MENU,INVALID_SCREEN, 2, "Averaging Period" },
    { ITEM_TYPE_MENU,INVALID_SCREEN, 3, "Logging Inverval" },
    { ITEM_TYPE_MENU,INVALID_SCREEN, 4, "Warning Levels" },
    { ITEM_TYPE_MENU,INVALID_SCREEN, 5, "Audio" },
    { ITEM_TYPE_MENU,             5, 6, "Calibration" },
    { ITEM_TYPE_MENU,INVALID_SCREEN, 7, "Language" }
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
    { ITEM_TYPE_LABEL, 12, 16 , "Are you sure?"    }
  }
},

//Screen 6 - Calibration Wait 1
{
  4,
  {
    { ITEM_TYPE_LABEL, 4, 32, "Waiting for 15s" },
    { ITEM_TYPE_LABEL, 0, 64, "Please expose"   },
    { ITEM_TYPE_LABEL, 0, 80, "to source."      },
    { ITEM_TYPE_DELAY, 60,100,"DELAYA\0 16,7"   } // 16 second delay
  } 
},

//Screen 7 - Calibration Wait 2
{
  2,
  {
    { ITEM_TYPE_LABEL, 4, 32, "Acquiring, 30s" },
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
  8,
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
    { ITEM_TYPE_VARNUM     ,120, 50, "TIMESEC2"  },
    { ITEM_TYPE_MENU_ACTION,  0,  7, "SaveTime"  }
  }
},

//Screen 11 - Set date
{
  8,
  {
    { ITEM_TYPE_HEAD       , 0 , 0  , "CPM"       }, 
    { ITEM_TYPE_VARNUM     , 10, 50 , "DATEDAY1"  },
    { ITEM_TYPE_VARNUM     , 32, 50 , "DATEDAY2"  },
    { ITEM_TYPE_VARNUM     , 54, 50 , "DATEMON1"  },
    { ITEM_TYPE_VARNUM     , 76, 50 , "DATEMON2"  },
    { ITEM_TYPE_VARNUM     , 98, 50 , "DATEYEAR1" },
    { ITEM_TYPE_VARNUM     ,120, 50 , "DATEYEAR2" },
    { ITEM_TYPE_MENU_ACTION,  0,   7, "SaveDate"  }
  }
}

};

