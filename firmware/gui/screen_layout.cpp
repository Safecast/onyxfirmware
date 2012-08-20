#include "screen_layout.h"

// types are: ITEM_TYPE_MENU, ITEM_TYPE_LABEL, ITEM_TYPE_SELECTNUM, ITEM_TYPE_SELECTION, ITEM_TYPE_VARLABEL, ITEM_TYPE_GRAPH

screen screens_layout[SCREEN_COUNT] = {

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
    { ITEM_TYPE_HEAD    , 0 ,  0, "CPM" }, 
    { ITEM_TYPE_LABEL   , 0 , 30, "Current Reading" },
    { ITEM_TYPE_LABEL   , 0 , 46, "CPM" },
    { ITEM_TYPE_LABEL   , 0 , 66, "CPMd" },
    { ITEM_TYPE_LABEL   , 0 , 82, "Svrt" },
    { ITEM_TYPE_VARLABEL, 32, 46, "CPM" },
    { ITEM_TYPE_VARLABEL, 40, 66, "CPMDEAD" },
    { ITEM_TYPE_VARLABEL, 40, 82, "SEIVERTS" }
  }
},

// Screen 2 - Reports menu
{
  5,
  {
    { ITEM_TYPE_HEAD,             0, 0, "CPM" }, 
    { ITEM_TYPE_MENU,             4, 1, "Graphs" },
    { ITEM_TYPE_MENU,INVALID_SCREEN, 2, "Visualisation" },
    { ITEM_TYPE_MENU,INVALID_SCREEN, 3, "Total/Timer" },
    { ITEM_TYPE_MENU,INVALID_SCREEN, 4, "Data Transfer" }
  }
},

// Screen 3 - Settings menu
{
  8,
  {
    { ITEM_TYPE_HEAD,             0, 0, "CPM" }, 
    { ITEM_TYPE_MENU,INVALID_SCREEN, 1, "Time/Date" },
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
    { ITEM_TYPE_HEAD    , 0 , 0  , "CPM" }, 
    { ITEM_TYPE_GRAPH   , 34, 110, "RECENTDATA" },
    { ITEM_TYPE_LABEL   , 4 , 112, "Last 30 seconds" }
  }
},

//Screen 5 - Calibration
{
  6,
  {
    { ITEM_TYPE_HEAD   , 0 , 0  , "CPM"  },
    { ITEM_TYPE_VARNUM , 50, 50 , "CAL1" },
    { ITEM_TYPE_LABEL  , 60, 60 , "."    },
    { ITEM_TYPE_VARNUM , 76, 50 , "CAL3" },
    { ITEM_TYPE_VARNUM , 94, 50 , "CAL3" },
    { ITEM_TYPE_VARNUM ,112, 50 , "CAL4" }
  }
}

};

