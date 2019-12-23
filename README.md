# esphapcontroller

This is work-in-progress!
This project aims to integrate  https://github.com/Yurik72/ESPHomeController  into native AppleHome Kit support.
Without any additional bridges like (mqtt, raspberry,...)

As well second target is to make friendship between [esp-idf](https://github.com/espressif/esp-idf) and arduino to use favorites libraries developed for arduino.

Many thanks to [maximkulkin](https://github.com/maximkulkin) for providing fine libraries for native integration,
this project used them.



## Usage


This is a application to be used with `Espressif IoT Development Framework (ESP-IDF)` and `arduino-esp32`. 

Please check ESP-IDF docs for getting started instructions and install instructions.

[Espressif IoT Development Framework](https://github.com/espressif/esp-idf)

Once the ESP-IDF is installed:
please check suggestion to use  `esp`  directory as root
1. Run mingw32
2. 
```c
$ cd esp
$ git clone https://github.com/Yurik72/esphapcontroller
$ cd esphapcontroller
$ make -C production/esp32/hapcontroller all
```
! Project already contains arduino-esp libraries for esp-idf and sdk configured.

Hovewer you can
```c
$  make -C production/esp32/hapcontroller menuconfig
```
to reconfigure SDK

and finally you can flash your esp
```c
$ make -C production/esp32/hapcontroller flash
```

## For Arduino IDE and your own development

please have a look [ESPHap](https://github.com/Yurik72/ESPHap) this is library to use native Apple integration allows to use Arduino IDE



