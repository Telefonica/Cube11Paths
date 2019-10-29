/*
   Cube11Paths BLE Transceiver - Nordic nRF52832
   (c)ElevenPaths 2018 jorge.rivera@11paths.com 

   See https://github.com/ElevenPaths/Cube11Paths for details.

   Platform Develop: nRF52832 - GT832_C       - Hardware ID: 0x00C7 Package: 0x2000 - Softdevice S132 - Crystal Oscillator -> NRF_CLOCK_LF_SRC_XTAL
   Platform Release: nRF52832 - HolyIOT-17095 - Hardware ID: 0x00E3 Package: 0x2002 - Softdevice S132 - RC/LR   Oscillator -> NRF_CLOCK_LF_SRC_RC
   
   nRF5x    SDK v11.0.0
   SoftDev  S132 v2.0.1
   Arduino   SDK v0.5.1  https://github.com/sandeepmistry/arduino-nRF5             https://sandeepmistry.github.io/arduino-nRF5/package_nRF5_boards_index.json
   BLEPeripheral v0.4.0  https://github.com/sandeepmistry/arduino-BLEPeripheral
   ARM GNU Comp  v5.2.1  20151202 (release) [ARM/embedded-5-branch revision 231848] 5_2-2015q4/bin/arm-none-eabi-g++

Pinout of ST-Link V2 clone programmer (supports SWIM and SWD): STLINK v2 JTAG v17 API v2 SWIM v4 VID 0x0483 PID 0x3748
            +-----+
    RST     | 1  2| SWDIO
    GND     | 3  4| GND
    SWIM      5  6| SWCLK
    3V3     | 7  8| 3V3
    5V      | 9 10| 5V
            +-----+

Pinout of GT832_C - MCU Variant: nRF52832 0x41414230 Package: 0x2000 --> NRF5_DEVICE_DEF(0x00C7, "52832", "QFN48", "B00", 512) 
         +-------------+
         |             |              
         |   GT832_C   | 
         |   CK XTAL   |
   P0_25 )             ( SWDIO
   P0_26 )             ( SWCLK
   P0_27 )          TX-( P0_18
   P0_28 )-RX          ( P0_10
   P0_29 )-SDA         C P0_09
   P0_30 )-SCL         ( GND
   P0_31 )             ( VCC     
         +-------------+

Pinout of HolyIOT-17095 - MCU Variant: nRF52832 0x41414230 Package: 0x2002 -->  NRF5_DEVICE_DEF(0x00E3, "52832", "CIAA", "B0", 512),  // Extracts from openocd/src/flash/nor/nrf5.c
         +-------------+
         |   HOLYIOT   |  
     GND |#   17095   #| SWDCLK
   P0.28 |#           #| SWDIO
   P0.29 |#-SDA       #| P0.21
   P0.30 |#-SCL    TX-#| P0.18
     VCC |#        RX-#| P0.15
         |__#_#_#_#_#__|
            1 0 5 6 8 
            P P P P P

UART & I2C pins definitions in file: /Users/jrivera/Library/Arduino15/packages/sandeepmistry/hardware/nRF5/0.5.1/variants/Generic/variant.h
I2C Slave custom settings in file:   /Users/jrivera/Library/Arduino15/packages/sandeepmistry/hardware/nRF5/0.5.1/libraries/Wire/Wire_nRF52.cpp
NRF_GPIO->PIN_CNF[PIN_WIRE_SDA]:     0b11000001110
NRF_GPIO->PIN_CNF[PIN_WIRE_SCL]:     0b11000001110
*/
// ----------------------------------------------- //

#define  MAYOR_VERSION   0          // (MSB o Most  Significant Bit/Byte)
#define  MINOR_VERSION  30          // (LSB o Least Significant Bit/Byte)

#define  HARD_SERIAL_SPEED 115200   // IMU UART SPEED
#define  I2C_SLAVE_ADDR      0x08   // I2C Slave Address 
#define  DEBUG                      // Define NO_DEBUG to compile without debug serial outputs
#define  IMUupdate            100   // Default time in ms to update IMU values on BLE characteristics
#define  UARTrestart           50   // ms without uart RX >= 8 bytes 

const char* firmFile = __FILE__;

#include <Wire.h>                   // Standart Wire library from Arduino Core for Nordic Semiconductor nRF5 based boards
#include "nRF52.hpp"                // Nordic nRF52832 helper functions.
#include "BLEconfig.hpp"            // Bluetooth LE configurations and helper functions. Require BLEPeripheral.h library from  https://github.com/sandeepmistry/arduino-BLEPeripheral/blob/master/API.md
#include "IMU.hpp"                  // IMU configurations and helper functions.
#include "protocol.hpp"             // Communication Protocol definitions and helper functions.
#include "packet.hpp"               // Packet definitons and helper functions.
#include "CORE.hpp"                 // Core configurations and helper functions.

// -------------------------------------------- //
//
void setup(){
      
    Wire.begin(I2C_SLAVE_ADDR);         // join i2c bus with address #8 

    BLEinit();                          // Configure and init Bluetooth LE as Peripheral (4 ms max on RC oscillator)
 
    Serial.begin(HARD_SERIAL_SPEED);      
   
    #ifdef DEBUG
        #warning "DEBUG ENABLE"
        nRF52info(); 
    #endif 
    
    lastRX = millis()+(UARTrestart*6);  // Store as last time uart rx  //  Addon 6 times UARTrestart for prevent restarts on boot (300ms)

}

// -------------------------------------------- //
//
void loop(){

    BLE.poll();                     

    if ( Serial.available() >= IMUFrameSize ){  

        IMUProcess(); 
        lastRX = millis() + UARTrestart;            // Store as last time uart rx 
    }
    
    if (   Wire.available() >= COREPacketSize ){

           CORERead(); 
    }   

    // Work-around to fix UART stops 
    if ( millis() > lastRX ){                       // Prevent constant UARTrestart (50 ms)  

        IMUerror3++;                                // UART restart counter
        IMUerror3Char.setValue(IMUerror3);          // Update on the fly 
        
        IMUerror4=uint16_t(NRF_UART0->ERRORSRC);    // Last uart error
        IMUerror4Char.setValue(IMUerror4);          // Update on the fly 

        #ifdef DEBUG
            Serial.print("DEBUG: UART RESTART ERRORSRC: 0b"); 
            Serial.println(IMUerror4,BIN);
        #endif 
                                                    // Work-around to fix UART stops
        Serial.end();                               // Stop  UART
        Serial.begin(HARD_SERIAL_SPEED);            // Start UART
                          
        lastRX = millis() + UARTrestart;             // Store as last time uart rx 
    }
 
}


/*
Serial & I2C pins definitions:
FILE: /Users/jrivera/Library/Arduino15/packages/sandeepmistry/hardware/nRF5/0.5.1/variants/Generic/variant.h

GT832_C 
----------------------------------
#define PIN_SERIAL_RX       (18u)  
#define PIN_SERIAL_TX       (28u)  
#define PIN_WIRE_SDA        (29u) 
#define PIN_WIRE_SCL        (30u) 

HOLYIOT-17095 
----------------------------------
#define PIN_SERIAL_RX       (15u)
#define PIN_SERIAL_TX       (18u)
#define PIN_WIRE_SDA        (29u) 
#define PIN_WIRE_SCL        (30u)
*/

/*  
 nRF52 I2C CUSTOM SETTINGS:
 FILE: /Users/jrivera/Library/Arduino15/packages/sandeepmistry/hardware/nRF5/0.5.1/libraries/Wire/Wire_nRF52.cpp

 void TwoWire::begin(uint8_t address) {
  //Slave mode
  master = false;

  NRF_GPIO->PIN_CNF[_uc_pinSCL] = ((uint32_t)GPIO_PIN_CNF_DIR_Input        << GPIO_PIN_CNF_DIR_Pos)     // Pin direction. Same physical register as DIR register. 0 = input

                                | ((uint32_t)GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos)   // Connect or disconnect input buffer. 0 = connect
                                | ((uint32_t)GPIO_PIN_CNF_PULL_Pullup    << GPIO_PIN_CNF_PULL_Pos)      // Pull configuration. 0 = no pull, 1 = down, 3 = up
                                | ((uint32_t)GPIO_PIN_CNF_DRIVE_S0D1       << GPIO_PIN_CNF_DRIVE_Pos)   // Drive configuration. 8 cases: S0S1, H0S1, etc..

                                | ((uint32_t)GPIO_PIN_CNF_SENSE_Disabled   << GPIO_PIN_CNF_SENSE_Pos);  // Pin sensing mechanism. 0 = Disable, 1 = high, 3 = low
  NRF_GPIO->PIN_CNF[_uc_pinSDA] = ((uint32_t)GPIO_PIN_CNF_DIR_Input        << GPIO_PIN_CNF_DIR_Pos)     // Pin direction. Same physical register as DIR register. 0 = input
  
                                | ((uint32_t)GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos)   // Disconnect is ok         //jrivera
                                | ((uint32_t)GPIO_PIN_CNF_PULL_Pullup    << GPIO_PIN_CNF_PULL_Pos)      // Disabled to Pullup       //jrivera
                                | ((uint32_t)GPIO_PIN_CNF_DRIVE_S0D1       << GPIO_PIN_CNF_DRIVE_Pos)   // S0S1 to S0D1 (H0D1 too)  //jrivera
  
                                | ((uint32_t)GPIO_PIN_CNF_SENSE_Disabled   << GPIO_PIN_CNF_SENSE_Pos);   // Pin direction. Same physical register as DIR register. 0 = input

 */

// ----------------------------------------------- //
// SAMPLE DEBUG OUTPUT
/*
Usando librería Wire con versión 1.0 en la carpeta: /Users/jrivera/Library/Arduino15/packages/sandeepmistry/hardware/nRF5/0.5.1/libraries/Wire 
Usando librería BLEPeripheral con versión 0.4.0 en la carpeta: /Users/jrivera/Documents/Arduino/libraries/BLEPeripheral 
El Sketch usa 37780 bytes (9%) del espacio de almacenamiento de programa. El máximo es 409600 bytes.

Cube11Paths BLE v0.30 (c) ElevenPaths 2017-2019 (Feb 19 2019 13:40:41)
FirmwareSRC:  /Users/jrivera/Documents/Arduino/BLE_Transmitter_30/BLE_Transmitter_30.ino
GCC version:  5.2.1 20151202 (release) [ARM/embedded-5-branch revision 231848] IDE (10805)
CPU variant:  nRF52832 0x41414230 Package: 0x2002
Memory:       Flash = 512 Kbytes, RAM = 64 Kbytes, Free = 54355 bytes 
MAC Address:  D0:9A:22:BF:D9:CE Device ID: 913FE7C6E7603F42
PIN_SERIAL_RX: 15
PIN_SERIAL_TX: 18
PIN_WIRE_SDA:  29
PIN_WIRE_SCL:  30
NRF_TWI0->ADDRESS: 0x8
SERIAL_BUFFER_SIZE: 64
NRF_GPIO->PIN_CNF[PIN_WIRE_SDA]: 0b11000001110
NRF_GPIO->PIN_CNF[PIN_WIRE_SCL]: 0b11000001110
IMU update every: 100 ms
UART re-start at: 50 ms

DEBUG: IMU(1) Z=3.7 Y=69.7 Z=-1.1 State: 0,0,0,0
 */
// -------------------------------------------- //
// EOF