// -----
// RotaryEncoder.cpp - Library for using rotary encoders.
// This class is implemented for use with the Arduino environment.
// Copyright (c) by Matthias Hertel, http://www.mathertel.de
// This work is licensed under a BSD style license. See http://www.mathertel.de/License.aspx
// More information on: http://www.mathertel.de/Arduino
// -----
// 18.01.2014 created by Matthias Hertel
// 17.06.2015 minor updates.
//
// 2017 jorge.rivera@11paths.com change pin setup to INPUT_PULLUP
// 2017 jorge.rivera@11paths.com change several int16_t to init8_t for memory saves
// -----

#include "Arduino.h"
#include "RotaryEncoder.h"

// The array holds the values –1 for the entries where a position was decremented,
// a 1 for the entries where the position was incremented
// and 0 in all the other (no change or not valid) cases.

const int8_t KNOBDIR[] = {
  0, -1,  1,  0,
  1,  0,  0, -1,
 -1,  0,  0,  1,
  0,  1, -1,  0  
};

// positions: [3] 1 0 2 [3] 1 0 2 [3]
// [3] is the positions where my rotary switch detends
// ==> right, count up
// <== left,  count down

// ----- Initialization and Default Values -----


RotaryEncoder::RotaryEncoder(uint8_t pin1, uint8_t pin2) { // jorge.rivera@11paths.com change to int8_t
  
  // Remember Hardware Setup
  _pin1 = pin1;
  _pin2 = pin2;
  
  // Setup the input pins
  //pinMode(pin1, INPUT);
  //digitalWrite(pin1, HIGH);   // turn on pullup resistor

  pinMode(pin1,INPUT_PULLUP);  // jorge.rivera@11paths.com chang


  //pinMode(pin2, INPUT);
  //digitalWrite(pin2, HIGH);   // turn on pullup resistor

  pinMode(pin2,INPUT_PULLUP);   // jorge.rivera@11paths.com chang

  // when not started in motion, the current state of the encoder should be 3
  _oldState = 3;

  // start with position 0;
  _position = 0;
  _positionExt = 0;
} // RotaryEncoder()


int8_t RotaryEncoder::getPosition() { // jorge.rivera@11paths.com change to int8_t
  return _positionExt;
} // getPosition()


void RotaryEncoder::setPosition(int8_t newPosition) { // jorge.rivera@11paths.com change to int8_t
  // only adjust the external part of the position.
  _position = ((newPosition<<2) | (_position & 0x03L));
  _positionExt = newPosition;
} // setPosition()


boolean RotaryEncoder::tick(void) //jorge.rivera@11paths.com change from void to boolean
{
  uint8_t sig1 = digitalRead(_pin1);      // jorge.rivera@11paths.com change to int8_t
  uint8_t sig2 = digitalRead(_pin2);      // jorge.rivera@11paths.com change to int8_t

  uint8_t thisState = sig1 | (sig2 << 1); // jorge.rivera@11paths.com change to int8_t

  if (_oldState != thisState) {

    _position += KNOBDIR[thisState | (_oldState<<2)];
    
    if (thisState == LATCHSTATE){
      _positionExt = _position >> 2;
    }

#if defined(ENCODER_STEPS)  // jorge.rivera@11paths.com addon
    if ((_positionExt == ENCODER_STEPS) or (_positionExt == -ENCODER_STEPS )) {
        _position = 0;     // ((0<<2) | (_position & 0x03L));
        _positionExt = 0;     
    } 
#endif
    
    _oldState = thisState;
    return true; // jorge.rivera@11paths.com addon
  } // if
  return false;  // jorge.rivera@11paths.com addon
} // tick()

#if defined(ENCODER_STEPS) 

boolean RotaryEncoder::isCardinal(){ 
  if (( _positionExt % (CARDINAL_STEP)) == 0 ) return true; 
  return false;
}

int8_t RotaryEncoder::isCardinalRange(uint8_t range){  // jorge.rivera@11paths.com change addon
  
    int8_t       i = 0;
    int8_t rel_pos = _positionExt; // jorge.rivera@11paths.com change to must be signed!!!

    if ( rel_pos<0 ) rel_pos+=ENCODER_STEPS; 
   
    while  (i <= ENCODER_STEPS){ 
      if ( abs(rel_pos - i)  <= range){
        return (i == ENCODER_STEPS ? 0 : i) ;     // jorge.rivera@11paths.com addon cardirnal point 0, 6, 12 or 18, or -1 if encoder position it's out of range.
      }else{  
         i=i+CARDINAL_STEP;
      }
    }
    return -1;
}

#endif
// ----------------------------------------------- //
// EOF
