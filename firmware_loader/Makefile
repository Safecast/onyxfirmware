APP = fwload

all: $(APP)

$(APP): fwload.c stm32ld/stm32ld.c stm32ld/serial_ftdi.c
	$(CC) -c -o stm32ld.o stm32ld/stm32ld.c
	$(CC) -c -o serial_ftdi.o stm32ld/serial_ftdi.c -I./libftd2xx1.1.0
	$(CC) -o $(APP) $(CFLAGS) -pthread fwload.c serial_ftdi.o stm32ld.o -lftd2xx -ldl -lrt -I./libftd2xx1.1.0 -L./libftd2xx1.1.0/x86_64 -L./libftd2xx1.1.0/i386

clean:
	rm -f *.o ; rm $(APP)
