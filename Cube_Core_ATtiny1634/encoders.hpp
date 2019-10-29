/*
   Cube11Paths Core Encoder Transmitter - AVR ATtiny1634
   (c)ElevenPaths 2017 jorge.rivera@11paths.com 

   FILE: "encoder.hpp" -> Rotary Encoders definitions. Require serveral changes on RotaryEncoder library.

               ATtiny1634R
         +---------------------+
 PCINT8  |~[ 1] D0/TX D16 [20]~|   PCINT10 - MOSI - SDA
 PCINT7  |~[ 2] D1    D15 [19]~|   PCINT10 - MISO 
 PCINT6  |~[ 3] D2    D14 [18]~|   PCINT11
 PCINT5  |~[ 4] D3    D13 [17]~|   PCINT12
 PCINT4  |~[ 5] D4    D12 [16]~|   PCINT13 - SDK - SCL
 PCINT3  |~[ 6] D5    D11 [15]~|   PCINT14 - INT0
 PCINT2  | [ 7] D6    RST [14] |   PCINT15 - RESET
 PCINT1  | [ 8] D7    D10 [13] |   PCINT16
 PCINT0  | [ 9] D8    D9  [12] |   PCINT17
         | [10] GND   VCC [11] |   
         +---------------------+

• Pin Change Interrupt 0 (PCI0): triggers if any enabled PCINT0 [07:00] pin toggles PORT A
• Pin Change Interrupt 1 (PCI1): triggers if any enabled PCINT1 [11:08] pin toggles PORT B
• Pin Change Interrupt 2 (PCI2): triggers if any enabled PCINT2 [17:12] pin toggles PORT C

------------------------------------------------------------------
Real      External               Digital Pin Change 
Pin       Interrupt Port         Pin     Interrupt Port ISP/UART   ENCODER/USE
--------------------------------------------------------------------------------- PCINT0 [07:00] pin toggles 7 (6) (5) (4) (3) (2) (1) 0
 2                               1       PCINT7    PA7  RX0        RFRX    (unused but needed by VirtualWire library vw_set_rx_pin(RFRX_PIN);)         
 3                               2       PCINT6    PA6             ENC-3-A
 4                               3       PCINT5    PA5             ENC-3-B
 5                               4       PCINT4    PA4             ENC-4-A
 6                               5       PCINT3    PA3             ENC-4-B
 7                               6       PCINT2    PA2             ENC-5-A
 8                               7       PCINT1    PA1             ENC-5-B
 9                               8       PCINT0    PA0             RFPTT    -
 --------------------------------------------------------------------------------- PCINT2 [17:12] pin toggles (17) (16) 15* (14) (13) 12  (16&14->Enc0) (12&17->Enc1)                                                             
12                               9       PCINT17   PC5             ENC-1-A
13                              10       PCINT16   PC4             ENC-1-B antes ENC-0-B
14                              17       PCINT15   PC3  RESET              (UNAVAILABLE)                          
15        INT0     PC2          11       PCINT14   PC2             ENC-0-A
16                              12       PCINT13   PC1  SCK  - SCL         (UNAVAILABLE)  
17                              13       PCINT12   PC0             ENC-0-B antes ENC-1-B
---------------------------------------------------------------------------------  PCINT1 [11:08] pin toggles XX  XX  XX   XX  (11)  (10) 09 08  // 4 bits (10&11->Enc2)                
18                              14       PCINT11   PB3             ENC-2-B               
19                              15       PCINT10   PB2  MISO       ENC-2-A
20                              16       PCINT9    PB1  MOSI - SDA                       
 1                               0       PCINT8    PB0  TX0
--------------------------------------------------------------------------------
*/

// Registers PCMSK0, PCMSK1, and PCMSK2 control which pins contribute to the pin change interrupts.
#define PCMSK0_MASK 0b01111110;            // triggers if any enabled PCINT0 [07:00] pin toggles  7 (6) (5)  (4)  (3)  (2)  (1)  0  //  8 bits // Enc-3, Enc-4, Enc-5
#define PCMSK1_MASK 0b00001100;            // triggers if any enabled PCINT1 [11:08] pin toggles XX  XX  XX   XX (11) (10)   9   8  //  4 bits (10&11->Enc2)                     
#define PCMSK2_MASK 0b00110101;            // triggers if any enabled PCINT2 [17:12] pin toggles XX  XX (17) (16) 15* (14)  13 (12) //  6 bits (16&14->Enc0) (13&17->Enc1)          
// -------------------------------------------- //

#include "RotaryEncoder.h" // https://github.com/mathertel/RotaryEncoder  Require serveral changes!!!!

#if !defined(ENCODER_STEPS) //  24  // STEP FOR REVOLUTION
    #error "ENCODER_STEPS must be defined in RotaryEncoder.h"
#endif

#define  ENCODER_0_A 11 // REAL 15-B
#define  ENCODER_0_B 13 // REAL 17-A  

#define  ENCODER_1_A  9 // REAL 12-B    
#define  ENCODER_1_B 10 // REAL 13-A       

#define  ENCODER_2_A 15 // REAL 19-B
#define  ENCODER_2_B 14 // REAL 18-A 

#define  ENCODER_3_A  2 // REAL  3-B
#define  ENCODER_3_B  3 // REAL  4-A

#define  ENCODER_4_A  4 // REAL  5-B
#define  ENCODER_4_B  5 // REAL  6-A

#define  ENCODER_5_A  6 // REAL  7-B  
#define  ENCODER_5_B  7 // REAl  8-A  

//--------------------------------------------------------------------------------//

RotaryEncoder encoder0(ENCODER_0_A, ENCODER_0_B);
RotaryEncoder encoder1(ENCODER_1_A, ENCODER_1_B); 
RotaryEncoder encoder2(ENCODER_2_A, ENCODER_2_B); 
RotaryEncoder encoder3(ENCODER_3_A, ENCODER_3_B); 
RotaryEncoder encoder4(ENCODER_4_A, ENCODER_4_B); 
RotaryEncoder encoder5(ENCODER_5_A, ENCODER_5_B);

typedef struct {
   RotaryEncoder  *encoder;
   int8_t          newPosition = 0; 
   int8_t          position    = 0; 
   int8_t          cardinal    = 0;
}structSide_t;

#if !defined(SIDES)
  #define      SIDES  6
#endif

structSide_t side[SIDES];          // Array of side struct

//--------------------------------------------------------------------------------//

ISR(PCINT0_vect) {  // triggers if any enabled 
  
      sideProcess(3);   // encoder 3 
      sideProcess(4);   // encoder 4
      sideProcess(5);   // encoder 5 
}

ISR(PCINT1_vect) {  // triggers if any enabled 

      sideProcess(2);   // encoder 2
}

ISR(PCINT2_vect) { // triggers if any enabled 
  
      sideProcess(0);   // encoder 0 
      sideProcess(1);   // encoder 1
} 

// -------------------------------------------- //

void sideProcess(const uint8_t sideCode){

  wake_time = millis()+STANDBY_TIME; 
   
  if (side[sideCode].encoder->tick()){ // return true if encoder have a new value after tick()

      side[sideCode].newPosition = side[sideCode].encoder->getPosition(); // if defined ENCODER_STEPS to 24 values from -23 to 23 
      
      if (side[sideCode].newPosition != side[sideCode].position ){   // if step position change 
    
          side[sideCode].position = side[sideCode].newPosition;      // store newPosition as position

          int8_t new_cardinal = side[sideCode].encoder->isCardinalRange();   

          if (new_cardinal >= 0 and side[sideCode].cardinal != new_cardinal ){

              int8_t turn = new_cardinal - side[sideCode].cardinal ; // compute turn as cardinal diference // ( 0, 6, 12, 18 ) - ( 0, 6, 12, 18 ) = ( -18, -12, -6, +6, +12, +18)
               
              side[sideCode].cardinal  = new_cardinal;

              //if cardinal diference of -15 and 15 convert to 5 and -5 
              if (turn == CARDINAL_STEP *  3) turn = -CARDINAL_STEP; // if turn =  18 --> turn = -6
              if (turn == CARDINAL_STEP * -3) turn =  CARDINAL_STEP; // if turn = -18 --> turn =  6
          
              if (turn<0) turn=((-1*turn)+(CARDINAL_STEP*2));  // convert -5 to 15 and -10 to 20  
                     
              turn = (turn-CARDINAL_STEP)/CARDINAL_STEP; // covert 5,10,15,20 to 0,1,2,3 OR 6,12,18,24 to 0,1,2,3    

              // Matrix index = (  0 ,                     1 ,             2 ,                     3 ) //
              //      {TurnRight (CW), Double TurnRight (2CW), TurnLeft (CCW), Double TurnLeft (2CCW)} //
              
              sendTurnCode(sideCode,turn); // send turn code index (0,1,2,3) 
             
          }else{
            // new position is not a cardinal point: do nothing
            // only for debug
          }
      }else{ 
         // no position change: do nothing
         // only for debug
      } 
  } // no tick() 
} // end sideProcess()

// ----------------------------------------------- //
// EOF

