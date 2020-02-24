#!/bin/sh
if [ ! -f ../libsodium/build-cortex-m0/lib/libsodium.a ]
then
	./compile_libsodium_cortex_m0.sh
fi
cd ../embedded/arduino_rf9x
make upload && sleep 2 && make monitor