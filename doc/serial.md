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

### GETCPM ###

Get counts per minute.

### GETCPM30 ###

Get counts per minute using fixed 30s window.

### GETCPMDEADTIME ###

Deadtime compensated CPM

### CPMVALID ###

Check if the CPM reading is valid, it usually takes some time for the readings to stablize.

|Value |Description       |
|------|------------------|
|0     |Not Ready         |
|1     |Valid Measurement |

### WRITEDAC ###
### READADC ###
### SETDEVICETAG ###
### SETMICREVERSE ###
### SETMICIPHONE ###
### DISPLAYPARAMS ###
### SETRTC ###

Set Real-Time Clock

Known Bugs

1. Input is not validated, pressing ```<enter>``` at the ```#>``` prompt will reset time to 1970
2. Device is not Y2038 compliant

```
>SETRTC
Current unixtime is 1366438999
#>1366439026
```

### SETALARM ###
### DISPLAYTEST ###
### BATINFODISP ###
### TESTHP ###
### LOGSTRESS ###
### TESTSIGN ###

Test PGP Signing

```
>TESTSIGN
Initializing PK system.
Computing SHA-1 hash of: Good evening gentlemen.

Hashed result to sign: adbf3364c89306874b99fc2e87d612774e1b7b29
Hashed result to sign: adbf3364c89306874b99fc2e87d612774e1b7b29
Initializing key parameters for 2048-bit RSA.
n:
0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef
0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef
0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef
0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef
Signing hash...
Signature is (in hex):
0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef
0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef
0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef
0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef
Decrypting test hash...
Decrypted hash (should match original hash):
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
uu_pktest PASS
Done.
```

### PUBKEY ###

Display PGP Public Key Block used for log signing

```
>PUBKEY
-----BEGIN PGP PUBLIC KEY BLOCK-----
Version: GnuPG v1.4.11 (GNU/Linux)

0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef
0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef
0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef
0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef
0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef
0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef
0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef
0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef
0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef
0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef
0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef
0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef
0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef
0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef
0123456789abcdef0123456789==
=1234
-----END PGP PUBLIC KEY BLOCK-----
```

### KEYVALID ###

Check PGP Key

```
>KEYVALID
uu_valid VALID KEY
```

### KEYVALDUMP ###

Dump key=val database

```
>KEYVALDUMP
key=val
GEIGERBEEP=false
WARNCPM=100
PULSEWIDTH=2
UTCOFFSETMINS=-240
```

### KEYVALSET ###
### CAPTOUCHTEST ###
### CAPTOUCHDUMP ###
### HELLO ###
### LIST GAMES ###

