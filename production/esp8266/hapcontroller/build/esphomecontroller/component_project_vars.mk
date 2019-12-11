# Automatically generated build file. Do not edit.
COMPONENT_INCLUDES += /home/yurik/esp/esphapcontroller/components/esphomecontroller/include /home/yurik/esp/esphapcontroller/components/ESPAsyncWebServer /home/yurik/esp/esphapcontroller/components/Adafruit_GFX /home/yurik/esp/esphapcontroller/components/Adafruit_NeoPixel /home/yurik/esp/esphapcontroller/components/WS2812FX /home/yurik/esp/esphapcontroller/components/ssd1306 /home/yurik/esp/esphapcontroller/components/DallasTemp /home/yurik/esp/esphapcontroller/components/OneWire /home/yurik/esp/esphapcontroller/components/Adafruit_Sensor /home/yurik/esp/esphapcontroller/components/BME280Adafruit
COMPONENT_LDFLAGS += -L$(BUILD_DIR_BASE)/esphomecontroller -lesphomecontroller
COMPONENT_LINKER_DEPS += 
COMPONENT_SUBMODULES += 
COMPONENT_LIBRARIES += esphomecontroller
COMPONENT_LDFRAGMENTS += 
component-esphomecontroller-build: component-arduino-build
