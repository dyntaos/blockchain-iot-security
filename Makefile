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

DEBUG =

.PHONY: all mkdirs debug clean

all: mkdirs $(BIN)/client

mkdirs:
	mkdir -p $(BIN) $(OBJ) $(LIB)

debug: debug_flag all

debug_flag:
	$(eval DEBUG = -D_DEBUG )

clean:
	rm -rf ./build


### Static Library ###

$(OBJ)/blockchainsec.o: blockchainsec.cpp
	$(CROSSCOMPILE)$(CC) -c -fPIC $(DEBUG) -o $@ $(INCLUDE) $<

$(LIB)/libblockchainsec.a: $(OBJ)/blockchainsec.o
	ar rcs $@ $^

### Shared Library ###
#$(LIB)/libblockchainsec.so: $(OBJ)/blockchainsec.o
#	$(CROSSCOMPILE)$(CC) -o $@ -shared -Wl,-soname,libblockchainsec.so $<



### Client Binary ###

$(OBJ)/client.o: client.cpp
	$(CROSSCOMPILE)$(CC) -c $(DEBUG) -o $@ $(INCLUDE) $<

$(BIN)/client: $(OBJ)/client.o $(LIB)/libblockchainsec.a
	$(CROSSCOMPILE)$(CC) -o $@ $< -L $(LIB) -lblockchainsec -lconfig++
	cp ./*.sol ./*.conf $(BIN)/
