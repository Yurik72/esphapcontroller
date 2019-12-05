




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
static int hap_accesory_current=0;
#define MAX_HAP_SERVICES 7
homekit_accessory_t *hap_accessories[2];
homekit_service_t* hap_services[MAX_HAP_SERVICES+1]={0};
homekit_server_config_t hap_config = {
    .accessories = hap_accessories,
    .password = "111-11-111"
};
void hap_init_homekit_server() {
	if(hap_accesory_current>1){
		set_callback_storage(on_storage_changed);
		hap_accessories[0] = NEW_HOMEKIT_ACCESSORY(.category=homekit_accessory_category_other, .services=hap_services);
		hap_accessories[1] = NULL;
    	homekit_server_init(&hap_config);
	}else{
		INFO("hap_init_homekit_server nothing to init ");
	}

}
int hap_init_accessory(const char* szname_value,const char* szmanufacturer,const char* szserialnumber,const char* szmodels,const char* szfirmware ){
	hap_services[0]= NEW_HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]) {
	        NEW_HOMEKIT_CHARACTERISTIC(NAME, szname_value),
	        NEW_HOMEKIT_CHARACTERISTIC(MANUFACTURER, szmanufacturer),
	        NEW_HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, szserialnumber),
	        NEW_HOMEKIT_CHARACTERISTIC(MODEL, szmodels),
	        NEW_HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, szfirmware),
	        NEW_HOMEKIT_CHARACTERISTIC(IDENTIFY, identify),
	        NULL
	    });
	hap_accesory_current=1;
	INFO("hap init accessory, next %d",hap_accesory_current);
	return hap_accesory_current;
}
homekit_service_t* hap_add_lightbulb_service(const char* szname,hap_callback cb,void* context){
	if(hap_accesory_current>=MAX_HAP_SERVICES){
		INFO("hap_add_service NOT possible, maximum services reached");
		return NULL;
	}
	hap_services[hap_accesory_current]=NEW_HOMEKIT_SERVICE(LIGHTBULB, .characteristics=(homekit_characteristic_t*[]) {
	            NEW_HOMEKIT_CHARACTERISTIC(NAME, szname),
	            NEW_HOMEKIT_CHARACTERISTIC(
	                ON, true,
	                .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(
	                		cb, .context=context
	                ),
	            ),
	            NULL
	        });
	homekit_service_t * res=hap_services[hap_accesory_current];
	hap_accesory_current++;
	hap_services[hap_accesory_current]=NULL;
	return res;
}
homekit_service_t* hap_add_rgbstrip_service(const char* szname,hap_callback cb,void* context){

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
homekit_service_t* hap_add_relaydim_service(const char* szname,hap_callback cb,void* context,float minval,float maxval){

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
						.min_value= &minval,
						.max_value=&maxval,
					  .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(
					  	cb, .context=context

					  )
					),
	            NULL
	        });
	return hap_add_service(service);
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
homekit_service_t* hap_add_light_service(const char* szname){

	homekit_service_t*service=NEW_HOMEKIT_SERVICE(LIGHT_SENSOR, .characteristics=(homekit_characteristic_t*[]) {
	            NEW_HOMEKIT_CHARACTERISTIC(NAME, szname),
	            NEW_HOMEKIT_CHARACTERISTIC(CURRENT_AMBIENT_LIGHT_LEVEL, 0),
	            NULL
	        });
	return hap_add_service(service);
}
homekit_service_t* hap_add_service(homekit_service_t* service ){
	if(hap_accesory_current>=MAX_HAP_SERVICES){
		INFO("hap_add_service NOT possible, maximum services reached");
		return NULL;
	}
	hap_services[hap_accesory_current]=service;

	hap_accesory_current++;
	hap_services[hap_accesory_current]=NULL;
	INFO("hap_add_service next %d",hap_accesory_current);
	return service;
}


