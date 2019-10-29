// -----
// RotaryEncoder.h - Library for using rotary encoders.
// This class is implemented for use with the Arduino environment.
// Copyright (c) by Matthias Hertel, http://www.mathertel.de
// This work is licensed under a BSD style license. See http://www.mathertel.de/License.aspx
// More information on: http://www.mathertel.de/Arduino
// -----
// 18.01.2014 created by Matthias Hertel
//
// 2017 jorge.rivera@11paths.com change pin setup INPUT_PULLUP
// 2017 jorge.rivera@11paths.com change several int16_t to init8_t for memory saves
// -----

#ifndef RotaryEncoder_h
#define RotaryEncoder_h

#include "Arduino.h"

#define LATCHSTATE 3

#define ENCODER_STEPS  24                     // jorge.rivera@11paths.com addon
#define CARDINAL_RANGE  2                     // jorge.rivera@11paths.com to set a cardinal point to send.
#define CARDINAL_STEP  ( ENCODER_STEPS / 4 )  // jorge.rivera@11paths.com addon

class RotaryEncoder
{
public:
  // ----- Constructor -----
  RotaryEncoder(uint8_t pin1, uint8_t pin2);  // jorge.rivera@11paths.com change from int to unit8_t
  
  // retrieve the current position
  int8_t  getPosition();                      // jorge.rivera@11paths.com change to int8_t

  // adjust the current position
  void setPosition(int8_t newPosition);       // jorge.rivera@11paths.com change to int8_t

  // call this function every some milliseconds or by using an interrupt for handling state changes of the rotary encoder.
  boolean tick(void);                         // jorge.rivera@11paths.com change from int to unit8_t from void to boolean

#if defined(ENCODER_STEPS) 
  boolean isCardinal(); 
  int8_t  isCardinalRange(uint8_t range=CARDINAL_RANGE);            // jorge.rivera@11paths.com change addon range
#endif 

private:
  uint8_t _pin1, _pin2; // Arduino pins used for the encoder.       // jorge.rivera@11paths.com change from int to unit8_t
  
  uint8_t _oldState;                                                // jorge.rivera@11paths.com change from int to unit8_t
  
  int8_t _position;     // Internal position (4 times _positionExt) // jorge.rivera@11paths.com change from int to unit8_t
  int8_t _positionExt;  // External position                        // jorge.rivera@11paths.com change from int to unit8_t

};

#endif
// ----------------------------------------------- //
// EOF

