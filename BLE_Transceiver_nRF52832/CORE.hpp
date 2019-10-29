/*
   Cube11Paths BLE Transceiver - Nordic nRF52832
   (c)ElevenPaths 2018 jorge.rivera@11paths.com 

   File "CORE.hpp" --> Core configurations and helper functions.
   
   Giiker Codes
   ----------------------------------------------------------------
   Azul     =  B --> Back  (atras)       -Z ===> CW=0x11   CCW=0x13
   Amarillo =  D --> Down  (abajo)       +Y ===> CW=0x21   CCW=0x23
   Naranja  =  L --> Left  (izquierda)   -X ===> CW=0x31   CCW=0x33
   Blanco   =  U --> Upper (arriba)      -Y ===> CW=0x41   CCW=0x43
   Rojo     =  R --> Rigth (derecha)     +X ===> CW=0x51   CCW=0x53
   Verde    =  F --> Front (frente)      +Z ===> CW=0x61   CCW=0x63 
                                                                                                           |------ LAST MOVES -----| 
                                                                                                           |  -3    -2    -1  LAST |
   DONE = { 0x12, 0x34, 0x56, 0x78, 0x33, 0x33, 0x33, 0x33, 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0x00, 0x00, 0x11, 0x13, 0x11, 0x13 }
                                                                                                  move.turn[   0,    1,    2,    3 ]
   CORE CODES
   --------------------------------------------------------------------------------------------------------------------------------------------
   const uint8_t turnCode[SIDES][TURN_SHIFTS] = {  // Matrix index = (  0 ,                     1 ,             2 ,                     3 )  //
   {0xA0,0xA1,0xA2,0xA3} ,   //  SIDE 0 (U) Upper / White  {TurnRight (CW), Double TurnRight (2CW), TurnLeft (CCW), Double TurnLeft (2CCW)}  //
   {0xB0,0xB1,0xB2,0xB3} ,   //  SIDE 1 (L) Left  / Orange {TurnRight (CW), Double TurnRight (2CW), TurnLeft (CCW), Double TurnLeft (2CCW)}  //
   {0xC0,0xC1,0xC2,0xC3} ,   //  SIDE 2 (F) Front / Green  {TurnRight (CW), Double TurnRight (2CW), TurnLeft (CCW), Double TurnLeft (2CCW)}  //
   {0xD0,0xD1,0xD2,0xD3} ,   //  SIDE 3 (R) Rigth / Red    {TurnRight (CW), Double TurnRight (2CW), TurnLeft (CCW), Double TurnLeft (2CCW)}  //
   {0xE0,0xE1,0xE2,0xE3} ,   //  SIDE 4 (B) Back  / Blue   {TurnRight (CW), Double TurnRight (2CW), TurnLeft (CCW), Double TurnLeft (2CCW)}  //
   {0xF0,0xF1,0xF2,0xF3}     //  SIDE 5 (D) Down  / Yellow {TurnRight (CW), Double TurnRight (2CW), TurnLeft (CCW), Double TurnLeft (2CCW)}};//

   Translate Core Codes to Giiker Codes
   --------------------------------------------
   0xA0 --> 0x41    ( 1010 0000 --> 0100 0001 )
   0xA2 --> 0x43
   0xB0 --> 0x31    ( 1011 0000 --> 0011 0001 )
   0xB2 --> 0x33
   0xC0 --> 0x61    ( 1100 0000 --> 0110 0001 )
   0xC2 --> 0x63
   0xD0 --> 0x51    ( 1101 0000 --> 0101 0001 )
   0xD2 --> 0x53
   0xE0 --> 0x11    ( 1110 0000 --> 0001 0001 )
   0xE2 --> 0x13
   0xF0 --> 0x21    ( 1111 0000 --> 0010 0001 )
   0xF2 --> 0x23
*/

typedef  union  {
   unsigned char bytes[20] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0} ;
   struct __attribute__((__packed__)){ // It needed by ARM arch like as Tennsy if packet size must be 4 bytes
        uint8_t  ToDo[16];
        uint8_t   turn[4];
   };
   
}gmove_t;

// Globals
// -------------------------------------------- //
gmove_t move;

#define moveSize (int8_t)sizeof(gmove_t) 

const uint8_t transCode[SIDES][TURN_SHIFTS]= {
  {0xA0,0x41,0xA2,0x43} ,
  {0xB0,0x31,0xB2,0x33} ,
  {0xC0,0x61,0xC2,0x63} , 
  {0xD0,0x51,0xD2,0x53} , 
  {0xE0,0x11,0xE2,0x13} ,
  {0xF0,0x21,0xF2,0x23}
};

// -------------------------------------------- //
uint16_t turnIndex  = 0; 
uint16_t WIREErrors = 0;      
 int16_t turnLosts  = 0;      
 uint8_t  lastCode  = 0xFF;   
uint16_t  lastData  = 0xFFFF; 

// Functions
// -------------------------------------------- //
void CORERead(packet_t *packet = &payload);
void COREProcess(packet_t *packet = &payload);

#ifdef DEBUG_VERBOSE
  void packetLogger(packet_t *packet = &payload);
#endif 

uint8_t gmoveCode(uint8_t code){
  return transCode[ (code >> 4 ) - 0x0A ][(code & 0x0F) +1];
}

// -------------------------------------------- //
//
void COREProcess(packet_t *packet){

  lastCode =  (uint8_t)(packet->code); 
  lastData = (uint16_t)(packet->data); 
  
  lastCodeChar.setValue(lastCode);      
  lastDataChar.setValue(lastData);     
        
  if (packet->code>=FIRST_TURN_CODE and packet->code<=LAST_TURN_CODE){ // code is a turn code
    turnIndex++;
    turnIndexChar.setValue(turnIndex); 
   
    if (packet->data!=turnIndex){ 
      turnLosts = (int16_t)(packet->data-turnIndex);
      turnLostsChar.setValue(turnLosts);
    }

    move.turn[3]=move.turn[2];
    move.turn[2]=move.turn[1];
    move.turn[1]=move.turn[0];
    move.turn[0]= gmoveCode(packet->code);

    moveCharacteristic.setValue( move.bytes, moveSize);
   
  }else{
    switch (packet->code){
      
      case REDY_CODE:     
         turnIndex=0;
         turnIndexChar.setValue(turnIndex);
      break;

      case BATT_CODE:
      
          // Fix battery percent, setting batt full if > 4.05v and batt emply if < 3.55v 

          int16_t batt_percent;
     
          //batt_percent = (int16_t)round( ((  (float)(packet->data/1000.0) - 3.55 ) * 100.0 / ( 4.05 - 3.55 )) );

          batt_percent = (int16_t)round( ((  (float)(packet->data/1000.0) - BATT_LOW ) * 100.0 / ( BATT_FULL - BATT_LOW )) ); 

          if (batt_percent>100){ 
            batt_percent = 100;
          }else{
            if (batt_percent<1) batt_percent = 1;
          }
          
          batteryCharacteristic.setValue((uint8_t)batt_percent);
          
          batteryStateChar.setValue(packet->data);

          #ifdef DEBUG
            Serial.print("DEBUG: Setting battery status: ");
            Serial.print( (uint8_t)batt_percent, DEC);
            Serial.print("% (");
            Serial.print( packet->data/1000.0,3);
            Serial.println("v)");
          #endif 
      break;

      case WAKE_CODE: 
          turnIndex=packet->data;
          turnIndexChar.setValue(turnIndex); 
      break;

    }// swich case end
  }
}

// -------------------------------------------- //
//
void CORERead(packet_t *packet){

   static int8_t read_index = 0;
   char inByte;
   
   while (Wire.available() > 0){

     inByte = Wire.read();
     packet->bytes[read_index] = inByte;

     if (read_index==3){ // 4 bytes readed

        read_index=0;
     
        if (inByte == CRC8((const uint8_t*) packet,3)){

          #ifdef DEBUG
            #ifdef DEBUG_VERBOSE
              packetLogger(packet);
            #else
            static char b[20];   
            static char* pb = b; 
            memset(pb,0,sizeof(b));
            sprintf(pb,(const char*)"RX 0x%02X 0x%04X 0x%02X\n", packet->code, packet->data, packet->crc);
            Serial.print("DEBUG: ");
            Serial.print(pb);
            #endif
          #endif 

          COREProcess(packet);
           
        }else{
          WIREErrors++;
          #ifdef DEBUG
            Serial.println("DEBUG: WIRE CRC error");
          #endif 
        }
      }else{ 
        read_index++;
      } 
  }//while end
} // CORERead() end
// -------------------------------------------- //
// EOF