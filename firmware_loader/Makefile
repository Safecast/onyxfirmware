all: fwload pkload

fwload: fwload.c stm32ld/stm32ld.c stm32ld/serial_ftdi.c
	$(CC) -c -o stm32ld.o stm32ld/stm32ld.c
	$(CC) -c -o serial_ftdi.o stm32ld/serial_ftdi.c -I./libftd2xx1.1.0
	$(CC) -o fwload $(CFLAGS) -pthread fwload.c serial_ftdi.o stm32ld.o -lftd2xx -ldl -lrt -I./libftd_linux/libftd2xx1.1.0 -L./libftd_linux/libftd2xx1.1.0/x86_64 -L./libftd_linux/libftd2xx1.1.0/i386

pkload: fwload pkload.c
	$(CC) -o pkload $(CFLAGS) -pthread pkload.c serial_ftdi.o stm32ld.o -lftd2xx -ldl -lrt -I./libftd_linux/libftd2xx1.1.0 -L./libftd_linux/libftd2xx1.1.0/x86_64 -L./libftd_linux/libftd2xx1.1.0/i386

fwload_mac: fwload.c stm32ld/stm32ld.c stm32ld/serial_ftdi.c
	$(CC) -c -o stm32ld.o stm32ld/stm32ld.c
	$(CC) -c -o serial_ftdi.o stm32ld/serial_ftdi.c -I./libftd_mac/64
	$(CC) -o fwload $(CFLAGS) -pthread fwload.c serial_ftdi.o stm32ld.o ./libftd_mac/64/*.o -I./libftd_mac/64 -L./libftd_mac/64 -framework IOKit -framework CoreFoundation
	

clean:
	rm -f *.o ; rm $(APP)
