##################################################
##      Blockchain IoT Security Framework       ##
##          University of Saskatchewan          ##
##                     2020                     ##
##----------------------------------------------##
##                  Kale Yuzik                  ##
##               kay851@usask.ca                ##
##################################################


CC = g++
CROSSCOMPILE =
CFLAGS =
CPPFLAGS = -std=gnu++17 -Wall -Wextra -pedantic
LDFLAGS =

GASONINC = ./gason/src
CXXOPTSINC = ./cxxopts/include
WIRINGPIINC = ./WiringPi/wiringPi
LIBCONFIGINC = ./libconfig/lib

INCLUDE = -I . -I $(CXXOPTSINC) -I $(GASONINC) -I $(WIRINGPIINC) -I $(LIBCONFIGINC)
ARCH = $(shell uname -s)$(shell uname -m)

BUILD = ./build
BIN = $(BUILD)/bin/$(ARCH)
OBJ = $(BUILD)/obj/$(ARCH)
LIB = $(BUILD)/lib/$(ARCH)

DEBUG =

.PHONY: all mkdirs debug clean

all: mkdirs $(BIN)/client $(BIN)/lora_trx

mkdirs:
	mkdir -p $(BIN) $(OBJ) $(LIB)

debug: debug_flag all

debug_flag:
	$(eval DEBUG = -D_DEBUG )

clean:
	rm -rf ./build


### Static Library ###

$(OBJ)/blockchainsec.o: blockchainsec.cpp
	$(CROSSCOMPILE)$(CC) $(CPPFLAGS) -c -fPIC $(DEBUG) -o $@ $(INCLUDE) $<

$(LIB)/libblockchainsec.a: $(OBJ)/blockchainsec.o
	ar rcs $@ $^

### Shared Library ###
#$(LIB)/libblockchainsec.so: $(OBJ)/blockchainsec.o
#	$(CROSSCOMPILE)$(CC) -o $@ -shared -Wl,-soname,libblockchainsec.so $<



### Client Binary ###

$(OBJ)/client.o: client.cpp
	$(CROSSCOMPILE)$(CC) $(CPPFLAGS) -c $(DEBUG) -o $@ $(INCLUDE) $<

$(OBJ)/lora_trx.o: lora_trx.cpp
	$(CROSSCOMPILE)$(CC) -Wall -std=c++11 -c -o $@ $(INCLUDE) $<

$(OBJ)/misc.o: misc.cpp
	$(CROSSCOMPILE)$(CC) $(CPPFLAGS) -c $(DEBUG) -o $@ $(INCLUDE) $<

$(BIN)/client: $(OBJ)/client.o $(OBJ)/lora_trx.o $(OBJ)/misc.o $(OBJ)/gason.o $(LIB)/libblockchainsec.a
	$(CROSSCOMPILE)$(CC) $(CPPFLAGS) -o $@ \
		$(OBJ)/client.o \
		$(OBJ)/lora_trx.o \
		$(OBJ)/misc.o \
		$(OBJ)/gason.o \
		-L $(LIB) \
		-lblockchainsec -lconfig++ -lwiringPi -lpthread
	cp ./*.sol ./*.conf $(BIN)/

### Client Dependencies ###

$(OBJ)/gason.o: $(GASONINC)/gason.cpp
	$(CROSSCOMPILE)$(CC) -std=c++11 -c -o $@ $<
