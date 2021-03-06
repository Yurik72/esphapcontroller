#include <Arduino.h>
#include <ArduinoJson.h>

#include "config.h"

#include "Utilities.h"
#include "BaseController.h"
#include "RelayDimController.h"

#ifdef ESP32
#include "esp32-hal-ledc.h"
//#include "WS2812Driver.h"
#endif

//REGISTER_CONTROLLER(RelayDimController)

const size_t bufferSize = JSON_OBJECT_SIZE(20);

RelayDimController::RelayDimController() {
	this->isinvert = false;
	this->pin = 0;
	this->pSmooth = new CSmoothVal();
#ifdef ESP32
	//.this->channel= get_next_available_channel ();
	this->channel=get_next_espchannel();
#endif
#ifdef	ENABLE_NATIVE_HAP
	this->ishap=true;
	this->hapservice=NULL;
	this->hap_on=NULL;
	this->hap_br=NULL;
#endif
}
RelayDimController::~RelayDimController() {
	if (this->pSmooth)
		delete this->pSmooth;
}
String  RelayDimController::serializestate() {

	DynamicJsonDocument jsonBuffer(bufferSize);
	JsonObject root = jsonBuffer.to<JsonObject>();
	root[FPSTR(szisOnText)] = this->get_state().isOn;
	root[FPSTR(szbrightnessText)] = this->get_state().brightness;
	root["isLdr"] = this->get_state().isLdr;
	root["ldrValue"] = this->get_state().ldrValue;

	String json;
	json.reserve(128);
	serializeJson(root, json);

	return json;
}
bool  RelayDimController::deserializestate(String jsonstate, CmdSource src) {

	DynamicJsonDocument jsonBuffer(bufferSize);
	DeserializationError error = deserializeJson(jsonBuffer, jsonstate);
	if (error) {
		DBG_OUTPUT_PORT.print(F("deserialize failed: "));
		DBG_OUTPUT_PORT.println(error.c_str());
		return false;
	}
	JsonObject root = jsonBuffer.as<JsonObject>();
	RelayDimState newState;
	newState.isOn = root[FPSTR(szisOnText)];
	newState.brightness = root[FPSTR(szbrightnessText)];
	newState.isLdr = root["isLdr"];
	newState.ldrValue = root["ldrValue"];
	this->AddCommand(newState, DimSet, src);
	//this->set_state(newState);
	return true;

}
void RelayDimController::loadconfig(JsonObject& json) {
	RelayDim::loadconfig(json);
	pin = json[FPSTR(szPinText)];
	isinvert = json["isinvert"];
	isEnableSmooth= json["issmooth"];
}
void RelayDimController::getdefaultconfig(JsonObject& json) {
	json[FPSTR(szPinText)] = pin;
	json["isinvert"] = isinvert;
	json[FPSTR(szservice)] = "RelayDimController";
	json[FPSTR(szname)] = "Relay";
	json["issmooth"]=false;
	RelayDim::getdefaultconfig(json);
}
void  RelayDimController::setup() {
#ifdef RELAYDIM_DEBUG
	DBG_OUTPUT_PORT.println(F("RelayDimController::setup()"));
#ifdef ESP32
	DBG_OUTPUT_PORT.print(F("Channel:"));
	DBG_OUTPUT_PORT.println(this->channel);
#endif
#endif
#ifdef ESP8266
	pinMode(pin, OUTPUT);
	digitalWrite(pin, this->isinvert ? HIGH : LOW);
#endif
#ifdef ESP32
//	if (gpio_hold_en((gpio_num_t)pin) != ESP_OK) {
//		DBG_OUTPUT_PORT.println("rtc_gpio_hold_en error");
//	}
	ledcSetup(channel, DIM_FREQ, DIM_RESOLUTION);
	ledcAttachPin(pin, channel);
	pinMode(pin, OUTPUT);
	setBrightness(100);
//	if (gpio_hold_en((gpio_num_t)pin) != ESP_OK) {
//		DBG_OUTPUT_PORT.println("rtc_gpio_hold_en error");
//	}
#endif
}

void RelayDimController::run() {
	if (this->isEnableSmooth && pSmooth->isActive())
		return;   ///ignore 
	command cmd;
	while (commands.Dequeue(&cmd)) {
		//DBG_OUTPUT_PORT.print("Process Command ");
		RelayDimState newState = cmd.state;
		switch (cmd.mode) {
		case DimSwitch:
			newState.isOn = !newState.isOn;
			//DBG_OUTPUT_PORT.println("DimSwitch");
			break;
		case DimSet:
		case DimRelayRestore:
			newState.isOn = cmd.state.isOn;
			//DBG_OUTPUT_PORT.println("RelayDimRestore/Set");
			break;
		case DimSetBrigthness:
			newState.brightness = cmd.state.brightness;
			break;
		case DimRelayOn:
			newState.isOn = true;
			newState.brightness = cmd.state.brightness;
			//DBG_OUTPUT_PORT.println("RelayDimOn");
			break;
		case DimSetLdrVal:
			newState.ldrValue = cmd.state.ldrValue;
			break;
		case DimSetIsLdr:
			newState.isLdr = cmd.state.isLdr;
			break;
		case DimRelayOff:
			newState.isOn = false;
			DBG_OUTPUT_PORT.println("RelayDimOff");
		default:break;
		}
		this->set_state(newState);
	}
	RelayDim::run();
}
void RelayDimController::setBrightness(uint8_t br) {
	
#ifdef ESP8266
	analogWrite(pin, DIMCALC_VAL(br, isinvert));
#endif
#ifdef ESP32
	ledcWrite(channel, DIMCALC_VAL(br, isinvert));
#endif
}
int RelayDimController::getLDRBrightness(int brigtness, int ldrval) {
	if (ldrval < 100)
		return 196;
	return ((double)(MAX_LDRVAL - ldrval) / MAX_LDRVAL)*brigtness;
}
void RelayDimController::set_state(RelayDimState state) {
	RelayDimState oldState = this->get_state();
	RelayDimController* self = this;
	
	bool ignore_br = false;
	if (oldState.isOn != state.isOn) {  // on/off
		if (state.isOn) {
			if (this->isEnableSmooth && !pSmooth->isActive()) {
				int brval = state.brightness;
				if (state.isLdr)
					brval = getLDRBrightness(state.brightness, state.ldrValue);
				pSmooth->start(0, brval,
					[self](int val) {
					self->setBrightness(val);
					
				},   //self->setbrightness(val, srcSmooth);},
					[self, state]() {self->AddCommand(state, DimSet, srcSmooth);});
				ignore_br = true;
			}
			else {
				if (this->isEnableSmooth && !pSmooth->isActive()) {

					pSmooth->start(oldState.brightness, 0,
						[self](int val) {
						self->setBrightness(val);
					},
						[self, state]() {
						self->AddCommand(state, DimSet, srcSmooth);
					});
				}
				else
				 this->setBrightness(0);
			}
		}
		else {
			this->setBrightness(0);
		}
	}
	if (state.isOn) {
		if (state.isLdr) {
			if ((oldState.brightness != state.brightness || oldState.ldrValue != state.ldrValue) && !ignore_br)
				this->setBrightness(getLDRBrightness(state.brightness, state.ldrValue));
		}
		else {
			if (oldState.brightness != state.brightness && !ignore_br)
				this->setBrightness(state.brightness);
		}

	}
	RelayDim::set_state(state);
	
	//digitalWrite(pin, (state.isOn ^ this->isinvert) ? HIGH : LOW);

}

bool RelayDimController::onpublishmqtt(String& endkey, String& payload) {
	endkey = szStatusText;
	payload = String(this->get_state().isOn ? 1 : 0);
	return true;
}
bool RelayDimController::onpublishmqttex(String& endkey, String& payload, int topicnr) {
	switch (topicnr) {
	case 0:
		endkey = szStatusText;
		payload = String(this->get_state().isOn ? 1 : 0);
		return true;
	case 1:
		if (!this->get_state().isOn) return false;
		endkey = "Brightness";
		payload = String(this->get_state().brightness);
		return true;
	default:
		return false;
	}

}
void RelayDimController::onmqqtmessage(String topic, String payload) {
#ifdef MQTT_DEBUG
	DBG_OUTPUT_PORT.println(F("RelayDimController MQT"));
	DBG_OUTPUT_PORT.print(F("topic :"));
	DBG_OUTPUT_PORT.println(topic);
	DBG_OUTPUT_PORT.print(F("payload :"));
	DBG_OUTPUT_PORT.println(payload);
#endif
	command setcmd;
	if (topic == F("Set")) {
		command setcmd;
		setcmd.mode = DimSet;
		setcmd.state.isOn = payload == "1";
		this->AddCommand(setcmd.state, setcmd.mode, srcMQTT);
	}

	if (topic.endsWith(F("Brightness"))) {
		setcmd.mode = DimSetBrigthness;
		setcmd.state.brightness = payload.toInt();
		this->AddCommand(setcmd.state, setcmd.mode, srcMQTT);
	}
	
}
#ifdef	ENABLE_NATIVE_HAP
void RelayDimController::setup_hap_service(){


	DBG_OUTPUT_PORT.println("RGBStripController::setup_hap_service()");
	if(!ishap)
		return;

	if(this->accessory_type>1){
		this->hapservice=hap_add_relaydim_service_as_accessory(this->accessory_type,this->get_name(),RelayDimController::hap_callback,this);
	}
	else
	{
		this->hapservice=hap_add_relaydim_service(this->get_name(),RelayDimController::hap_callback,this);
	}

	this->hap_on=homekit_service_characteristic_by_type(this->hapservice, HOMEKIT_CHARACTERISTIC_ON);
	this->hap_br=homekit_service_characteristic_by_type(this->hapservice, HOMEKIT_CHARACTERISTIC_BRIGHTNESS);
}
void RelayDimController::notify_hap(){
	if(this->ishap && this->hapservice){
		DBG_OUTPUT_PORT.println("RGBStripController::notify_hap");

		RelayDimState newState=this->get_state();
		if(this->hap_on->value.bool_value!=newState.isOn){
			this->hap_on->value.bool_value=newState.isOn;
		  homekit_characteristic_notify(this->hap_on,this->hap_on->value);
		}
		if(this->hap_br->value.int_value !=newState.ldrValue){
			this->hap_br->value.int_value=newState.ldrValue;
		  homekit_characteristic_notify(this->hap_br,this->hap_br->value);
		}

	}
}
void RelayDimController::hap_callback(homekit_characteristic_t *ch, homekit_value_t value, void *context){
	DBG_OUTPUT_PORT.println("RGBStripController::hap_callback");

	if(!context){
		return;
	};

	    RelayDimController* ctl= (RelayDimController*)context;
	    RelayDimState newState=ctl->get_state();
	    RelayDimCMD cmd = DimRelayOn;
		bool isSet=false;
		if(ch==ctl->hap_on && ch->value.bool_value!=newState.isOn){
			newState.isOn=ch->value.bool_value;
			cmd =newState.isOn?DimRelayOn:DimRelayOn;
			isSet=true;
		}
		if(ch==ctl->hap_br && ch->value.int_value!=newState.brightness){
			newState.brightness=ch->value.int_value;
			cmd=DimSetBrigthness;
			isSet=true;
		}

	//	newState.isOn=value.bool_value;
		if(isSet)
			ctl->AddCommand(newState, cmd, srcHAP);

}

#endif
