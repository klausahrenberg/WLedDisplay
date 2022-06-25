#include <Arduino.h>
#include "WNetwork.h"
#include "WLedDisplay.h"

#define APPLICATION "LedDisplay"
#define VERSION "1.25"
#define FLAG_SETTINGS 0x17
#define DEBUG false

#define PIN_STATUS_LED 13 //LED_BUILTIN //13

WNetwork *network;
WLedDisplay *display;

void setup() {
  if (DEBUG) {
		Serial.begin(9600);
	}
	network = new WNetwork(DEBUG, APPLICATION, VERSION, PIN_STATUS_LED, FLAG_SETTINGS);
	display = new WLedDisplay(network);
	network->addDevice(display);
}

void loop() {
  network->loop(millis());
	delay(5);
}
