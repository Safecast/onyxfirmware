USB Serial interface
====================

Connecting with C-Kermit [Linux]
--------------------------------

    set line /dev/ttyUSB0
    set speed 115200
    set flow-control none
    set terminal lf-display crlf
    set carrier-watch off

Commands
--------
### HELP ###

Display help

### LOGXFER ###
### READ CSV LOG ###
### LOGSIG ###
### LOGPAUSE ###
### LOGRESUME ###
### LOGCLEAR ###

Clear stored logs

```
>LOGCLEAR
Clearing flash log
Cleared
```

### VERSION ###

Firmware Version number

```
>VERSION
Version: 12.9
```

### GUID ###

Global Uniquie ID

```
>GUID
uu_guid 0123456789abcdef0123456789abcdef
```

### GETDEVICETAG ###
### MAGREAD ###

The Hall Effect sensor is located on the end opposite the geiger tube.
 
|Value |Description       |
|------|------------------|
|0     |Magnet detected   |
|1024  |No Magnet detected|
 
### WRITEDAC ###
### READADC ###
### SETDEVICETAG ###
### SETMICREVERSE ###
### SETMICIPHONE ###
### DISPLAYPARAMS ###
### SETRTC ###
### SETALARM ###
### DISPLAYTEST ###
### BATINFODISP ###
### TESTHP ###
### LOGSTRESS ###
### TESTSIGN ###
### PUBKEY ###
### KEYVALID ###
### KEYVALDUMP ###
### KEYVALSET ###
### CAPTOUCHTEST ###
### CAPTOUCHDUMP ###
### HELLO ###
### LIST GAMES ###

