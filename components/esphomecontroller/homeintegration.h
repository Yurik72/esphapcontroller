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
int hap_initbase_accessory_service(const char* szname_value,const char* szmanufacturer,const char* szserialnumber,const char* szmodels,const char* szfirmware );
homekit_service_t* hap_new_homekit_accessory_service(const char *szname,const char *szserialnumber);
typedef void(*hap_callback)(homekit_characteristic_t *ch, homekit_value_t value, void *context);
homekit_service_t* hap_add_lightbulb_service(const char* szname,hap_callback cb,void* context);
homekit_service_t* hap_new_lightbulb_service(const char* szname,hap_callback cb,void* context);
homekit_service_t* hap_new_lightbulb_dim_service(const char* szname,hap_callback cb,void* context);
homekit_service_t*  hap_add_relaydim_service_as_accessory(int acctype,const char* szname,hap_callback cb,void* context);
homekit_service_t*  hap_add_lightbulb_service_as_accessory(int acctype,const char* szname,hap_callback cb,void* context);

homekit_service_t* hap_add_rgbstrip_service(const char* szname,hap_callback cb,void* context);
homekit_service_t* hap_add_relaydim_service(const char* szname,hap_callback cb,void* context);
homekit_service_t* hap_add_temperature_service(const char* szname);
homekit_service_t* hap_add_humidity_service(const char* szname);
homekit_service_t*  hap_add_temp_hum_as_accessory(int acctype,const char* szname,homekit_service_t** pp_temp,homekit_service_t** pp_hum);

homekit_service_t* hap_add_pressure_service(const char* szname);

homekit_service_t* hap_add_light_service(const char* szname);
homekit_service_t* hap_new_light_service(const char* szname);

homekit_service_t* hap_add_service(homekit_service_t* service );

void hap_setbase_accessorytype(int val);
void hap_init_homekit_server() ;
void hap_init_homekit_base_accessory();
