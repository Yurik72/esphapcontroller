PROJECT_NAME = hapcontroller

CFLAGS += -I$(abspath ../../..) -DHOMEKIT_SHORT_APPLE_UUIDS -DEX_STORAGE_CHAR

EXTRA_COMPONENT_DIRS += \
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


	extras/http-parser \
	$(abspath ../../components/esp-8266/cJSON) \
	$(abspath ../../components/common/wolfssl) \
	$(abspath ../../components/common/homekit)
	
include $(IDF_PATH)/make/project.mk
