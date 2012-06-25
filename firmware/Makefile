GLOBAL_CFLAGS   := -Os -g3 -gdwarf-2  -mcpu=cortex-m3 -mthumb -march=armv7-m \
		   -nostdlib -ffunction-sections -fdata-sections	     \
		   -Wl,--gc-sections -DBOARD_safecast -DMCU_STM32F101RE -DSTM32_HIGH_DENSITY -DVECT_TAB_BASE 
GLOBAL_CXXFLAGS := -fno-rtti -fno-exceptions -Wall
GLOBAL_ASFLAGS  := -mcpu=cortex-m3 -march=armv7-m -mthumb		     \
		   -x assembler-with-cpp 
LDFLAGS  = -mcpu=cortex-m3 -mthumb -Xlinker     \
            --gc-sections --print-gc-sections --march=armv7-m -Wall

firmware: main.cpp

#libmaple
	arm-none-eabi-gcc $(GLOBAL_CFLAGS) -c libmaple/spi.c -o spi.o
	arm-none-eabi-gcc $(GLOBAL_CFLAGS) -c libmaple/rcc.c -o rcc.o
	arm-none-eabi-gcc $(GLOBAL_CFLAGS) -c libmaple/timer.c -o timer.o
	arm-none-eabi-gcc $(GLOBAL_CFLAGS) -c libmaple/adc.c -o adc.o
	arm-none-eabi-gcc $(GLOBAL_CFLAGS) -c libmaple/util.c -o util.o
	arm-none-eabi-gcc $(GLOBAL_CFLAGS) -c libmaple/syscalls.c -o syscalls.o
	arm-none-eabi-gcc $(GLOBAL_CFLAGS) -c libmaple/usart.c -o usart.o
	arm-none-eabi-gcc $(GLOBAL_CFLAGS) -c libmaple/gpio.c -o gpio.o
	arm-none-eabi-gcc $(GLOBAL_CFLAGS) -c libmaple/i2c.c -o i2c.o
	arm-none-eabi-gcc $(GLOBAL_CFLAGS) -c libmaple/systick.c -o systick.o
	arm-none-eabi-gcc $(GLOBAL_CFLAGS) -c libmaple/nvic.c -o nvic.o
	arm-none-eabi-gcc $(GLOBAL_CFLAGS) -c libmaple/exti.c -o exti.o
	arm-none-eabi-gcc $(GLOBAL_CFLAGS) -c libmaple/flash.c -o flash.o
	arm-none-eabi-gcc $(GLOBAL_CFLAGS) -c ./display/spi_aux.c -I./libmaple -I. -o spi_aux.o

# Our code
	arm-none-eabi-g++ $(GLOBAL_CFLAGS) $(GLOBAL_CXXFLAGS) -c ./safecast_config.cpp -I./libmaple -I. -o safecast_config.o
	arm-none-eabi-g++ $(GLOBAL_CFLAGS) $(GLOBAL_CXXFLAGS) -c ./wirish_boards.cpp -I./libmaple -I. -o wirish_boards.o

	arm-none-eabi-g++ $(GLOBAL_CFLAGS) $(GLOBAL_CXXFLAGS) -I./libmaple -I. -c display/oled.cpp -o oled.o
	arm-none-eabi-g++ $(GLOBAL_CFLAGS) $(GLOBAL_CXXFLAGS) -c power.cpp -I. -I./libmaple -o power.o
#	arm-none-eabi-g++ $(GLOBAL_CFLAGS) $(GLOBAL_CXXFLAGS) -I./libmaple -I./wirish -I./wirish/boards -c captouch.cpp -I./wirish/comm -o ./captouch.o
	arm-none-eabi-g++ $(GLOBAL_CFLAGS) $(GLOBAL_CXXFLAGS) -I./display -c main.cpp -I./libmaple -I./buzzer -o main.o

# Final linker step
	arm-none-eabi-g++ -T./linker_files/jtag.ld -L./linker_files -mcpu=cortex-m3 -mthumb -Xlinker --gc-sections --print-gc-sections --march=armv7-m -Wall wirish_boards.o spi.o spi_aux.o oled.o main.o power.o gpio.o rcc.o timer.o adc.o util.o syscalls.o usart.o safecast_config.o i2c.o systick.o nvic.o exti.o flash.o -o safecast.elf -Wl,-Map,safecast.map 

	arm-none-eabi-objcopy -v -Obinary safecast.elf safecast.bin

clean:
	rm safecast.bin safecast.elf safecast.map main.o *.o
