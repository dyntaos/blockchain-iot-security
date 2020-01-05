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
CPPFLAGS = -std=gnu++17 -Wall -Wextra -pedantic -g
LDFLAGS =

JSONINC = ./json/include
CXXOPTSINC = ./cxxopts/include
WIRINGPIINC = ./WiringPi/wiringPi
LIBCONFIGINC = ./libconfig/lib

INCLUDE = -I . -I $(CXXOPTSINC) -I $(JSONINC) -I $(WIRINGPIINC) -I $(LIBCONFIGINC)
ARCH = $(shell uname -s)$(shell uname -m)

BUILD = ./build
BIN = $(BUILD)/bin/$(ARCH)
OBJ = $(BUILD)/obj/$(ARCH)
LIB = $(BUILD)/lib/$(ARCH)

DEBUG =

.PHONY: all mkdirs debug clean

all: mkdirs $(BIN)/client

mkdirs:
	mkdir -p $(BIN) $(OBJ) $(LIB)

debug: debug_flag all

debug_flag:
	$(eval DEBUG = -D_DEBUG)

clean:
	rm -rf ./build ./client

lora: _lora mkdirs $(OBJ)/lora_trx.o $(BIN)/client

_lora:
	$(eval LINK_LORA = -lwiringPi)
	$(eval LORA_OBJ = $(OBJ)/lora_trx.o)
	$(eval LORA_GATEWAY = -DLORA_GATEWAY)


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
	$(CROSSCOMPILE)$(CC) $(CPPFLAGS) -c $(LORA_GATEWAY) $(DEBUG) -o $@ $(INCLUDE) $<

$(OBJ)/lora_trx.o: lora_trx.cpp
	$(CROSSCOMPILE)$(CC) -Wall -Wextra -std=c++11 --pedantic -g -c $(LORA_GATEWAY) $(DEBUG) -o $@ $(INCLUDE) $<

$(OBJ)/misc.o: misc.cpp
	$(CROSSCOMPILE)$(CC) $(CPPFLAGS) -c $(LORA_GATEWAY) $(DEBUG) -o $@ $(INCLUDE) $<

$(BIN)/client: $(OBJ)/client.o $(OBJ)/misc.o $(LIB)/libblockchainsec.a
	$(CROSSCOMPILE)$(CC) $(CPPFLAGS) -o $@ \
		$(OBJ)/client.o \
		$(LORA_OBJ) \
		$(OBJ)/misc.o \
		-L $(LIB) \
		-lblockchainsec -lconfig++ -lpthread -lboost_system $(LINK_LORA)
	cp ./*.sol ./*.conf $(BIN)/
	ln -fs $@ ./client

