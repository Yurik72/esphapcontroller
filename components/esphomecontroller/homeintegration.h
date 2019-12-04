#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <homekit/homekit.h>
#include <homekit/characteristics.h>
#define INFO(message, ...) printf(">>> Home Integration: " message "\n", ##__VA_ARGS__)

void init_accessory();
void init_homekit_server();

typedef void(*callback_storagechanged)(char * szstorage,int size);
void set_callback_storage_change(callback_storagechanged fn);

int hap_init_storage_ex(char* szdata,int size);
int hap_get_storage_size_ex();


//esp controller usage
int hap_init_accessory(const char* szname_value,const char* szmanufacturer,const char* szserialnumber,const char* szmodels,const char* szfirmware );
typedef void(*hap_callback)(homekit_characteristic_t *ch, homekit_value_t value, void *context);
homekit_service_t* hap_add_lightbulb_service(const char* szname,hap_callback cb,void* context);
void hap_init_homekit_server() ;
