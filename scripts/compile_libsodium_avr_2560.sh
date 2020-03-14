#!/bin/sh -x

INCLUDEDIR=`pwd`/Arduino-Plus-master/hardware/tools/avr/avr32/include
# INCLUDEDIR=`pwd`/Arduino-Plus-master/hardware/tools/avr_new/avr/include
LIBDIR=`pwd`/Arduino-Plus-master/hardware/tools/avr/avr32/lib
cd ../libsodium
SODIUMDIR=`pwd`

export PATH=$HOME/Downloads/arduino-1.8.11/hardware/tools/avr/bin:$PATH
export CFLAGS=" -ffunction-sections -fdata-sections -I ${INCLUDEDIR} -Os -mmcu=atmega2560 -DRANDOMBYTES_CUSTOM_IMPLEMENTATION -DRANDOMBYTES_DEFAULT_IMPLEMENTATION=NULL -DNDEBUG -D_NO__ERRNO"
./configure --host=avr --prefix="${SODIUMDIR}/build-avr-2560" && make -j4 install

if [ $? -eq 0 ]
then
	echo libsodium was successfully compiled for AVR 2560 in: ../libsodium/build-avr-2560/lib/libsodium.a
else
	echo Error compiling libsodium for AVR 2560!
fi
