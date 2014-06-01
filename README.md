safecastgeiger
==============

A firmware project for the Safecast geiger counter (see http://www.bunniestudios.com/blog/?p=2218) based on ARM Cortex-M3 MCU (STM32F101RET6, http://www.st.com/web/catalog/mmc/FM141/SC1169/SS1031/LN1567/PF206930).

Please see the LICENSE file for licensing information. 


User firmware loaders and log uploading
=======================================

MacOSX
------

A simple tool for updating to the latest firmware release and uploading logs for Mac OS X is available here:

http://41j.com/OnyxLoader.zip

Source code for these firmware loaders is available here: http://github.com/new299/onyxloader


Windows
-------

You need to install the FTDI drivers from here: http://www.ftdichip.com/Drivers/CDM/CDM20824_Setup.exe 
You may also need to install the VisualStudio runtime from here: http://www.microsoft.com/en-us/download/details.aspx?id=30679

You can then run the windows tool which is available here:

http://41j.com/OnyxLoader.exe


Linux
-------

See the source in firmware_loader (also compiled when you execute `make upload monitor` inside firmware directory).


Building
========

The initial codebase was forked from the libmaple project. This provides board support for the STM32 microcontroller. libmaple has been modified to suit the purposes of this project, support for the STM32F103RET6 which this geiger counter uses was added. A few of the functions provided by libmaple have the potential to block under certain error conditions, this has been patched. libmaple also comes with a C++ wrapper library called "Wirish" for the most part this has been removed.

To build the software you need a ARM G++ compiler toolchain that targets bare metal. We have been using the Codesourcey tools:

http://static.leaflabs.com/pub/codesourcery/

In particular the 32bit Linux binary distribution: gcc-arm-none-eabi-latest-linux32.tar.gz

You will also need a copy of imagemagick.

To build the codebase you may want to download/extract this and add it to your path. You should then be able to run make in the firmware subdirectory to build a firmware image (safecast.bin).

To program the firmware you can use the tool provided in the firmware_loader subdirectory. To build it type "make". The script "test" containing a usage example.

Example Build Setup
===================

* Install imagemagick

```
sudo apt-get install imagemagick
```
* Get the codesourcey tool chain

```
mkdir $HOME/armcompiler
cd $HOME/armcompiler
wget http://static.leaflabs.com/pub/codesourcery/gcc-arm-none-eabi-latest-linux32.tar.gz
tar xzvf gcc-arm-none-eabi-latest-linux32.tar.gz
```

* Add codesourcery tools to your path, this can be added to your .bashrc:

```
export PATH=$PATH:$HOME/armcompiler/arm/bin
```

*  And then checkout and build:

```
cd $HOME/gitcode # or whereever you want to keep your code 
git clone git@github.com:Safecast/onyxfirmware.git
git checkout devel # work on development branch
cd firmware
make
make upload # you may need to type make in ../firmware_loader first.
```
