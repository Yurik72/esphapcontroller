PROJECT_NAME = hapcontroller

CFLAGS += -I$(abspath ../../..) -DHOMEKIT_SHORT_APPLE_UUIDS_NO -DEX_STORAGE_CHAR -DHOMEKIT_DEBUG

EXTRA_COMPONENT_DIRS += \
  $(abspath ../../../components/common) \
  $(abspath ../../../components/esp-32) \
  $(abspath ../../../components/OneWire) \
  $(abspath ../../../components/Adafruit_GFX) \
  $(abspath ../../../components/DallasTemp) \
   $(abspath ../../../components/arduino) \
   $(abspath ../../../components/esphomecontroller) \
   $(abspath ../../../components/ESPAsyncWebServer) \
    $(abspath ../../../components/ArduinoJson)

include $(IDF_PATH)/make/project.mk
