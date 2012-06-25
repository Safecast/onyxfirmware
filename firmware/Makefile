CFLAGS   := -Os -g3 -gdwarf-2  -mcpu=cortex-m3 -mthumb -march=armv7-m \
		   -nostdlib -ffunction-sections -fdata-sections	     \
		   -Wl,--gc-sections -DBOARD_safecast -DMCU_STM32F101RE -DSTM32_HIGH_DENSITY -DVECT_TAB_BASE 
CXXFLAGS := $(CFLAGS) -fno-rtti -fno-exceptions -Wall
ASFLAGS  := -mcpu=cortex-m3 -march=armv7-m -mthumb		     \
		   -x assembler-with-cpp 
LDFLAGS  = -mcpu=cortex-m3 -mthumb -Xlinker     \
            --gc-sections --print-gc-sections --march=armv7-m -Wall

CC = arm-none-eabi-gcc
CXX = arm-none-eabi-g++

LIBMAPLEOBJS = libmaple/spi.o libmaple/rcc.o libmaple/timer.o libmaple/adc.o libmaple/util.o libmaple/syscalls.o libmaple/usart.o libmaple/gpio.o libmaple/i2c.o libmaple/systick.o libmaple/nvic.o libmaple/exti.o libmaple/flash.o

firmware: main.cpp $(LIBMAPLEOBJS)
# Our code
	arm-none-eabi-gcc $(CFLAGS) -c ./display/spi_aux.c -I./libmaple -I. -o spi_aux.o
	arm-none-eabi-g++ $(CXXFLAGS) -c ./safecast_config.cpp -I./libmaple -I. -o safecast_config.o
	arm-none-eabi-g++ $(CXXFLAGS) -c ./wirish_boards.cpp -I./libmaple -I. -o wirish_boards.o

	arm-none-eabi-g++ $(CXXFLAGS) -I./libmaple -I. -c display/oled.cpp -o oled.o
	arm-none-eabi-g++ $(CXXFLAGS) -c power.cpp -I. -I./libmaple -o power.o
#	arm-none-eabi-g++ $(CXXFLAGS) -I./libmaple -I./wirish -I./wirish/boards -c captouch.cpp -I./wirish/comm -o ./captouch.o
	arm-none-eabi-g++ $(CXXFLAGS) -I./display -c main.cpp -I./libmaple -I./buzzer -o main.o

# Final linker step
	arm-none-eabi-g++ -T./linker_files/jtag.ld -L./linker_files -mcpu=cortex-m3 -mthumb -Xlinker --gc-sections --print-gc-sections --march=armv7-m -Wall wirish_boards.o spi_aux.o oled.o main.o power.o safecast_config.o $(LIBMAPLEOBJS) -o safecast.elf -Wl,-Map,safecast.map 

	arm-none-eabi-objcopy -v -Obinary safecast.elf safecast.bin

.o: 
	arm-none-eabi-gcc $(GLOBAL_CFLAGS) -c 

clean:
	rm safecast.bin safecast.elf safecast.map main.o libmaple/*.o
