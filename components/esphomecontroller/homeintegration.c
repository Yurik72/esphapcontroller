




#include "homeintegration.h"

#include <stdio.h>
#include <esp_wifi.h>
#include <esp_event_loop.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <driver/gpio.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>





#include <homekit/homekit.h>
#include <homekit/characteristics.h>
//#include "../../../../components/common/homekit/src/storage.h"
#include <homekit/storage_ex.h>

#define MAX_SERVICES 20
//#define WIFI_SSID "mywifi"
//#define WIFI_PASSWORD "mypassword"
void on_wifi_ready();
/*
esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
        case SYSTEM_EVENT_STA_START:
            printf("STA start\n");
            esp_wifi_connect();
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            printf("WiFI ready\n");
            on_wifi_ready();
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            printf("STA disconnected\n");
            esp_wifi_connect();
            break;
        default:
            break;
    }
    return ESP_OK;
}
*/
/*
static void wifi_init() {
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}
*/
static callback_storagechanged callbackstorage=NULL;
const int led_gpio = 2;

const uint8_t relay_gpios[] = {
  12, 5, 14, 13
};
const size_t relay_count = sizeof(relay_gpios) / sizeof(*relay_gpios);


void relay_write(int relay, bool on) {
    printf("Relay %d %s\n", relay, on ? "ON" : "OFF");
    gpio_set_level(relay, on ? 1 : 0);
}

void led_write(bool on) {
    gpio_set_level(led_gpio, on ? 0 : 1);
}

void gpio_init() {
    gpio_set_direction(led_gpio, GPIO_MODE_OUTPUT);
    led_write(false);

    for (int i=0; i < relay_count; i++) {
        gpio_set_direction(relay_gpios[i], GPIO_MODE_OUTPUT);
        relay_write(relay_gpios[i], true);
    }
}

void identify_task(void *_args) {
    relay_write(relay_gpios[0], true);

    for (int i=0; i<3; i++) {
        relay_write(relay_gpios[0], true);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        relay_write(relay_gpios[0], false);
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }

    relay_write(relay_gpios[0], true);

    vTaskDelete(NULL);
}

void identify(homekit_value_t _value) {
    printf("LED identify\n");
    xTaskCreate(identify_task, "LED identify", 2048, NULL, 2, NULL);
}

void relay_callback(homekit_characteristic_t *ch, homekit_value_t value, void *context) {
    uint8_t *gpio = context;
    relay_write(*gpio, value.bool_value);
}

homekit_accessory_t *accessories[2];


homekit_server_config_t config = {
    .accessories = accessories,
    .password = "111-11-111"
};

void init_accessory() {
    uint8_t macaddr[6];
    esp_read_mac(macaddr, ESP_MAC_WIFI_STA);

    int name_len = snprintf(NULL, 0, "Relays-%02X%02X%02X",
                            macaddr[3], macaddr[4], macaddr[5]);
    char *name_value = malloc(name_len+1);
    snprintf(name_value, name_len+1, "Relays-%02X%02X%02X",
             macaddr[3], macaddr[4], macaddr[5]);

    homekit_service_t* services[MAX_SERVICES + 1];
    homekit_service_t** s = services;

    *(s++) = NEW_HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]) {
        NEW_HOMEKIT_CHARACTERISTIC(NAME, name_value),
        NEW_HOMEKIT_CHARACTERISTIC(MANUFACTURER, "HaPK"),
        NEW_HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "0"),
        NEW_HOMEKIT_CHARACTERISTIC(MODEL, "Relays"),
        NEW_HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "0.1"),
        NEW_HOMEKIT_CHARACTERISTIC(IDENTIFY, identify),
        NULL
    });

    for (int i=0; i < relay_count; i++) {
        int relay_name_len = snprintf(NULL, 0, "Relay %d", i + 1);
        char *relay_name_value = malloc(name_len+1);
        snprintf(relay_name_value, relay_name_len+1, "Relay %d", i + 1);

        *(s++) = NEW_HOMEKIT_SERVICE(LIGHTBULB, .characteristics=(homekit_characteristic_t*[]) {
            NEW_HOMEKIT_CHARACTERISTIC(NAME, relay_name_value),
            NEW_HOMEKIT_CHARACTERISTIC(
                ON, true,
                .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(
                    relay_callback, .context=(void*)&relay_gpios[i]
                ),
            ),
            NULL
        });
    }

    *(s++) = NULL;

    accessories[0] = NEW_HOMEKIT_ACCESSORY(.category=homekit_accessory_category_other, .services=services);
    accessories[1] = NULL;
}

void on_storage_changed(){

	if(callbackstorage)
		callbackstorage(get_ex_storage(),get_ex_storage_size());
}
void init_homekit_server() {
	set_callback_storage(on_storage_changed);
    homekit_server_init(&config);
}
/*
void on_wifi_ready() {
    homekit_server_init(&config);
}
*/
//storage handling


void set_callback_storage_change(callback_storagechanged fn){
	callbackstorage=fn;
}


int hap_get_storage_size_ex(){
	return get_ex_storage_size();
}
int hap_init_storage_ex(char* szdata,int size){
	return init_storage_ex(szdata,size);
}


//ESP Home Controller usage
static int hap_mainservices_current=0;
static int hap_mainaccesories_current=0;
#define MAX_HAP_SERVICES 7
#define MAX_HAP_ACCESSORIES 4
homekit_accessory_t *hap_accessories[MAX_HAP_ACCESSORIES+1]={0};
homekit_service_t* hap_services[MAX_HAP_SERVICES+1]={0};
homekit_server_config_t hap_config = {
    .accessories = hap_accessories,
    .password = "111-11-111"
};
static const char* sz_acc_name=NULL;
static const char* sz_acc_manufacturer=NULL;
static const char* sz_acc_serialnumber=NULL;
static const char* sz_acc_models=NULL;
static const char* sz_acc_firmware=NULL;
static int base_acctype=homekit_accessory_category_other;
static int base_accessory_index=-1;
static bool paired = false;

void hap_init_homekit_server() {
	if(hap_mainservices_current>1){
		set_callback_storage(on_storage_changed);
		 paired = homekit_is_paired();
		 INFO("homekit_is_paired %d",paired);
		 if(base_accessory_index==-1){
			 hap_init_homekit_base_accessory();
		 }else{
			 homekit_accessory_t*old=hap_accessories[base_accessory_index];
			 hap_accessories[base_accessory_index] =
					 NEW_HOMEKIT_ACCESSORY(
					 				.category=(homekit_accessory_category_t)base_acctype,//  homekit_accessory_category_lightbulb,
					 				.services=hap_services);
					// homekit_accessory_clone(hap_accessories[base_accessory_index]);
			 //to do delete old;
		 }
		//hap_accessories[0] = NEW_HOMEKIT_ACCESSORY(
		//		.category=(homekit_accessory_category_t)base_acctype,//  homekit_accessory_category_lightbulb,
		//		.services=hap_services);
		//hap_accessories[hap_mainaccesories_current] = NULL;

    	homekit_server_init(&hap_config);
	}else{
		INFO("hap_init_homekit_server nothing to init ");
	}

}

void hap_init_homekit_base_accessory(){
if(base_accessory_index>=0){
	INFO("base accessory already set ");
	return;
}
hap_accessories[hap_mainaccesories_current] = NEW_HOMEKIT_ACCESSORY(
				.category=(homekit_accessory_category_t)base_acctype,
				.services=hap_services);
base_accessory_index=hap_mainaccesories_current;
hap_mainaccesories_current++;
hap_accessories[hap_mainaccesories_current] = NULL;
INFO("base accessory index %d ",base_accessory_index);
}

void hap_setbase_accessorytype(int val){
	base_acctype=val;
}

int hap_initbase_accessory_service(const char* szname_value,const char* szmanufacturer,const char* szserialnumber,const char* szmodels,const char* szfirmware ){
	sz_acc_name=szname_value;
	sz_acc_manufacturer=szmanufacturer;
	sz_acc_serialnumber=szserialnumber;
	sz_acc_models=szmodels;
	sz_acc_firmware=szfirmware;
	hap_services[0]=hap_new_homekit_accessory_service(szname_value,szserialnumber);

	hap_mainservices_current=1;
	INFO("hap init base accessory service , next %d",hap_mainservices_current);
	return hap_mainservices_current;
}
homekit_service_t* hap_new_homekit_accessory_service(const char *szname,const char * szserialnumber){
	return NEW_HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]) {
		        NEW_HOMEKIT_CHARACTERISTIC(NAME, sz_acc_name),
		        NEW_HOMEKIT_CHARACTERISTIC(MANUFACTURER, sz_acc_manufacturer),
		        NEW_HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, szserialnumber),
		        NEW_HOMEKIT_CHARACTERISTIC(MODEL, sz_acc_models),
		        NEW_HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, sz_acc_firmware),
		        NEW_HOMEKIT_CHARACTERISTIC(IDENTIFY, identify),
		        NULL
		    });
}
homekit_service_t* hap_new_lightbulb_service(const char* szname,hap_callback cb,void* context){

	return NEW_HOMEKIT_SERVICE(LIGHTBULB, .primary = true,.characteristics=(homekit_characteristic_t*[]) {
	            NEW_HOMEKIT_CHARACTERISTIC(NAME, szname),
	            NEW_HOMEKIT_CHARACTERISTIC(
	                ON, true,
	                .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(
	                		cb, .context=context
	                ),
	            ),
	            NULL
	        });


}
homekit_service_t* hap_new_lightbulb_dim_service(const char* szname,hap_callback cb,void* context){

	return NEW_HOMEKIT_SERVICE(LIGHTBULB, .primary = true,.characteristics=(homekit_characteristic_t*[]) {
	            NEW_HOMEKIT_CHARACTERISTIC(NAME, szname),
	            NEW_HOMEKIT_CHARACTERISTIC(
	                ON, true,
	                .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(
	                		cb, .context=context
	                ),
	            ),
				NEW_HOMEKIT_CHARACTERISTIC(
				  BRIGHTNESS, 100,
				  .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(
				  	cb, .context=context
				  	)
				),
	            NULL
	        });


}
homekit_service_t* hap_add_lightbulb_service(const char* szname,hap_callback cb,void* context){

	return hap_add_service(hap_new_lightbulb_service(szname,cb,context));
}
homekit_service_t*  hap_add_lightbulb_service_as_accessory(int acctype,const char* szname,hap_callback cb,void* context){

	homekit_service_t* baseservice=hap_new_homekit_accessory_service(szname,"0");
	homekit_service_t* lbservice= hap_new_lightbulb_service(szname,cb,context);
	homekit_service_t* svc[3];
	svc[0]=baseservice;//hap_new_homekit_accessory_service(szname,"0");
	svc[1]=lbservice;//hap_new_lightbulb_service(szname,cb,context);
	svc[2]=NULL;
	hap_accessories[hap_mainaccesories_current] = NEW_HOMEKIT_ACCESSORY(
			.category=(homekit_accessory_category_t)acctype,
			.services=svc
							);

	hap_mainaccesories_current++;
    hap_accessories[hap_mainaccesories_current] = NULL;
    INFO("added light bulb as accessory , next accessory %d",hap_mainaccesories_current);
return lbservice;
}
homekit_service_t* hap_add_rgbstrip_service(const char* szname,hap_callback cb,void* context){

	homekit_service_t*service=NEW_HOMEKIT_SERVICE(LIGHTBULB,.primary = true, .characteristics=(homekit_characteristic_t*[]) {
	            NEW_HOMEKIT_CHARACTERISTIC(NAME, szname),
	            NEW_HOMEKIT_CHARACTERISTIC(
	                ON, true,
	                .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(
	                		cb, .context=context
	                	)
					),

					NEW_HOMEKIT_CHARACTERISTIC(
					  BRIGHTNESS, 100,
					  .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(
					  	cb, .context=context
					  	)
					),

					NEW_HOMEKIT_CHARACTERISTIC(
		                HUE, 0,
						  .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(
						  	cb, .context=context
						  )
		             ),
					 NEW_HOMEKIT_CHARACTERISTIC(
			                SATURATION, 0,

							  .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(
							  	cb, .context=context
						  )
			          ),

	            NULL
	        });
	return hap_add_service(service);
}
homekit_service_t* hap_add_relaydim_service(const char* szname,hap_callback cb,void* context){

	homekit_service_t*service=NEW_HOMEKIT_SERVICE(LIGHTBULB, .characteristics=(homekit_characteristic_t*[]) {
	            NEW_HOMEKIT_CHARACTERISTIC(NAME, szname),
	            NEW_HOMEKIT_CHARACTERISTIC(
	                ON, true,
	                .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(
	                		cb, .context=context
	                )
					),
					NEW_HOMEKIT_CHARACTERISTIC(
					  BRIGHTNESS, 100,
					  .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(
					  	cb, .context=context

					  )
					),
	            NULL
	        });
	return hap_add_service(service);
}
homekit_service_t*  hap_add_relaydim_service_as_accessory(int acctype,const char* szname,hap_callback cb,void* context){

	homekit_service_t* baseservice=hap_new_homekit_accessory_service(szname,"0");
	homekit_service_t* lbservice= hap_new_lightbulb_dim_service(szname,cb,context);
	homekit_service_t* svc[3];
	svc[0]=baseservice;
	svc[1]=lbservice;
	svc[2]=NULL;
	hap_accessories[hap_mainaccesories_current] = NEW_HOMEKIT_ACCESSORY(
			.category=(homekit_accessory_category_t)acctype,
			.services=svc
							);

	hap_mainaccesories_current++;
    hap_accessories[hap_mainaccesories_current] = NULL;
    INFO("added light bulb as accessory , next accessory %d",hap_mainaccesories_current);
return lbservice;
}
homekit_service_t* hap_add_temperature_service(const char* szname){

	homekit_service_t*service=NEW_HOMEKIT_SERVICE(TEMPERATURE_SENSOR, .characteristics=(homekit_characteristic_t*[]) {
	            NEW_HOMEKIT_CHARACTERISTIC(NAME, szname),
	            NEW_HOMEKIT_CHARACTERISTIC(CURRENT_TEMPERATURE, 0),
	            NULL
	        });
	return hap_add_service(service);
}
homekit_service_t* hap_add_humidity_service(const char* szname){

	homekit_service_t*service=NEW_HOMEKIT_SERVICE(TEMPERATURE_SENSOR, .characteristics=(homekit_characteristic_t*[]) {
	            NEW_HOMEKIT_CHARACTERISTIC(NAME, szname),
	            NEW_HOMEKIT_CHARACTERISTIC(CURRENT_RELATIVE_HUMIDITY, 0),
	            NULL
	        });
	return hap_add_service(service);
}
homekit_service_t*  hap_add_temp_hum_as_accessory(int acctype,const char* szname,homekit_service_t** pp_temp,homekit_service_t** pp_hum){

	homekit_service_t* baseservice=hap_new_homekit_accessory_service(szname,"0");
	homekit_service_t* temp=NEW_HOMEKIT_SERVICE(TEMPERATURE_SENSOR, .characteristics=(homekit_characteristic_t*[]) {
        NEW_HOMEKIT_CHARACTERISTIC(NAME, szname),
        NEW_HOMEKIT_CHARACTERISTIC(CURRENT_TEMPERATURE, 0),
        NULL
    });
	homekit_service_t* hum= NEW_HOMEKIT_SERVICE(TEMPERATURE_SENSOR, .characteristics=(homekit_characteristic_t*[]) {
        NEW_HOMEKIT_CHARACTERISTIC(NAME, szname),
        NEW_HOMEKIT_CHARACTERISTIC(CURRENT_RELATIVE_HUMIDITY, 0),
        NULL
    });
	homekit_service_t* svc[4];
	svc[0]=baseservice;
	svc[1]=temp;
	svc[2]=hum;
	svc[3]=NULL;
	hap_accessories[hap_mainaccesories_current] = NEW_HOMEKIT_ACCESSORY(
			.category=(homekit_accessory_category_t)acctype,
			.services=svc
							);

	hap_mainaccesories_current++;
    hap_accessories[hap_mainaccesories_current] = NULL;
    INFO("add_temp_hum as accessory , next accessory %d",hap_mainaccesories_current);
    if(pp_temp)
    	*pp_temp=temp;
    if(pp_hum)
    	*pp_hum=hum;

return temp;

}
homekit_service_t* hap_new_light_service(const char* szname){
	return NEW_HOMEKIT_SERVICE(LIGHT_SENSOR, .characteristics=(homekit_characteristic_t*[]) {
		            NEW_HOMEKIT_CHARACTERISTIC(NAME, szname),
		            NEW_HOMEKIT_CHARACTERISTIC(CURRENT_AMBIENT_LIGHT_LEVEL, 0),
		            NULL
		        });
}
homekit_service_t* hap_add_light_service(const char* szname){


	return hap_add_service(hap_new_light_service(szname));
}
homekit_service_t* hap_add_service(homekit_service_t* service ){
	if(hap_mainservices_current>=MAX_HAP_SERVICES){
		INFO("hap_add_service NOT possible, maximum services reached");
		return NULL;
	}
	hap_init_homekit_base_accessory();
	hap_services[hap_mainservices_current]=service;

	hap_mainservices_current++;
	hap_services[hap_mainservices_current]=NULL;
	INFO("hap_add_service next %d",hap_mainservices_current);
	return service;
}


