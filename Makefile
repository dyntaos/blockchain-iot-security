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
INCLUDE = -I . -I ../gason/src
ARCH = $(shell uname -s)$(shell uname -m)

BUILD = ./build
BIN = $(BUILD)/bin/$(ARCH)
OBJ = $(BUILD)/obj/$(ARCH)
LIB = $(BUILD)/lib/$(ARCH)


.PHONY: all clean

all: mkdirs $(OBJ)/blockchain-sec-lib.o

mkdirs:
	mkdir -p $(BIN) $(OBJ) $(LIB)

clean:
	rm -rf ./build




$(OBJ)/blockchain-sec-lib.o: blockchain-sec-lib.cpp
	$(CROSSCOMPILE)$(CC) -c -o $@ $(INCLUDE) $<

#$BIN/blockchain-client: blockchain-client.o
#	$(CROSSCOMPILE)$(CC) -o $@ $<
