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
LIBHYDROGENINC = ./libhydrogen

INCLUDE = -I . -I $(CXXOPTSINC) -I $(JSONINC) -I $(WIRINGPIINC) -I $(LIBCONFIGINC)
ARCH = $(shell uname -s)$(shell uname -m)

BUILD = ./build
BIN = $(BUILD)/bin/$(ARCH)
OBJ = $(BUILD)/obj/$(ARCH)
LIB = $(BUILD)/lib/$(ARCH)

DEBUG =

.PHONY: all mkdirs debug clean

all: mkdirs $(BIN)/client $(BIN)/client_test

mkdirs:
	mkdir -p $(BIN) $(OBJ) $(LIB)

debug: debug_flag all

debug_flag:
	$(eval DEBUG = -D_DEBUG)

clean:
	rm -rf ./build ./client ./client_test

lora: _lora mkdirs $(OBJ)/lora_trx.o $(BIN)/client

_lora:
	$(eval LINK_LORA = -lwiringPi)
	$(eval LORA_OBJ = $(OBJ)/lora_trx.o)
	$(eval LORA_GATEWAY = -DLORA_GATEWAY)


### Static Library ###

$(OBJ)/blockchainsec.o: blockchainsec.cpp
	$(CROSSCOMPILE)$(CC) $(CPPFLAGS) -c -fPIC $(DEBUG) -o $@ $(INCLUDE) $<

$(OBJ)/subscription_server.o: subscription_server.cpp
	$(CROSSCOMPILE)$(CC) $(CPPFLAGS) -c $(DEBUG) -o $@ $(INCLUDE) $<

$(OBJ)/ethabi.o: ethabi.cpp
	$(CROSSCOMPILE)$(CC) $(CPPFLAGS) -c $(DEBUG) -o $@ $(INCLUDE) $<

$(OBJ)/lora_trx.o: lora_trx.cpp
	$(CROSSCOMPILE)$(CC) -Wall -Wextra -std=c++11 --pedantic -g -c $(LORA_GATEWAY) $(DEBUG) -o $@ $(INCLUDE) $<

$(OBJ)/misc.o: misc.cpp
	$(CROSSCOMPILE)$(CC) $(CPPFLAGS) -c $(LORA_GATEWAY) $(DEBUG) -o $@ $(INCLUDE) $<

$(LIB)/libblockchainsec.a: $(OBJ)/blockchainsec.o $(OBJ)/subscription_server.o $(LORA_OBJ) $(OBJ)/ethabi.o $(OBJ)/misc.o
	ar rcs $@ $^





### Client Binary ###

$(OBJ)/client_test.o: client_test.cpp
	$(CROSSCOMPILE)$(CC) $(CPPFLAGS) -c $(LORA_GATEWAY) $(DEBUG) -o $@ $(INCLUDE) $<

$(BIN)/client_test: $(OBJ)/client_test.o $(LIB)/libblockchainsec.a
	$(CROSSCOMPILE)$(CC) $(CPPFLAGS) -o $@ \
		$(OBJ)/client_test.o \
		-L $(LIB) \
		-lblockchainsec -lconfig++ -lpthread -lboost_system -lsodium $(LINK_LORA)
	cp ./*.sol ./*.conf $(BIN)/
	ln -fs $@ ./client_test

$(OBJ)/client.o: client.cpp
	$(CROSSCOMPILE)$(CC) $(CPPFLAGS) -c $(LORA_GATEWAY) $(DEBUG) -o $@ $(INCLUDE) $<

$(BIN)/client: $(OBJ)/client.o $(LIB)/libblockchainsec.a
	$(CROSSCOMPILE)$(CC) $(CPPFLAGS) -o $@ \
		$(OBJ)/client.o \
		-L $(LIB) \
		-lblockchainsec -lconfig++ -lpthread -lboost_system -lsodium $(LINK_LORA)
	cp ./*.sol ./*.conf $(BIN)/
	ln -fs $@ ./client
