/*
   Cube11Paths BLE Transceiver - Nordic nRF52832
   (c)ElevenPaths 2018 jorge.rivera@11paths.com 

   File "IMU.hpp" --> IMU configurations and helper functions.
   
*/

// IMU GY-25 MPU-6050
//                       |   Yaw(Z)   |  Pitch(Y)  |   Roll(X)  |
//                 START | HIGH   LOW | HIGH   LOW | HIGH   LOW |  END  --> Calculation method: Angle=((HIGH<<8) |LOW)/100;
// Frame Sample: [  0xAA,  0x00, 0x64,  0x03, 0xE8,  0x27, 0x10,  0x55 ] 
// Frame Angle Decode:     Yaw=+1.00° Pitch=+10.00° Roll=+100.00°       --> Angle values: from -180.00º to +180.00º
// 

#define IMUFrameStart   uint8_t(0xAA) // Preamble Flag 0xAA
#define IMUFrameEnd     uint8_t(0x55) // End      Flag 0x55

#define IMUFrameStartPos     0  //
#define IMUFrameFirstPos     1  //
#define IMUFrameEndPos       7  // 
#define IMUFrameSize (int8_t)8  //

typedef  union  {
   uint8_t  bytes[IMUFrameSize] = {0,0,0,0,0,0,0,0} ;
   struct __attribute__((__packed__)){ // It needed by ARM arch like as Tennsy if packet size must be 8 bytes
         uint8_t    start;
        uint16_t      yaw;
        uint16_t    pitch;
        uint16_t     roll;     
         uint8_t      end;           
   };
   
}IMUFrame_t;

// Global Object
// -------------------------------------------- //
IMUFrame_t IMUBuffer;

// Global vars
// -------------------------------------------- //
uint16_t IMUUpdateEvery  = IMUupdate;           // ms to update IMU values to BLE caracteristic 
uint8_t  IMUBufferPos    = IMUFrameStartPos;
static unsigned long initialtime = 0;          

uint16_t IMUerror1  = 0;      // IMU frame not start as IMUFrameStart
uint16_t IMUerror2  = 0;      // IMU frame not end as IMUFrameEnd 
uint16_t IMUerror3  = 0;      // UART RESTARTS COUNTER
uint16_t IMUerror4  = 0;      // UART ERROR REGISTER (NRF_UART0->ERRORSRC)

boolean UpdateIMUerrors = true; 
unsigned long lastRX = 0;     // Store last rx millis()

static unsigned long lastprint = 0;          

static uint8_t c; 

// Functions
// -------------------------------------------- //
// Byte swap unsigned short
uint16_t swap_uint16( uint16_t val ) 
{
    return (val << 8) | (val >> 8 );
}

// -------------------------------------------- //
// Byte swap short
int16_t swap_int16( int16_t val ) 
{
    return (val << 8) | ((val >> 8) & 0xFF);  // swapped = (num>>8) | (num<<8)
}

// -------------------------------------------- //
//
void IMUProcess(){

  while (Serial.available() > 0){
    
      c = Serial.read();
      
      if ((c == IMUFrameStart) and IMUBufferPos == IMUFrameFirstPos) { 
          IMUBuffer.bytes[IMUFrameStartPos] = c;
      }else{
         if (IMUBufferPos == IMUFrameEndPos){
             IMUBufferPos =  IMUFrameStartPos; 
             if (c == IMUFrameEnd ){ 
                IMUBuffer.bytes[IMUFrameEndPos] = c;
                
                if (initialtime < millis()){
                    initialtime = millis()+IMUUpdateEvery;                      // Set initialtime to next time to update IMU Characteristics
                    imuCharTX.setValue(IMUBuffer.bytes,sizeof(IMUBuffer));      // Update IMU buffer char

                    if (UpdateIMUerrors){                                       // If IMUerrors vars 1 & 2 changed, 3 & 4 updated on the fly
                      IMUerror1Char.setValue(IMUerror1);                        // Update IMUerror1 char
                      IMUerror2Char.setValue(IMUerror2);                        // Update IMUerror1 char
                      UpdateIMUerrors = false;                                  // Clear update flag
                    }

                    #ifdef DEBUG
                      if (millis()/10000 !=  lastprint){ // Output every second
                          lastprint = millis()/10000;
                      
                          Serial.print("DEBUG: IMU("); Serial.print(lastprint,DEC);
                          Serial.print(") Z=");  Serial.print((swap_int16(IMUBuffer.yaw)/100.0),1);
                          Serial.print(" Y=");  Serial.print((swap_int16(IMUBuffer.pitch)/100.0),1);
                          Serial.print(" Z=");  Serial.print((swap_int16(IMUBuffer.roll)/100.0),1);
                          Serial.print(" State: ");
                          Serial.print(IMUerror1);
                          Serial.print(",");
                          Serial.print(IMUerror2);
                          Serial.print(",");
                          Serial.print(IMUerror3); 
                          Serial.print(",");
                          Serial.print(IMUerror4); 
                          Serial.println();
                      }
                   #endif
                }else{
                }
            }else{
              #ifdef DEBUG
                Serial.println("Frame end error -1");
              #endif
              IMUBufferPos++; 
              IMUerror1++;    
              UpdateIMUerrors = true; // Set update flag
            }
        }else{
            IMUBuffer.bytes[IMUBufferPos] = c;
            IMUBufferPos++;
            if (IMUBufferPos > IMUFrameEndPos){
              IMUBufferPos = IMUFrameStartPos; // Start from begin
              #ifdef DEBUG
                Serial.println("Frame end error -2");
              #endif
              IMUerror2++;    
              UpdateIMUerrors = true; // Set update flag
            }
        }
      }
  } // while end
} // IMUProcess() end
// -------------------------------------------- //
// EOF
