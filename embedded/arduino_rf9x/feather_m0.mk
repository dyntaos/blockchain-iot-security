
ARDUINO_PACKAGE_DIR = /home/kale/.arduino15/packages
ARDUINO_DIR   = /home/kale/Downloads/arduino-1.8.11
ARDMK_DIR     = ../Arduino-Makefile
# AVR_TOOLS_DIR = /usr/include
MONITOR_PORT  = /dev/ttyACM0
ARDUINO_PORT  = /dev/ttyACM0
BOARD_TAG     = adafruit_feather_m0
TOOL_PREFIX   = arm-none-eabi
ALTERNATE_CORE_PATH = /home/kale/.arduino15/packages/adafruit/hardware/samd/1.5.9
ARDUINO_LIBS  = SPI Adafruit_ZeroDMA
CPPFLAGS += -I ../../RadioHead
#CPPFLAGS += -I ../../libsodium/src/libsodium/sodium
# MONITOR_BAUDRATE =

include ../Arduino-Makefile/Sam.mk
