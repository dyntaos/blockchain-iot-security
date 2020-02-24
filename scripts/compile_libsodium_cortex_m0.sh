#!/bin/sh -x
cd ../libsodium
SODIUMDIR=`pwd`
export PATH=$HOME/.arduino15/packages/arduino/tools/arm-none-eabi-gcc/7-2017q4/bin:$PATH
export LDFLAGS='--specs=nosys.specs'
export CFLAGS='-Os -mcpu=cortex-m0plus'
./configure --host=arm-none-eabi --prefix="${SODIUMDIR}/build-cortex-m0" && make install

if [ $? -eq 0 ]
then
	echo libsodium was successfully compiled for ARM Cortex M0 in: ../libsodium/build-cortex-m0/lib/libsodium.a
else
	echo Error compiling libsodium for ARM Cortex M0!
fi
