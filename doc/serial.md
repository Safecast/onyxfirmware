USB Serial interface
====================

Connecting with C-Kermit [Linux]
------------------------
set line /dev/ttyUSB0
set speed 115200
set flow-control none
set terminal lf-display crlf
set carrier-watch off

Commands
--------
### Help ###
* HELP

### Log ###
* LOGXFER
* READ CSV LOG
* LOGSIG
* LOGPAUSE
* LOGRESUME
* LOGCLEAR

### Get ###
* VERSION
* GUID
* GETDEVICETAG
* MAGREAD
  The Hall Effect sensor is located on the end opposite the geiger tube.
  0 Magnet detected
  1024 No Magnet detected

### ?? ###
* WRITEDAC
* READADC

### set ###
* SETDEVICETAG
* SETMICREVERSE
* SETMICIPHONE
* DISPLAYPARAMS
* SETRTC
* SETALARM

### test/debug ###
* DISPLAYTEST
* BATINFODISP
* TESTHP
* LOGSTRESS
* TESTSIGN
* PUBKEY
* KEYVALID
* KEYVALDUMP
* KEYVALSET
* CAPTOUCHTEST
* CAPTOUCHDUMP

### misc ###
* HELLO
* LIST GAMES

