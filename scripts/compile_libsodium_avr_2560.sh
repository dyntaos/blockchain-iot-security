#!/bin/sh -x
cd ../libsodium
SODIUMDIR=`pwd`
export PATH=$HOME/Downloads/arduino-1.8.11/hardware/tools/avr/bin:$PATH
#export LDFLAGS='--specs=nosys.specs'
export CFLAGS='-Os -mmcu=atmega2560'
./configure --host=avr --prefix="${SODIUMDIR}/build-avr-2560" && make install

if [ $? -eq 0 ]
then
	echo libsodium was successfully compiled for AVR 2560 in: ../libsodium/build-avr-2560/lib/libsodium.a
else
	echo Error compiling libsodium for AVR 2560!
fi
