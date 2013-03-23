OnyxLoader - Mac
================

There are two versions of the mac firmware loaders: OnyxLoader and OnyxLoaderA.

Firmware loaders can be downloaded from:

http://41j.com/OnyxLoader.zip

http://41j.com/OnyxLoaderA.zip

They are functionaly identical, however OnyxLoaderA will unload FTDI drivers as used by the Arduino and other tools.
Unfortunately these drivers and the drivers used by this firmware loader can not co-exist. As OnyxLoaderA unloads the
Arduino drivers you will need to reboot in order to reactivate the Arduino tools.


Troubleshooting
===============

1. Ensure you have the latest available loaders supplied at the links above.

2. Try **both** OnyxLoader and OnyxLoaderA, even if you know you have the Arduino drivers on you system.

3. When running OnyxLoaderA ensure you are using the correct password.

4. The tools require a working internet connection, make sure your internet connection is working and you can connect to 41j.com in a web browser.
