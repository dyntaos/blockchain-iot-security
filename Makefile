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

# Disable warnings about future GCC abi changes
CFLAGS = -Wno-psabi
CPPFLAGS = -std=gnu++17 -Wall -Wextra -pedantic -g
LDFLAGS =

JSONINC = ./json/include
CXXOPTSINC = ./cxxopts/include
WIRINGPIINC = ./WiringPi/wiringPi
LIBCONFIGINC = ./libconfig/lib
SODIUMINC = ./libsodium/src/libsodium/include

INCLUDE = -I . -I $(CXXOPTSINC) -I $(JSONINC) -I $(WIRINGPIINC) -I $(LIBCONFIGINC) -I $(SODIUMINC)
ARCH = $(shell uname -s)$(shell uname -m)

BUILD = ./build
BIN = $(BUILD)/bin/$(ARCH)
OBJ = $(BUILD)/obj/$(ARCH)
LIB = $(BUILD)/lib/$(ARCH)

RADIOHEADCFLAGS = -g -DRASPBERRY_PI -DBCM2835_NO_DELAY_COMPATIBILITY -D__BASEFILE__=\"$*\"
RADIOHEADBASE = ./RadioHead
RADIOHEADINC = -I$(RADIOHEADBASE) -I$(RADIOHEADBASE)/examples/raspi


DEBUG =

.PHONY: all mkdirs debug clean lora _lora

all: mkdirs $(BIN)/client $(BIN)/client_blockchain_test $(BIN)/client_data_receiver $(BIN)/client_console

mkdirs:
	mkdir -p $(BIN) $(OBJ) $(LIB)

debug: debug_flag all

debug_flag:
	$(eval DEBUG = -D_DEBUG)

clean:
	rm -rf ./build ./client ./client_blockchain_test ./client_data_receiver ./client_console

lora: _lora mkdirs $(OBJ)/lora_trx.o $(OBJ)/RasPi.o $(OBJ)/RH_RF95.o $(OBJ)/RHDatagram.o $(OBJ)/RHHardwareSPI.o $(OBJ)/RHSPIDriver.o $(OBJ)/RHGenericDriver.o $(OBJ)/RHGenericSPI.o all

_lora:
	$(eval LINK_LORA=-lbcm2835 -lwiringPi )
	$(eval LORA_OBJ=$(OBJ)/lora_trx.o \
					$(OBJ)/RasPi.o \
					$(OBJ)/RH_RF95.o \
					$(OBJ)/RHDatagram.o \
					$(OBJ)/RHHardwareSPI.o \
					$(OBJ)/RHSPIDriver.o \
					$(OBJ)/RHGenericDriver.o \
					$(OBJ)/RHGenericSPI.o )
	$(eval LORA_GATEWAY=-DLORA_GATEWAY )


### RadioHead Library ###

$(OBJ)/RasPi.o: $(RADIOHEADBASE)/RHutil/RasPi.cpp
	$(CC) $(RADIOHEADCFLAGS) -o $@ -c $(RADIOHEADBASE)/RHutil/RasPi.cpp $(RADIOHEADINC)

$(OBJ)/RH_RF95.o: $(RADIOHEADBASE)/RH_RF95.cpp
	$(CC) $(RADIOHEADCFLAGS) -o $@ -c $(RADIOHEADINC) $<

$(OBJ)/RHDatagram.o: $(RADIOHEADBASE)/RHDatagram.cpp
	$(CC) $(RADIOHEADCFLAGS) -o $@ -c $(RADIOHEADINC) $<

$(OBJ)/RHHardwareSPI.o: $(RADIOHEADBASE)/RHHardwareSPI.cpp
	$(CC) $(RADIOHEADCFLAGS) -o $@ -c $(RADIOHEADINC) $<

$(OBJ)/RHSPIDriver.o: $(RADIOHEADBASE)/RHSPIDriver.cpp
	$(CC) $(RADIOHEADCFLAGS) -o $@ -c $(RADIOHEADINC) $<

$(OBJ)/RHGenericDriver.o: $(RADIOHEADBASE)/RHGenericDriver.cpp
	$(CC) $(RADIOHEADCFLAGS) -o $@ -c $(RADIOHEADINC) $<

$(OBJ)/RHGenericSPI.o: $(RADIOHEADBASE)/RHGenericSPI.cpp
	$(CC) $(RADIOHEADCFLAGS) -o $@ -c $(RADIOHEADINC) $<



### Static Library ###   #TODO: Can I remove -fPIC?

$(OBJ)/eth_interface.o: eth_interface.cpp
	$(CROSSCOMPILE)$(CC) $(CFLAGS) $(CPPFLAGS) -c -fPIC $(DEBUG) -o $@ $(INCLUDE) $<

$(OBJ)/blockchainsec.o: blockchainsec.cpp
	$(CROSSCOMPILE)$(CC) $(CFLAGS) $(CPPFLAGS) -c -fPIC $(DEBUG) -o $@ $(INCLUDE) $<

$(OBJ)/data_receiver.o: data_receiver.cpp
	$(CROSSCOMPILE)$(CC) $(CFLAGS) $(CPPFLAGS) -c $(DEBUG) -o $@ $(INCLUDE) $<

$(OBJ)/subscription_server.o: subscription_server.cpp
	$(CROSSCOMPILE)$(CC) $(CFLAGS) $(CPPFLAGS) -c $(DEBUG) -o $@ $(INCLUDE) $<

$(OBJ)/console.o: console.cpp
	$(CROSSCOMPILE)$(CC) $(CFLAGS) $(CPPFLAGS) -c $(DEBUG) -o $@ $(INCLUDE) $<

$(OBJ)/ethabi.o: ethabi.cpp
	$(CROSSCOMPILE)$(CC) $(CFLAGS) $(CPPFLAGS) -c $(DEBUG) -o $@ $(INCLUDE) $<

$(OBJ)/lora_trx.o: lora_trx.cpp
	$(CROSSCOMPILE)$(CC) \
		$(CFLAGS) \
		$(RADIOHEADCFLAGS) \
		-Wall -Wextra --pedantic -g -c \
		$(LORA_GATEWAY) \
		$(DEBUG) \
		-o $@ \
		$(INCLUDE) \
		$(RADIOHEADINC) \
		$<

$(OBJ)/misc.o: misc.cpp
	$(CROSSCOMPILE)$(CC) $(CFLAGS) $(CPPFLAGS) -c $(LORA_GATEWAY) $(DEBUG) -o $@ $(INCLUDE) $<

$(OBJ)/base64.o: cpp-base64/base64.cpp
	$(CROSSCOMPILE)$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $(INCLUDE) $<

$(LIB)/libblockchainsec.a:	$(LORA_OBJ) \
							$(OBJ)/eth_interface.o \
							$(OBJ)/blockchainsec.o \
							$(OBJ)/data_receiver.o \
							$(OBJ)/subscription_server.o \
							$(OBJ)/console.o \
							$(OBJ)/ethabi.o \
							$(OBJ)/misc.o \
							$(OBJ)/base64.o
	ar rcs $@ $(LORA_OBJ) $^




### Client Binary ###

$(OBJ)/client.o: client.cpp
	$(CROSSCOMPILE)$(CC) $(CFLAGS) $(CPPFLAGS) -c $(LORA_GATEWAY) $(DEBUG) -o $@ $(INCLUDE) $(RADIOHEADINC) $<

$(BIN)/client: $(OBJ)/client.o $(LIB)/libblockchainsec.a
	$(CROSSCOMPILE)$(CC) $(CPPFLAGS) -o $@ \
		$(OBJ)/client.o \
		-L $(LIB) \
		-lblockchainsec -lconfig++ -lpthread -lboost_system -lsodium $(LINK_LORA)
	cp ./*.sol ./*.conf $(BIN)/
	ln -fs $@ ./client


$(OBJ)/client_data_receiver.o: client_data_receiver.cpp
	$(CROSSCOMPILE)$(CC) $(CFLAGS) $(CPPFLAGS) -c $(LORA_GATEWAY) $(DEBUG) -o $@ $(INCLUDE) $(RADIOHEADINC) $<

$(BIN)/client_data_receiver: $(OBJ)/client_data_receiver.o $(LIB)/libblockchainsec.a
	$(CROSSCOMPILE)$(CC) $(CPPFLAGS) -o $@ \
		$(OBJ)/client_data_receiver.o \
		-L $(LIB) \
		-lblockchainsec -lconfig++ -lpthread -lboost_system -lsodium $(LINK_LORA)
	cp ./*.sol ./*.conf $(BIN)/
	ln -fs $@ ./client_data_receiver


$(OBJ)/client_blockchain_test.o: client_blockchain_test.cpp
	$(CROSSCOMPILE)$(CC) $(CFLAGS) $(CPPFLAGS) -c $(LORA_GATEWAY) $(DEBUG) -o $@ $(INCLUDE) $(RADIOHEADINC) $<

$(BIN)/client_blockchain_test: $(OBJ)/client_blockchain_test.o $(LIB)/libblockchainsec.a
	$(CROSSCOMPILE)$(CC) $(CPPFLAGS) -o $@ \
		$(OBJ)/client_blockchain_test.o \
		-L $(LIB) \
		-lblockchainsec -lconfig++ -lpthread -lboost_system -lsodium $(LINK_LORA)
	cp ./*.sol ./*.conf $(BIN)/
	ln -fs $@ ./client_blockchain_test


$(OBJ)/client_console.o: client_console.cpp
	$(CROSSCOMPILE)$(CC) $(CFLAGS) $(CPPFLAGS) -c $(LORA_GATEWAY) $(DEBUG) -o $@ $(INCLUDE) $(RADIOHEADINC) $<

$(BIN)/client_console: $(OBJ)/client_console.o $(LIB)/libblockchainsec.a
	$(CROSSCOMPILE)$(CC) $(CPPFLAGS) -o $@ \
		$(OBJ)/client_console.o \
		-L $(LIB) \
		-lblockchainsec -lconfig++ -lpthread -lboost_system -lsodium $(LINK_LORA)
	cp ./*.sol ./*.conf $(BIN)/
	ln -fs $@ ./client_console
