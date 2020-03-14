#!/bin/sh
if [ ! -f ../libsodium/build-avr-2560/lib/libsodium.a ]
then
	./compile_libsodium_avr_2560.sh
fi

INCLUDEDIR=`pwd`/avr-2560/temp/include
LIBDIR=`pwd`/avr-2560/temp/lib

cd ../embedded/avr-2560_rf9x
make
#make CFLAGS="-I ${INCLUDEDIR}" LDFLAGS="-L ${LIBDIR}"; # && make upload && sleep 2 && make monitor