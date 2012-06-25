GLOBAL_CFLAGS   := -Os -g3 -gdwarf-2  -mcpu=cortex-m3 -mthumb -march=armv7-m \
		   -nostdlib -ffunction-sections -fdata-sections	     \
		   -Wl,--gc-sections -DBOARD_safecast -DMCU_STM32F101RE -DSTM32_HIGH_DENSITY -DVECT_TAB_BASE 
GLOBAL_CXXFLAGS := -fno-rtti -fno-exceptions -Wall
GLOBAL_ASFLAGS  := -mcpu=cortex-m3 -march=armv7-m -mthumb		     \
		   -x assembler-with-cpp 
LDFLAGS  = -mcpu=cortex-m3 -mthumb -Xlinker     \
            --gc-sections --print-gc-sections --march=armv7-m -Wall

firmware: main.cpp
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

	arm-none-eabi-g++ $(GLOBAL_CFLAGS) $(GLOBAL_CXXFLAGS) -I./libmaple -I./wirish -I./wirish/boards -c captouch.cpp -I./wirish/comm -o ./captouch.o
	arm-none-eabi-g++ $(GLOBAL_CFLAGS) $(GLOBAL_CXXFLAGS) -I./libmaple -I. -c display/oled.cpp -o oled.o
	arm-none-eabi-g++ $(GLOBAL_CFLAGS) $(GLOBAL_CXXFLAGS) -c power.cpp -I./wirish -I./libmaple -I./wirish/boards -I./wirish/comm -o power.o
	arm-none-eabi-g++ $(GLOBAL_CFLAGS) $(GLOBAL_CXXFLAGS) -I./display -c main.cpp -I./libmaple -I./wirish -I./wirish/boards -I./buzzer -o main.o

	arm-none-eabi-g++ $(GLOBAL_CFLAGS) $(GLOBAL_CXXFLAGS) -I./libmaple -I./wirish -I./wirish/boards -I./wirish/comm -c wirish/boards.cpp -o ./boards.o
	arm-none-eabi-g++ $(GLOBAL_CFLAGS) $(GLOBAL_CXXFLAGS) -I./libmaple -I./wirish -I./wirish/boards -I./wirish/comm -c wirish/ext_interrupts.cpp -o ./ext_interrupts.o
	arm-none-eabi-g++ $(GLOBAL_CFLAGS) $(GLOBAL_CXXFLAGS) -I./libmaple -I./wirish -c ./wirish/boards/safecast.cpp -o ./boards_safecast.o
	arm-none-eabi-g++ $(GLOBAL_CFLAGS) $(GLOBAL_CXXFLAGS) -Iwirish -Iwirish/comm -I./libmaple -I./wirish/boards -c wirish/wirish_digital.cpp -o wirish_digital.o
	arm-none-eabi-g++ $(GLOBAL_CFLAGS) $(GLOBAL_CXXFLAGS) -Iwirish -Iwirish/comm -I./libmaple -I./wirish/boards -c ./wirish/comm/HardwareSPI.cpp -o HardwareSPI.o
	arm-none-eabi-g++ $(GLOBAL_CFLAGS) $(GLOBAL_CXXFLAGS) -Iwirish -Iwirish/comm -I./libmaple -I./wirish/boards -c ./wirish/HardwareTimer.cpp -o HardwareTimer.o
	arm-none-eabi-g++ $(GLOBAL_CFLAGS) $(GLOBAL_CXXFLAGS) -Iwirish -Iwirish/comm -I./libmaple -I./wirish/boards -c ./wirish/Print.cpp -o Print.o
	arm-none-eabi-g++ $(GLOBAL_CFLAGS) $(GLOBAL_CXXFLAGS) -Iwirish -Iwirish/comm -I./libmaple -I./wirish/boards -c wirish/wirish_time.cpp -o ./wirish_time.o
	arm-none-eabi-g++ -T./jtag.ld -mcpu=cortex-m3 -mthumb -Xlinker --gc-sections --print-gc-sections --march=armv7-m -Wall spi.o spi_aux.o oled.o main.o power.o wirish_digital.o HardwareSPI.o wirish_time.o gpio.o rcc.o boards_safecast.o timer.o adc.o util.o syscalls.o usart.o HardwareTimer.o captouch.o i2c.o ext_interrupts.o systick.o nvic.o exti.o boards.o flash.o -o safecast.elf -Wl,-Map,safecast.map 

	arm-none-eabi-objcopy -v -Obinary safecast.elf safecast.bin

clean:
	rm safecast.bin safecast.elf safecast.map main.o *.o
