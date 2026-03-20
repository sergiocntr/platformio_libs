#ifndef nodeRelay_h
#define nodeRelay_h
#include "Arduino.h"
//#include <ESP8266WiFi.h>
//#include <Bounce2.h>
class nodeRelay
{
  public:
    nodeRelay(int relayPin);
    nodeRelay(int relayPin,int buttonPin);
    void relay(char mychar);
    void relay(uint8_t mybyte);
    bool relayState();

  private:
    int _relayPin;
    int _buttonPin;
    bool _relayState;

};
#endif
