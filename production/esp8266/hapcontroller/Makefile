PROGRAM = hapcontroller


EXTRA_COMPONENTS = \
	extras/http-parser \
	$(abspath ../../components/esp-8266/cJSON) \
	$(abspath ../../components/common/wolfssl) \
	$(abspath ../../components/common/homekit) \
  $(abspath ../../../components/OneWire) \
  $(abspath ../../../components/Adafruit_GFX) \
  $(abspath ../../../components/DallasTemp) \
   $(abspath ../../../components/arduino) \
   $(abspath ../../../components/esphomecontroller) \
   $(abspath ../../../components/ESPAsyncWebServer) \
    $(abspath ../../../components/ArduinoJson)
FLASH_SIZE ?= 32

EXTRA_CFLAGS += -I../.. -DHOMEKIT_SHORT_APPLE_UUIDS

include $(SDK_PATH)/common.mk

monitor:
	$(FILTEROUTPUT) --port $(ESPPORT) --baud 115200 --elf $(PROGRAM_OUT)


