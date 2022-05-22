#ifndef W_LED_DISPLAY_H
#define W_LED_DISPLAY_H

#include <Arduino.h>
#include <WMax7219.h>

#define DEVICE_ID "display"

#define MAX_DEVICES	12
#define DIN_PIN     23
#define CLK_PIN     18
#define CS_PIN      15

class WLedDisplay: public WDevice {
public:
	WLedDisplay(WNetwork* network)
		: WDevice(network, DEVICE_ID, network->getIdx(), DEVICE_TYPE_ON_OFF_SWITCH) {
    this->mx = new WMax7219(DIN_PIN, CS_PIN, CLK_PIN, MAX_DEVICES);
		this->addPin(mx);
    //On
		this->onProperty = WProperty::createOnOffProperty("on", "Power");
		this->onProperty->setBoolean(false);
		this->onProperty->setOnChange(std::bind(&WLedDisplay::onPropertyChanged, this, std::placeholders::_1));
		this->addProperty(onProperty);
		//Streamtext
		this->title = WProperty::createStringProperty("title", "Title");
    this->title->setOnChange(std::bind(&WLedDisplay::titleChanged, this, std::placeholders::_1));
		this->addProperty(this->title);
  }

protected:

  void onPropertyChanged(WProperty* property) {
		if (onProperty->getBoolean()) {
			titleChanged(this->title);
		} else {
			mx->clear();
			mx->setCommand(max7219_reg_shutdown, 0x00);
		}
  }

  void titleChanged(WProperty* property) {
		if (onProperty->getBoolean()) {
			mx->setCommand(max7219_reg_shutdown, 0x01);
			mx->showText(this->title->c_str());
		}
  }

private:
  WProperty* onProperty;
  WProperty* title;
	WMax7219* mx;

};

#endif
