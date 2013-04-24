USB Serial interface
====================

Connecting with C-Kermit [Linux]
--------------------------------

    set line /dev/ttyUSB0
    set speed 115200
    set flow-control none
    set terminal lf-display crlf
    set carrier-watch off

Connection with screen [Linux, Mac, any Unix]
--------------------------------------------

    screen /dev/ttyUSB0 115200

Use Ctrl-A, Ctrl-\ then "quit" to exit screen.

Commands
--------
### HELP ###

Display help

### LOGXFER ###

Tranfer the logs in JSON format. LOGXFER pauses logging, and then resumes after transfer.

### READ CSV LOG ###
### LOGSIG ###
### LOGPAUSE ###

Suspends logging on the device.

### LOGRESUME ###

Resumes logging.

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

Set Real-Time Clock. Input should be a Unix timestamp.

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

Runs a display test at the current contrast level: displays vertical then horizontal lines.

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

Try it yourself!

### LIST GAMES ###

Try it yourself too...