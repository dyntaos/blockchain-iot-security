#!/bin/sh
if [ ! -f ../libsodium/build-cortex-m0/lib/libsodium.a ]
then
	./compile_libsodium_cortex_m0.sh
fi
cd ../embedded/feather-m0_rf9x
make upload && sleep 2 && make monitor
