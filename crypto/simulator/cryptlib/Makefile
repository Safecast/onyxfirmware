###################################################
# 7/2006
# Chumby security library
###################################################

.DEFAULT:

#############################################
# Macro
#############################################

#CROSS_OBJ=arm-linux-objcopy
#CC = arm-linux-gcc
#LD = arm-linux-ld
#AR = arm-linux-ar
#STRIP = arm-linux-strip
CROSS_OBJ=objcopy
CC = gcc
LD = ld
AR = ar
STRIP = strip


#CFLAGS =  -D_STANDALONE_ -Os -g -march=armv5
CFLAGS =  -Os -I. -DHAVE_CONFIG_H
#THUMBFLAGS = -mthumb

AESFILES = aes.o
MPARITH = mp.o mpbarrett.o
RSAFILES = rsa.o rsakp.o mpnumber.o mpprime.o fips186.o entropy.o
SHA1FILES = sha1.o
CRYPTOFILES = $(AESFILES) $(SHA1FILES) $(RSAFILES) $(MPARITH)
SHELLFILES = main.o parse.o

OBJS = $(SHELLFILES) beecrypt412_sm.a

all: simpleAuth testCrypto genkeys exportKeys testBlock

##############################################
# Targets
##############################################

%.o: %.c 
	$(CC) $(CFLAGS) $(THUMBFLAGS) -c -o $@ $<

# builds the crypto library
# $@ refers to libcrypto.a in this case
# must put this into a library to obey terms of LGPL
# then we link the library with our other code later on
beecrypt412_sm.a: $(CRYPTOFILES)
	$(AR) -rvu $@ $(CRYPTOFILES)
	ranlib $@

# link into main and go
simpleAuth: $(SHELLFILES) beecrypt412_sm.a
	$(CC) $(OBJS) -o $@
	$(STRIP) simpleAuth

testCrypto: test.o beecrypt412_sm.a
	$(CC) test.o beecrypt412_sm.a -o $@

testParse: testParse.o parse.o
	$(CC) testParse.o parse.o -o $@

testBlock: testBlock.o beecrypt412_sm.a
	$(CC) testBlock.o beecrypt412_sm.a -o $@

genkeys: genkeys.o beecrypt412_sm.a
	$(CC) genkeys.o beecrypt412_sm.a -o $@
	$(STRIP) genkeys

genrandom: genrandom.o beecrypt412_sm.a
	$(CC) genrandom.o beecrypt412_sm.a -o $@
	$(STRIP) genrandom

exportKeys: exportKeys.o beecrypt412_sm.a
	$(CC) exportKeys.o beecrypt412_sm.a -o $@
	$(STRIP) exportKeys

###############################################
# Clean
###############################################

clean:
	rm -rf *.o
	rm -rf *.a
	rm -rf *.exe