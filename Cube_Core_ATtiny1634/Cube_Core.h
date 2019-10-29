/*
   Cube11Paths Core Encoder Transmitter - AVR ATtiny1634
   (c)ElevenPaths 2017 jorge.rivera@11paths.com 

   FILE: "Cube_Core.h" --> Main definitions and helpers functions.

*/

#pragma GCC optimize ("-O3") 
#pragma GCC push_options     

#define MAYOR_VERSION   0   // (MSB o Most  Significant Bit/Byte)
#define MINOR_VERSION  48   // (LSB o Least Significant Bit/Byte)

#define VERSION_WORD  (uint16_t) (( MAYOR_VERSION <<8) | MINOR_VERSION )           // one byte (1) -> MAYOR, one byte (0) -> MINOR
#define VERSION_FLOAT    (float)  ( MAYOR_VERSION   +  ( MINOR_VERSION / 100.0 ))  // MAYOR dot MINOR

#define CODE_NAME   "Cube11Paths"           
 
#define UART_SPEED                  115200  // If debug only
#define I2C_SLAVE_ADDR                0x08  // 
#define RFPTT_PIN                        8  // Real pin 9 as digital 8 (PA0)
#define WAIT_NRF52                      25  // Millis to wait for nRF52 boot
#define STANDBY_TIME (unsigned long)180000  // Time await to go sleep
#define WIREUpdateTime   (unsigned long)25  // Wait for wire txs
#define DEBUG                               // Change to NO_DEBUG to compile without debug serial outputs

// --------------------------------------------------------------- //

#if !defined(__AVR_ATtiny1634__)
   #error "MCU ATtiny1634 required"
#elif defined(USIDR) // wire review
   #warning "USIDR USIWire_h USI_TWI_Master/USI_TWI_Master.h"
#else
   #warning "USI / WIRE NEED YOUR ATTENTION"
#endif

// --------------------------------------------------------------- //
// Standard libraries from avr-libc http://savannah.nongnu.org/projects/avr-libc/ Version 2.0.0 released 15 Feb 2016
// AVG GCC Compiler v4.9.2 from     https://gcc.gnu.org/wiki/avr-gcc
#include <avr/boot.h>               // boot_lock_fuse_bits_get()
#include <avr/io.h>                 // SIGNATURE_0 SIGNATURE_1 SIGNATURE_2 better than boot_signature_byte_get( 0x00 ), boot_signature_byte_get( 0x02 ), boot_signature_byte_get( 0x04 )
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>

#include <Wire.h>                   // From ATTinyCore v1.2.3 https://github.com/SpenceKonde/ATTinyCore    

// SDA PORT B1 (Real20) Arduino D16 / A6  --> USI Data Input (Two Wire Mode)
// SCL PORT C1 (Real16) Arduino D12 / A10 
#define PIN_I2C_SDA 16
#define PIN_I2C_SCL 12
  
// -------------------------------------------- //

volatile unsigned long    wake_time = 0;        
              uint16_t  sleep_count = 0;        
               int16_t last_vccRead = 0;        
               
boolean                    I2Cready = false;    // Check por I2C SCL pull-up

void sideProcess(const uint8_t sideCode);       // Declared in file encoders.hpp
void sendTurnCode(uint8_t side, int8_t turn);   // Declared in file queue.hpp
// -------------------------------------------- //

#include "prescaler.h"        // https://github.com/fschaefer/Prescaler Require serveral changes to support ATtiny1634.
#include "encoders.hpp"       // Rotary Encoders definitions. Require serveral changes on RotaryEncoder library.
#include "protocol.hpp"       // Communication Protocol definitions and helper functions. 
#include "packet.hpp"         // Packet definitons and helper functions.
#include "queue.hpp"          // Queue definitons and helper functions. Require standart library <RingBuf.h> // https://github.com/wizard97/ArduinoRingBuffer

// --------------------------------------------------------------------------------------------------------------------------------------------------- //
// ISR function for WDT interrupt vector to write a debug message before reset cpu (can be an asm jump 0 to do a software reset)
//
ISR(WDT_vect)
{
  #ifdef DEBUG
    Serial.println("WDT Reset!!!");
    wait_serial();
  #endif

  bitSet(WDTCSR,WDE);    // Force reboot in next wdt overflow
  while(1);              // Wait until reboot
}  

// -------------------------------------------- //
//
#define ADC_DISCARDS  10                             
uint16_t vccRead (uint8_t discard = ADC_DISCARDS) {  

  // ADMUX – ADC Multiplexer Selection Register
  byte savedADMUX = ADMUX ; // Store previus ADMUX

  ADCSRA = 0b10000110; // ADCSRA_saved; //power_adc_enable(); -> 0b10000110 = 0x86 = 134
                       // Bit  7 -  ADEN: ADC Enable Writing this bit to one enables the ADC. By writing it to zero, the ADC is turned off.
                       // Bits ADPS[2:0]: ADC Prescaler Select Bits. These bits determine the division factor between the system clock frequency and the input clock to the ADC

  ADMUX  = 0b00001101; // This value is only valid for ATtiny1634. See datashet.
                       // Bits 7:6 – REFS[1:0]: Reference Selection Bits ( 0 0 Vcc as voltage reference)
                       // Bits 3:0 –  MUX[3:0]: Analog Channel and Gain Selection Bits (1101 Bandgap voltage) 
                      
  delayMicroseconds(2000); // Wait for Vref to settle 

  while (discard-- > 0) {               // Discard previous result
      ADCSRA |= _BV(ADSC);              // Start conversion
      while (bit_is_set(ADCSRA,ADSC));  // Measuring
  }
 
  ADCSRA |= _BV(ADSC);                  // Start conversion
  while (bit_is_set(ADCSRA,ADSC));      // Measuring
  
  ADMUX = savedADMUX;                   // Restore previus ADMUX
                                        // ADCSRA – ADC Control and Status Register A
 
  bitSet(ADCSRA,4);                     // Bit 4 – ADIF ADC Interrupt Flag cleared by writing a logical one to the flag
  ADCSRA = 0;                           // power_adc_disable();
  
  return (uint16_t) (float)((1023.0 * 1.037 * 1000.0) / ADC);  // Compute Vcc (in mV); 1125300 = 1.1 * 1023 * 1000 // CALIBRATED from 1.063 to 1.0655
}

// -------------------------------------------- //
//
uint16_t vccReadMin(uint8_t discard = ADC_DISCARDS){            

  uint16_t tmp_vccRead;
  uint16_t min_vccRead = vccRead();

  for (uint8_t i = 0; i < discard; i++){  
      tmp_vccRead = vccRead();
      if (tmp_vccRead < min_vccRead) min_vccRead = tmp_vccRead;
  }
  return min_vccRead;
}

// -------------------------------------------- //
//
uint16_t vccReadAvg(uint8_t discard = ADC_DISCARDS*2 ){       

  float    avg_vccRead = vccRead(discard);

  for (uint8_t i = 0; i < discard; i++){   
      avg_vccRead = ((avg_vccRead+vccRead(discard))/2.0);
  }
  return uint16_t(avg_vccRead);
}

// -------------------------------------------- //
//
inline int freeRam () 
{
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

// -------------------------------------------- //
//
#ifdef DEBUG
inline void wait_serial(void){      // http://forum.arduino.cc/index.php?topic=151014.0
  Serial.flush(); 
  while (!(UCSR0A & (1 << UDRE0)))  // Wait for empty transmit buffer
     UCSR0A |= 1 << TXC0;           // mark transmission not complete
  while (!(UCSR0A & (1 << TXC0)));  // Wait for the transmission to complete 
} 

// -------------------------------------------- //
//
inline boolean serial_tx_active(void){ 

  if (!(UCSR0A & (1 << UDRE0))){
      UCSR0A |= 1 << TXC0;
      return true;
    
  }else{
      if (!(UCSR0A & (1 << TXC0))){
        return true;  
      }else{
        return false;  
      }
  }
}
#endif 

// ----------------------------------------------- //
// EOF
