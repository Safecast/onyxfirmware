CPM Calculation
---------------

The device determines the CPM from the last 1000 events or events in the last 2 minutes (whichever is less).

If the CPM is fluctuating rapidly, the device invalidates the previously acquired data and restarts acquisition. This is determined by taking two 5 seconds windows covering the last 10 seconds. If they differ by more than a factor of 100, acquisition is restarted. This accounts for situations where you remove the device from a source and return to background level readings, in this situation you are unlikely to want to wait 2 minutes for the CPM level to return to normal.

Logging
-------

While in sleep mode the device logs a measurement every 30minutes. This measurement acquires data using a fixed 30second window. The logging interval can be configured by the user, shorter inte
rvals will result in reduced battery life.
