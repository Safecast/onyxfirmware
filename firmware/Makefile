CFLAGS   := -Os -gdwarf-2  -mcpu=cortex-m3 -mthumb -march=armv7-m \
		   -lnostdlib -ffunction-sections -fdata-sections	     \
		   -Wl,--gc-sections -DBOARD_safecast -DMCU_STM32F101RE -DSTM32_HIGH_DENSITY -DVECT_TAB_BASE -I. -I./libmaple -I./display -I./userinput -I./buzzer -I./accelerometer -I./geiger -I./led -I./gui -I./realtime -I./controller -I./flash_storage -I./serialinterface -I./misc -I./qrcode -D'OS100VERSION=$(shell cat $(TOP)RELEASENUMBER)'
CXXFLAGS := $(CFLAGS) -fno-rtti -fno-exceptions -Wall -lnostdlib
ASFLAGS  := -mcpu=cortex-m3 -march=armv7-m -mthumb		     \
		   -x assembler-with-cpp 
LDFLAGS  = -mcpu=cortex-m3 -mthumb -Xlinker     \
            --gc-sections --print-gc-sections --march=armv7-m -Wall 

CC = arm-none-eabi-gcc
CXX = arm-none-eabi-g++

LIBMAPLEOBJS = libmaple/spi.o libmaple/rcc.o libmaple/timer.o libmaple/adc.o libmaple/util.o libmaple/syscalls.o libmaple/usart.o libmaple/gpio.o libmaple/i2c.o libmaple/systick.o libmaple/nvic.o libmaple/exti.o libmaple/flash.o libmaple/bkp.o libmaple/dac.o

OUROBJS = ./display/display.o ./display/spi_aux.o ./display/oled.o ./userinput/captouch.o ./accelerometer/accel.o ./misc/power.o ./misc/wirish_boards.o ./misc/safecast_config.o ./main.o ./realtime/realtime.o ./realtime/rtc.o ./gui/GUI.o ./display/nfont.o ./gui/screen_layout.o ./buzzer/buzzer.o ./geiger/Geiger.o ./misc/utils.o ./flash_storage/flashstorage.o ./serialinterface/serialinterface.o ./controller/Controller.o ./userinput/switch.o ./qrcode/qr_encodeem.o ./qrcode/qr_utils.o ./qrcode/qr_xfer.o

BINARYOBJS = ./binary_data/tinyfont_data.o ./binary_data/font_data.o ./binary_data/bignumbers_data.o ./binary_data/flash_data.o ./binary_data/fixed_images.o

all: firmware

libcs3:
	cd ./libcs3_stm32_src;make high-density;cd ..

firmware: main.cpp $(LIBMAPLEOBJS) $(OUROBJS) libcs3
	##convert safemed128.png -depth 8 RGB:image_1_data888
	##./to565 < image_1_data888 > image_1_data
	convert ./binary_data/tinyfont.png -depth 1 gray:./binary_data/tinyfont_data
	convert ./binary_data/font.png -depth 2 gray:./binary_data/font_data
	convert ./binary_data/bignumbers.png -depth 2 gray:./binary_data/bignumbers_data
	convert ./binary_data/fixed_images.png -depth 2 gray:./binary_data/fixed_images
	##arm-none-eabi-ld -r -b binary -o image_1_data.o image_1_data
	arm-none-eabi-ld -r -b binary -o ./binary_data/font_data.o ./binary_data/font_data
	arm-none-eabi-ld -r -b binary -o ./binary_data/bignumbers_data.o ./binary_data/bignumbers_data
	arm-none-eabi-ld -r -b binary -o ./binary_data/tinyfont_data.o ./binary_data/tinyfont_data
	arm-none-eabi-ld -r -b binary -o ./binary_data/fixed_images.o ./binary_data/fixed_images
	arm-none-eabi-objcopy -I binary -O elf32-littlearm -B arm --rename-section .data=.USER_FLASH,alloc,load,readonly,data,contents ./binary_data/font_data ./binary_data/font_data.o
	arm-none-eabi-objcopy -I binary -O elf32-littlearm -B arm --rename-section .data=.USER_FLASH,alloc,load,readonly,data,contents ./binary_data/bignumbers_data ./binary_data/bignumbers_data.o
	arm-none-eabi-objcopy -I binary -O elf32-littlearm -B arm --rename-section .data=.USER_FLASH,alloc,load,readonly,data,contents ./binary_data/tinyfont_data ./binary_data/tinyfont_data.o
	arm-none-eabi-objcopy -I binary -O elf32-littlearm -B arm --rename-section .data=.USER_FLASH,alloc,load,readonly,data,contents ./binary_data/fixed_images ./binary_data/fixed_images.o
	arm-none-eabi-objcopy -I binary -O elf32-littlearm -B arm --rename-section .data=.USER_FLASH,alloc,load,readonly,data,contents ./binary_data/flash_data ./binary_data/flash_data.o
	# Final linker step
	arm-none-eabi-g++ -T./linker_files/safecast_flash.ld -L./linker_files -mcpu=cortex-m3 -mthumb -Xlinker --gc-sections --print-gc-sections --march=armv7-m -Wall $(OUROBJS) $(LIBMAPLEOBJS) $(BINARYOBJS) ./libcs3_stm32_src/*.o -o safecast.elf -Wl,-Map,safecast.map 
	# Convert elf to flat binary for programming.
	arm-none-eabi-objcopy -v -Obinary safecast.elf safecast.bin

to565: to565.c
	gcc to565.c -o to565

clean:
	rm safecast.bin safecast.elf safecast.map $(LIBMAPLEOBJS) $(OUROBJS)
	rm image_1_data*
