APP = auto_test

all: $(APP)

$(APP): $(APP).c
	$(CC) -c -o $(APP).o $(APP).c
	$(CC) -o $(APP) $(CFLAGS) $(APP).o

clean:
	rm -f *.o

distclean: clean
	rm -f $(APP)
