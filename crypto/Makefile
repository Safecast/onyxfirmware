APP = catkeys

all: $(APP)

$(APP): catkeys.c 
	$(CC) -c -o catkeys.o catkeys.c
	$(CC) -o $(APP) $(CFLAGS) catkeys.o -luuid

clean:
	rm -f *.o ; rm $(APP)
