
ARDUINO_PACKAGE_DIR = /home/kale/.arduino15/packages
ARDUINO_DIR   = /home/kale/Downloads/arduino-1.8.11
ARDMK_DIR     = ../Arduino-Makefile
# AVR_TOOLS_DIR = /usr/include
MONITOR_PORT  = /dev/ttyACM0
ARDUINO_PORT  = /dev/ttyACM0
BOARD_TAG     = adafruit_feather_m0
TOOL_PREFIX   = arm-none-eabi
ALTERNATE_CORE_PATH = /home/kale/.arduino15/packages/adafruit/hardware/samd/1.5.9
ARDUINO_LIBS  = SPI Adafruit_ZeroDMA BlockchainSecRadioHead

AVRDUDE      = /usr/bin/avrdude
AVRDUDE_CONF = /etc/avrdude.conf
CPPFLAGS += -fno-fat-lto-objects
#CPPFLAGS += -ffat-lto-objects -fuse-linker-plugin
#CORE_OBJS += --use-linker-plugin --plugin /home/kale/.arduino15/packages/arduino/tools/arm-none-eabi-gcc/7-2017q4/lib/gcc/arm-none-eabi/7.2.1/liblto_plugin.so
CPPFLAGS += -I ../../RadioHead \
			-I ../../libsodium/src/libsodium/include/sodium \
			-I ../../libsodium/src/libsodium/crypto_scalarmult/curve25519 \
			-I ../../libsodium/src/libsodium/crypto_scalarmult/curve25519/ref10
LDFLAGS = -L /home/kale/Desktop/CMPT400/dev/blockchain-iot-security/libsodium/build-cortex-m0/lib -lsodium
MONITOR_BAUDRATE = 115200

include ../Arduino-Makefile/Sam.mk
