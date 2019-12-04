//#include <stdio.h>
//#include <esp_wifi.h>
///#include <esp_event_loop.h>
//#include <esp_log.h>
//#include <esp_system.h>
//#include <nvs_flash.h>
//#include <driver/gpio.h>

//#include <freertos/FreeRTOS.h>
//#include <freertos/task.h>



#define MAX_SERVICES 20
extern "C"{
//#include "homeintegration.h"
}

#include "Arduino.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include <esphap_config.h>
#include "../../../../components/esphomecontroller/utilities.h"
//#include "wifi.h"




#if defined ASYNC_WEBSERVER
#include "ESP32WiFiManager.h"
#define USE_EADNS
#else
#include <WiFiManager.h>        //https://github.com/tzapu/WiFiManager
#endif

#ifdef ENABLE_HOMEBRIDGE
#include <AsyncMqttClient.h>
#endif

#include <FS.h>
#if !defined(ESP8266)
#include <SPIFFS.h>
#endif
//#include <ESPHomeController/config.h>

#ifdef ASYNC_WEBSERVER

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

 AsyncWebServer asserver(80);
#include "spiffs_webserver_async.h"

#endif



#if defined ASYNC_WEBSERVER
ESP32WiFiManager* pwifiManager=NULL;
#endif
bool shouldSaveConfig = false;
#ifndef ESP8266
WiFiEventId_t eventID;
#endif


#ifndef ASYNC_WEBSERVER
#if defined(HTTP_OTA)
#if defined(ESP8266)
#include <ESP8266HTTPUpdateServer.h>
 ESP8266HTTPUpdateServer httpUpdater;
#else
#include "ESP32HTTPUpdateServer.h"
 ESP32HTTPUpdateServer httpUpdater;
#endif
#endif
#endif
#if defined(HTTP_OTA) && defined(ASYNC_WEBSERVER)
#include "ESPAsyncUpdateServer.h"
 ESPAsyncHTTPUpdateServer httpUpdater;
#endif
void startwifimanager();

 bool wifidirectconnect(){
	 return false;
 }

void storage_changed(char * szstorage,int size);
void init_hap_storage();

#include "Controllers.h"
Controllers controllers;

void setup(){
	 DBG_OUTPUT_PORT.begin(115200);

	 if (!SPIFFS.begin(true)) {
		 DBG_OUTPUT_PORT.print("SPIFFS Mount failed");
	 }
	 if (!readConfigFS()) {
		 DBG_OUTPUT_PORT.println("Fail To Load config.json ! ");
		 SPIFFS.format();  //compatibility with prev version
	 }

	 WiFi.setHostname(HOSTNAME);
#if defined(ESP8266)
	 startwifimanager();
#else
	 if (!wifidirectconnect())   //this will decrease sketch size
		 startwifimanager();
#endif
#ifdef MDNS_SERVER
	 //setup mdns
	 DBG_OUTPUT_PORT.print("Starting MDNS  host:");
	 DBG_OUTPUT_PORT.println(HOSTNAME);
	 if (!isAPMode) {
		 if (MDNS.begin(HOSTNAME)) {
			// DBG_OUTPUT_PORT.println("MDNS responder started");
			 MDNS.addService("_http", "_tcp", 80);
		 }

	 }
#endif

	 //will setup file browser
#if !defined(ESP8266)
	 const String FILES[] = {  "/index.html", "/js/bundle.min.js.gz","/filebrowse.html" };//"/filebrowse.html"
	 if (!isAPMode){
		 for(int i=0;i<sizeof(FILES)/sizeof(*FILES);i++)
			check_anddownloadfile(szfilesourceroot, FILES[i]);
	 //	downloadnotexistingfiles(szfilesourceroot, filestodownload);
     }
#endif
	DBG_OUTPUT_PORT.println("Wifi setup done");
	if (!isAPMode) {
		DBG_OUTPUT_PORT.println("Setting FILEHANDLES");
		SETUP_FILEHANDLES  ///setup file browser
	}

#if defined(ASYNC_WEBSERVER)
		setExternRebootFlag(&isReboot);
#endif
#if defined (HTTP_OTA) && defined(ASYNC_WEBSERVER)
		if (!isAPMode) {
			httpUpdater.setExternRebootFlag(&isReboot);
			httpUpdater.setup(asserver, "/update");
		}
#endif
#if defined ASYNC_WEBSERVER
	if(!isAPMode){
		asserver.begin();
		DBG_OUTPUT_PORT.println("ASYNC WEBSERVER started");
	}
#endif
	DBG_OUTPUT_PORT.println("starting setup controllers");
	controllers.setup();
	DBG_OUTPUT_PORT.println("controllers.setup() done");


#if defined ASYNC_WEBSERVER
	//if (!isAPMode)
		controllers.setuphandlers(asserver);
#endif
#ifdef	ENABLE_NATIVE_HAP
		//init hap
		// DBG_OUTPUT_PORT.println("initilizing accessory");
		// init_hap_storage();
		// set_callback_storage_change(storage_changed);


		 //init_accessory();
		 //DBG_OUTPUT_PORT.println("init accessory done");
		 //init_homekit_server();
		 //DBG_OUTPUT_PORT.println("init HOME KIT SERVER done");
#endif
}
void loop(){
	 vTaskDelay(10);
	if (isReboot) {
		delay(1000);
		ESP.restart();
		return;
	}
	controllers.handleloops();
}






/// used by WiFi manager
void saveConfigCallback() {
	 shouldSaveConfig = true;
}
#if defined(ESP8266)
void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
	//DBG_OUTPUT_PORT.println("WiFi On Disconnect.");

}
#else
void onWifiDisconnect() {
	//DBG_OUTPUT_PORT.println("WiFi On Disconnect.");
	//controllers.onWifiDisconnect();
}

#endif
void startwifimanager() {



#if defined ASYNC_WEBSERVER
	//DBG_OUTPUT_PORT.println("AsyncWiFiManager");
	//AsyncWiFiManager wifiManager(&asserver, &dns);
	pwifiManager = new ESP32WiFiManager();
#else
	WiFiManager wifiManager;
#endif

#if defined ASYNC_WEBSERVER
	pwifiManager->setAPCallback(configModeCallback);
#else
	wifiManager.setAPCallback(configModeCallback);
#endif

#if !defined ASYNC_WEBSERVER
	WiFiManagerParameter local_host(name_localhost_host, "Local hostname", HOSTNAME, 64);
	wifiManager.addParameter(&local_host);
#else
	AsyncWiFiManagerParameter local_host(name_localhost_host, "Local hostname", HOSTNAME, 64);
	pwifiManager->addParameter(&local_host);
#endif

#if defined ENABLE_HOMEBRIDGE
	//set config save notify callback
#if! defined ASYNC_WEBSERVER
	WiFiManagerParameter hb_mqtt_host("host", "MQTT hostname", mqtt_host, 64);
	WiFiManagerParameter hb_mqtt_port("port", "MQTT port", mqtt_port, 6);
	WiFiManagerParameter hb_mqtt_user("user", "MQTT user", mqtt_user, 32);
	WiFiManagerParameter hb_mqtt_pass("pass", "MQTT pass", mqtt_pass, 32);
	//add all your parameters here
	wifiManager.addParameter(&hb_mqtt_host);
	wifiManager.addParameter(&hb_mqtt_port);
	wifiManager.addParameter(&hb_mqtt_user);
	wifiManager.addParameter(&hb_mqtt_pass);

#else
	AsyncWiFiManagerParameter hb_mqtt_host("host", "MQTT hostname", mqtt_host, 64);
	AsyncWiFiManagerParameter hb_mqtt_port("port", "MQTT port", mqtt_port, 6);
	AsyncWiFiManagerParameter hb_mqtt_user("user", "MQTT user", mqtt_user, 32);
	AsyncWiFiManagerParameter hb_mqtt_pass("pass", "MQTT pass", mqtt_pass, 32);
	pwifiManager->addParameter(&hb_mqtt_host);
	pwifiManager->addParameter(&hb_mqtt_port);
	pwifiManager->addParameter(&hb_mqtt_user);
	pwifiManager->addParameter(&hb_mqtt_pass);

#endif
#endif
#if! defined ASYNC_WEBSERVER
	wifiManager.setSaveConfigCallback(saveConfigCallback);
	wifiManager.setConfigPortalTimeout(CONFIG_PORTAL_TIMEOUT);
#else
	pwifiManager->setSaveConfigCallback(saveConfigCallback);
	pwifiManager->setConfigPortalTimeout(CONFIG_PORTAL_TIMEOUT);
#endif
	//finally let's wait normal wifi connection
	isOffline=false;
#if defined ASYNC_WEBSERVER
	if (!pwifiManager->autoConnect(HOSTNAME,NULL,!isOffline)) {
#else
	if (!wifiManager.autoConnect(HOSTNAME, NULL)) {
#endif
		DBG_OUTPUT_PORT.println("failed to connect and hit timeout");
		//reset and try again, or maybe put it to deep sleep
		if (!isOffline) {
			ESP.restart();
			delay(1000);
		}
		else {
			DBG_OUTPUT_PORT.println("Entering offline mode");
			isAPMode = true;
#if defined ASYNC_WEBSERVER
			pwifiManager->startOfflineApp(HOSTNAME, NULL);
#endif
		}

	}
#if defined(ESP8266)
	if(!isAPMode)
		WiFi.onStationModeDisconnected(onWifiDisconnect);
#else
	eventID = WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
		onWifiDisconnect();
	}, WiFiEvent_t::SYSTEM_EVENT_STA_DISCONNECTED);
#endif
#if ! defined ASYNC_WEBSERVER
	if (shouldSaveConfig) {
		char localHost[32];
		strcpy(localHost, local_host.getValue());
		if (strlen(localHost) > 0);
		strcpy(HOSTNAME, localHost);
#if defined ENABLE_HOMEBRIDGE
		strcpy(mqtt_host, hb_mqtt_host.getValue());
		strcpy(mqtt_port, hb_mqtt_port.getValue());
		strcpy(mqtt_user, hb_mqtt_user.getValue());
		strcpy(mqtt_pass, hb_mqtt_pass.getValue());
#endif
		writeConfigFS(true);

	}
#else
	if (shouldSaveConfig) {
			char localHost[32];
			strcpy(localHost, local_host.getValue());
			if (strlen(localHost) > 0);
			strcpy(HOSTNAME, localHost);
#if defined ENABLE_HOMEBRIDGE
		strcpy(mqtt_host, hb_mqtt_host.getValue());
		strcpy(mqtt_port, hb_mqtt_port.getValue());
		strcpy(mqtt_user, hb_mqtt_user.getValue());
		strcpy(mqtt_pass, hb_mqtt_pass.getValue());
#endif
			writeConfigFS(true);
	}
#endif
	if (!isAPMode) {
		delete pwifiManager;
		pwifiManager = NULL;
	}
	else {
		pwifiManager->cleanParameters();
	}

}


/*
extern "C" {
void app_main(void) {
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );
    setup();
   // gpio_init();

   // wifi_init();
    //init_accessory();
}

}
*/
