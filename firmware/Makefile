GLOBAL_CFLAGS   := -Os -g3 -gdwarf-2  -mcpu=cortex-m3 -mthumb -march=armv7-m \
		   -nostdlib -ffunction-sections -fdata-sections	     \
		   -Wl,--gc-sections -DBOARD_safecast -DMCU_STM32F101RE -DSTM32_HIGH_DENSITY
GLOBAL_CXXFLAGS := -fno-rtti -fno-exceptions -Wall
GLOBAL_ASFLAGS  := -mcpu=cortex-m3 -march=armv7-m -mthumb		     \
		   -x assembler-with-cpp 
LDFLAGS  = -mcpu=cortex-m3 -mthumb -Xlinker     \
            --gc-sections --print-gc-sections --march=armv7-m -Wall

firmware: main.cpp
	arm-none-eabi-g++ $(GLOBAL_CFLAGS) $(GLOBAL_CXXFLAGS) -I./display main.cpp -I./libmaple -I./wirish -I./wirish/boards -o main.o
	arm-none-eabi-gcc $(GLOBAL_CFLAGS) libmaple/spi.c -o spi.o
	arm-none-eabi-g++ $(GLOBAL_CFLAGS) $(GLOBAL_CXXFLAGS) -Iwirish -Iwirish/comm -I./libmaple -I./wirish/boards display/oled.cpp -o oled.o
	arm-none-eabi-g++ -mcpu=cortex-m3 -mthumb -Xlinker --gc-sections --print-gc-sections --march=armv7-m -Wall main.o -o safecast.elf -Wl,-Map,safecast.map 

	arm-none-eabi-objcopy -v -Obinary safecast.elf safecast.bin

clean:
	rm safecast.bin safecast.elf safecast.map main.o
