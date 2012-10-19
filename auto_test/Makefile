APP = auto_test

all: $(APP)

$(APP): auto_test.c 
	$(CC) -c -o auto_test.o auto_test.c
	$(CC) -o $(APP) $(CFLAGS) auto_test.o

clean:
	rm -f *.o ; rm $(APP)
