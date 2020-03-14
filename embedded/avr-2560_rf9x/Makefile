
ARDUINO_PACKAGE_DIR = /home/kale/.arduino15/packages
ARDUINO_DIR   = /home/kale/Downloads/arduino-1.8.11
ARDMK_DIR     = ../Arduino-Makefile
# AVR_TOOLS_DIR = /usr/include
MONITOR_PORT  = /dev/ttyACM0
ARDUINO_PORT  = /dev/ttyACM0
BOARD_TAG     = mega
TOOL_PREFIX   = avr
ALTERNATE_CORE_PATH = /home/kale/Downloads/arduino-1.8.11/hardware/arduino/avr
ARDUINO_LIBS  = SPI BlockchainSecRadioHead
MCU = atmega2560
AVRDUDE      = /usr/bin/avrdude
AVRDUDE_CONF = /etc/avrdude.conf
CFLAGS = -Os
#-Wl,-s
LDFLAGS=-Wl,--gc-sections -Wl,--print-gc-sections -Wl,-Map=/home/kale/Desktop/CMPT400/dev/blockchain-iot-security/embedded/avr-2560_rf9x/build-mega/link.map
#LDFLAGS =  -Wl,--gc-sections -Wl,--print-gc-sections -Wl,-s
CPPFLAGS += -fno-fat-lto-objects
#CPPFLAGS += -ffat-lto-objects -fuse-linker-plugin
#CORE_OBJS += --use-linker-plugin --plugin /home/kale/.arduino15/packages/arduino/tools/arm-none-eabi-gcc/7-2017q4/lib/gcc/arm-none-eabi/7.2.1/liblto_plugin.so
CPPFLAGS += -I ../../RadioHead \
			-I ../../libsodium/src/libsodium/include/sodium \
			-I ../../libsodium/src/libsodium/crypto_scalarmult/curve25519 \
			-I ../../libsodium/src/libsodium/crypto_scalarmult/curve25519/ref10
#LDFLAGS = -L /home/kale/Desktop/CMPT400/dev/blockchain-iot-security/libsodium/build-avr-2560/lib -lsodium
#LDFLAGS = /home/kale/Desktop/CMPT400/dev/blockchain-iot-security/libsodium/build-avr-2560/lib/libsodium.a
#LINKER_SCRIPTS = /home/kale/Desktop/CMPT400/dev/blockchain-iot-security/libsodium/build-avr-2560/lib/libsodium.a


# -L /home/kale/Downloads/arduino-1.8.11/hardware/tools/avr/avr/lib
LINKER_SCRIPTS = -L /home/kale/Desktop/CMPT400/dev/blockchain-iot-security/libsodium/build-avr-2560/lib \
			 -lsodium

			#  -L /home/kale/Desktop/CMPT400/dev/blockchain-iot-security/scripts/Arduino-Plus-master/hardware/tools/avr/avr32/lib \
			# -L /home/kale/Desktop/CMPT400/dev/blockchain-iot-security/scripts/avr-2560/temp/lib \
			# -L /home/kale/Desktop/CMPT400/dev/blockchain-iot-security/libsodium/build-avr-2560/lib \
			# -lsodium \
			# -L /home/kale/Downloads/arduino-1.8.11/hardware/tools/avr/lib/avrxmega7 \
			# -L /home/kale/Downloads/arduino-1.8.11/hardware/tools/avr/lib/avrxmega6 \
			# -L /home/kale/Downloads/arduino-1.8.11/hardware/tools/avr/lib/avrxmega5 \
			# -L /home/kale/Downloads/arduino-1.8.11/hardware/tools/avr/lib/avrxmega4 \
			# -L /home/kale/Downloads/arduino-1.8.11/hardware/tools/avr/lib/avrxmega3

MONITOR_BAUDRATE = 115200

include ../Arduino-Makefile/Arduino.mk
