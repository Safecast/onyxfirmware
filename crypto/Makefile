APP = catkeys

all: $(APP)

$(APP): $(APP).c
	$(CC) -c -o $(APP).o $(APP).c
	$(CC) -o $(APP) $(CFLAGS) $(APP).o -luuid

clean:
	rm -f *.o

distclean: clean
	rm -f $(APP)
