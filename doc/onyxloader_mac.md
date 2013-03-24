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

1. Try rebooting the system.

2. Ensure that OnyxLoader isn't already running, force quit if required (right click with the Option key pressed and select "force quit").

2. Ensure you have the latest available loaders supplied at the links above.

3. Try **both** OnyxLoader and OnyxLoaderA, even if you know you have the Arduino drivers on you system.

4. When running OnyxLoaderA ensure you are using the correct password.

5. The tools require a working internet connection, make sure your internet connection is working and you can connect to 41j.com in a web browser.

6. If all the above fail please file a github issue, please include the firmware version number used, and any errors displayed.
