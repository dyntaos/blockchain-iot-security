##################################################
##      Blockchain IoT Security Framework       ##
##          University of Saskatchewan          ##
##                     2020                     ##
##----------------------------------------------##
##                  Kale Yuzik                  ##
##               kay851@usask.ca                ##
##################################################


CC = gcc
CROSSCOMPILE =
CFLAGS =
CPPFLAGS = -Wall -Wextra -pedantic
LDFLAGS =
ARCH = $(shell uname -s)$(shell uname -m)

BUILD = ./build/$(ARCH)
BIN = $(BUILD)/bin
OBJ = $(BUILD)/obj
LIB = $(BUILD)/lib


.PHONY: all clean

all: mkdirs
	@echo "$(ARCH)"

mkdirs:
	mkdir -p $(BIN) $(OBJ) $(LIB)

clean:
	rm -rf ./build




blockchain-client.o: blockchain-client.cpp
	$(CROSSCOMPILE)$(CC) -c -o $@ $<

blockchain-client: blockchain-client.o
	$(CROSSCOMPILE)$(CC) -o $@ $<
