#include <nodeRelay.h>
#include "Arduino.h"
int _relayPin;
int _buttonPin;
bool _relayState;
nodeRelay::nodeRelay(int relayPin) {

  pinMode(relayPin, OUTPUT);     // Initialize the relay pin as an output
  digitalWrite(relayPin,LOW);
  _relayPin = relayPin;
}
nodeRelay::nodeRelay(int relayPin,int buttonPin) {
  pinMode(relayPin, OUTPUT);     // Initialize the relay pin as an output
  pinMode(buttonPin, INPUT_PULLUP);     // Initialize the relay pin as an INPUT_PULLUP
  digitalWrite(relayPin,HIGH);
  _relayPin = relayPin;
  _buttonPin = buttonPin;
}
void nodeRelay::relay(char mychar){ //funziona al contrario mettendo a zero il positivo
  if (mychar == '1') 
  {
    digitalWrite(_relayPin, LOW);   // Turn the LED on (Note that LOW is the voltage level
    //Serial.println("relayPin -> LOW");
    _relayState = 0;
  } else if (mychar == '0') 
  {
    digitalWrite(_relayPin, HIGH);  // Turn the LED off by making the voltage HIGH
    //Serial.println("relayPin -> HIGH");
    _relayState = 1;
  }
}
void nodeRelay::relay(uint8_t mybyte){ //funziona al contrario mettendo a zero il positivo
if (mybyte == 1 || mybyte == '1') {
    digitalWrite(_relayPin, LOW);
    _relayState = 0;
  } else if (mybyte == 0 || mybyte == '0') { 
    digitalWrite(_relayPin, HIGH);
    _relayState = 1;
  }
}
bool nodeRelay::relayState(){
  return _relayState;
}
