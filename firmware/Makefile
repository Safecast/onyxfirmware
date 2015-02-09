OS100VERSION := $(shell cat $(TOP)RELEASENUMBER)
RELEASE_NAME := firmware-$(OS100VERSION)

CFLAGS := -Os -gdwarf-2 -mcpu=cortex-m3 -mthumb -march=armv7-m \
	-lnostdlib -ffunction-sections -fdata-sections -Wl,--gc-sections \
	-DBOARD_safecast -DMCU_STM32F101RE -DSTM32_HIGH_DENSITY -DVECT_TAB_BASE -D'OS100VERSION="$(OS100VERSION)"' \
	-I. -I./libmaple -I./display -I./userinput -I./buzzer -I./accelerometer -I./geiger -I./led -I./gui -I./realtime \
	-I./controller -I./flash_storage -I./serialinterface -I./misc -I./qrcode -I./signing -I./signing/beecrypt -I./log_read -I./modem \
	-I ./libjson
CXXFLAGS := $(CFLAGS) -fno-rtti -fno-exceptions -Wall -lnostdlib
ASFLAGS := -mcpu=cortex-m3 -march=armv7-m -mthumb -x assembler-with-cpp
LDFLAGS := -mcpu=cortex-m3 -mthumb -Xlinker --gc-sections --print-gc-sections --march=armv7-m -Wall

CC = arm-none-eabi-gcc
CXX = arm-none-eabi-g++
OBJCOPY_DEFAULT := arm-none-eabi-objcopy -I binary -O elf32-littlearm -B arm --rename-section .data=.USER_FLASH,alloc,load,readonly,data,contents
LD_DEFAULT := arm-none-eabi-ld -r -b binary

LIBMAPLEOBJS = libmaple/spi.o libmaple/rcc.o libmaple/timer.o libmaple/adc.o libmaple/util.o libmaple/syscalls.o libmaple/usart.o libmaple/gpio.o libmaple/i2c.o libmaple/systick.o libmaple/nvic.o libmaple/exti.o libmaple/flash.o libmaple/bkp.o libmaple/dac.o libmaple/dma.o

OUROBJS = ./display/display.o ./display/spi_aux.o ./display/oled.o ./userinput/captouch.o ./accelerometer/accel.o ./misc/power.o ./misc/wirish_boards.o ./misc/safecast_config.o ./main.o ./realtime/realtime.o ./realtime/rtc.o ./gui/GUI.o ./display/nfont.o ./gui/screen_layout.o ./buzzer/buzzer.o ./geiger/Geiger.o ./misc/utils.o ./flash_storage/flashstorage.o ./serialinterface/serialinterface.o ./controller/Controller.o ./userinput/switch.o ./qrcode/qr_encodeem.o ./qrcode/qr_utils.o ./qrcode/qr_xfer.o ./signing/hal.o ./log_read/log_read.o ./modem/modem.o

BINARYOBJS = ./binary_data/tinyfont_data.o ./binary_data/font_data.o ./binary_data/bignumbers_data.o ./binary_data/flash_data.o ./binary_data/fixed_images.o ./binary_data/splashscreen.o ./binary_data/private_key_data.o ./binary_data/help_screens_en.o ./binary_data/help_screens_jp.o

PICTUREOBJS = ./binary_data/{fixed_images,bignumbers_data,font_data,tinyfont_data,help_screens_en,help_screens_jp,splashscreen,splashscreen888}

JSONOBJS = ./libjson/Objects_static/internalJSONNode.o ./libjson/Objects_static/JSONAllocator.o ./libjson/Objects_static/JSONChildren.o \
           ./libjson/Objects_static/JSONDebug.o ./libjson/Objects_static/JSONIterators.o ./libjson/Objects_static/JSONMemory.o ./libjson/Objects_static/JSONNode.o \
           ./libjson/Objects_static/JSONNode_Mutex.o ./libjson/Objects_static/JSONPreparse.o ./libjson/Objects_static/JSONStream.o ./libjson/Objects_static/JSONValidator.o \
           ./libjson/Objects_static/JSONWorker.o ./libjson/Objects_static/JSONWriter.o ./libjson/Objects_static/libjson.o 

AESFILES = ./signing/aes.o
MPARITH = ./signing/mp.o ./signing/mpbarrett.o
RSAFILES = ./signing/rsa.o ./signing/rsakp.o ./signing/mpnumber.o ./signing/mpprime.o ./signing/fips186.o ./signing/entropy.o
SHA1FILES = ./signing/sha1.o
CRYPTOFILES = $(AESFILES) $(SHA1FILES) $(RSAFILES) $(MPARITH)

all:	release
	@echo
	@echo "$(RELEASE_NAME) is ready!"
	@echo

libcs3:
	cd ./libcs3_stm32_src; make high-density; cd ..

json:
	cd ./libjson; make all; cd ..

to565:
	g++ ./misc/to565.c -o ./misc/to565

pictures: to565
	convert ./binary_data/splashscreen.png -depth 8 RGB:./binary_data/splashscreen888
	./misc/to565 < ./binary_data/splashscreen888 > ./binary_data/splashscreen
	convert ./binary_data/tinyfont.png -depth 1 gray:./binary_data/tinyfont_data
	convert ./binary_data/font.png -depth 2 gray:./binary_data/font_data
	convert ./binary_data/bignumbers.png -depth 2 gray:./binary_data/bignumbers_data
	convert ./binary_data/fixed_images.png -depth 2 gray:./binary_data/fixed_images
	convert ./binary_data/help_screens_en.png -depth 2 gray:./binary_data/help_screens_en
	convert ./binary_data/help_screens_jp.png -depth 2 gray:./binary_data/help_screens_jp

safecast.elf:	main.cpp $(LIBMAPLEOBJS) $(OUROBJS)	libcs3 json pictures $(CRYPTOFILES)
	$(LD_DEFAULT) -o ./binary_data/font_data.o ./binary_data/font_data
	$(LD_DEFAULT) -o ./binary_data/bignumbers_data.o ./binary_data/bignumbers_data
	$(LD_DEFAULT) -o ./binary_data/tinyfont_data.o ./binary_data/tinyfont_data
	$(LD_DEFAULT) -o ./binary_data/fixed_images.o ./binary_data/fixed_images
	$(LD_DEFAULT) -o ./binary_data/help_screens_en.o ./binary_data/help_screens_en
	$(LD_DEFAULT) -o ./binary_data/help_screens_jp.o ./binary_data/help_screens_jp
	$(LD_DEFAULT) -o ./binary_data/splashscreen.o ./binary_data/splashscreen
	$(LD_DEFAULT) -o ./binary_data/private_key_data.o ./binary_data/private_key_data
	$(OBJCOPY_DEFAULT) ./binary_data/font_data ./binary_data/font_data.o
	$(OBJCOPY_DEFAULT) ./binary_data/bignumbers_data ./binary_data/bignumbers_data.o
	$(OBJCOPY_DEFAULT) ./binary_data/tinyfont_data ./binary_data/tinyfont_data.o
	$(OBJCOPY_DEFAULT) ./binary_data/fixed_images ./binary_data/fixed_images.o
	$(OBJCOPY_DEFAULT) ./binary_data/help_screens_en ./binary_data/help_screens_en.o
	$(OBJCOPY_DEFAULT) ./binary_data/help_screens_jp ./binary_data/help_screens_jp.o
	$(OBJCOPY_DEFAULT) ./binary_data/flash_data ./binary_data/flash_data.o
	$(OBJCOPY_DEFAULT) ./binary_data/splashscreen ./binary_data/splashscreen.o
	arm-none-eabi-objcopy -I binary -O elf32-littlearm -B arm --rename-section .data=.private_key_data,alloc,load,readonly,data,contents ./binary_data/private_key_data ./binary_data/private_key_data.o

	# Final linker step
	$(CXX) -T./linker_files/safecast_flash.ld -L./linker_files $(LDFLAGS) $(OUROBJS) $(LIBMAPLEOBJS) $(BINARYOBJS) $(CRYPTOFILES) ./libcs3_stm32_src/*.o $(JSONOBJS) -o safecast.elf -Wl,-Map,safecast.map
	# clean up to remove unused functions and reduce code size
	arm-none-eabi-strip safecast.elf

release:	safecast.elf
	# Convert elf to flat binary for programming.
	arm-none-eabi-objcopy -v -Obinary safecast.elf $(RELEASE_NAME).bin

upload_mac: release
	@echo
	@echo "Checking for valid firmware uploader..."
	[ -e ../firmware_loader/fwload ] || exit
	@echo
	@echo
	@echo "Starting device firmware upload..."
	# sudo kextunload -b com.FTDI.driver.FTDIUSBSerialDriver
	# On Mavericks, Apple is handling the FTDI driver, for better or worse:
	#sudo kextunload -b com.apple.driver.AppleUSBFTDI 
	../firmware_loader/fwload -f $(RELEASE_NAME).bin
	sudo kextload -b com.apple.driver.AppleUSBFTDI 
	# sudo kextload -b com.FTDI.driver.FTDIUSBSerialDriver
	@echo
	@echo "Done... seems it worked fine :)"

upload:	release
	@echo
	@echo "Checking for valid firmware uploader..."
	[ -e ../firmware_loader/fwload ] || exit
	@echo
	@echo
	@echo "Starting device firmware upload..."
	-sudo rmmod ftdi_sio
	-sudo rmmod usbserial
	sudo ../firmware_loader/fwload -f $(RELEASE_NAME).bin
	sudo modprobe usbserial
	sudo modprobe ftdi_sio
	@echo
	@echo "All seems to have worked fine ;-)"

monitor:
	screen /dev/ttyUSB0 115200

clean:
	rm -f safecast.elf safecast.map $(LIBMAPLEOBJS) $(OUROBJS) $(BINARYOBJS) $(PICTUREOBJS) $(CRYPTOFILES)
	cd ./libjson; make clean; cd ..
	rm -f libcs3_stm32_src/*.o libcs3_stm32_src/*.a
	rm -f misc/to565

distclean:	clean
	rm -f firmware-*.bin
