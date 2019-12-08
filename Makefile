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

all: mkdirs $(BIN)/client

mkdirs:
	mkdir -p $(BIN) $(OBJ) $(LIB)

clean:
	rm -rf ./build


### Static Library ###

$(OBJ)/blockchainsec.o: blockchainsec.cpp
	$(CROSSCOMPILE)$(CC) -c -fPIC -o $@ $(INCLUDE) $<

$(LIB)/libblockchainsec.a: $(OBJ)/blockchainsec.o
	ar rcs $@ $^

### Shared Library ###
#$(LIB)/libblockchainsec.so: $(OBJ)/blockchainsec.o
#	$(CROSSCOMPILE)$(CC) -o $@ -shared -Wl,-soname,libblockchainsec.so $<



### Client Binary ###

$(OBJ)/client.o: client.cpp
	$(CROSSCOMPILE)$(CC) -c -o $@ $(INCLUDE) $<

$(BIN)/client: $(OBJ)/client.o $(LIB)/libblockchainsec.a
	$(CROSSCOMPILE)$(CC) -o $@ $< -L $(LIB) -lblockchainsec
