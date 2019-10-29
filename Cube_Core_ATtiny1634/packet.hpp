/*
   Cube11Paths Core Encoder Transmitter - AVR ATtiny1634
   (c)ElevenPaths 2017 jorge.rivera@11paths.com 

   FILE: packet.hpp -> Packet definitons and helper functions.
   
*/

typedef union {
  uint32_t full = 0;
  struct __attribute__((__packed__)){ // It needed by ARM arch like as Tennsy if packet size must be 4 bytes
    uint8_t  code;
    uint16_t data;
    uint8_t   crc; 
  };
}packet_t;

#define packetSize sizeof(packet_t) // Size of packet (4 bytes => 32 bits)

// Global vars
// -------------------------------------------- //
packet_t payload;
unsigned long wire_tx_time = 0;                 // Wait for wire tx
// -------------------------------------------- //

// Generic TX funcions
// -------------------------------------------- //
#define   tx_active()                       false       // old vw_tx_active()
#define     tx_packet              wire_tx_packet       // rf_tx_packet     
#define  tx_data_code                tx_data_code       // unchanged 
void  wait_tx(uint8_t i = 1) {delay(WIREUpdateTime*i);} // 

extern void wait_serial(void);

// -------------------------------------------- //
// CRC8 (polynomial X^8+X^5+X^4+X^0) - MAXIM/DALLAS  1-Wire/SMBus 
// https://www.maximintegrated.com/en/app-notes/index.mvp/id/27 // https://crccalc.com 
//
uint8_t CRC8(const uint8_t *data, uint8_t len) {
    uint8_t crc = 0x00;
    while (len--) {
        uint8_t extract = *data++;
        for (uint8_t tempI = 8; tempI; tempI--) {
            uint8_t sum = (crc ^ extract) & 0x01;
            crc >>= 1;
            if (sum) {
                crc ^= 0x8C;
            }
            extract >>= 1;
        }
    }
    return crc;
}

// -------------------------------------------- //
// Send packet to I2C slave device
void wire_tx_packet(packet_t *packet){ 

    int8_t state = -1; //return wire transmition result 

    packet->crc   = CRC8((const uint8_t*)packet,3);    // compute CRC
    
#ifdef DEBUG
    char b[20];
    char* pb = b;
    memset(pb,0,sizeof(b));
    sprintf(pb,(const char*)"TX 0x%02X 0x%04X 0x%02X ", packet->code, packet->data, packet->crc);
    Serial.print(pb);
#endif    

    // Wait for wire tx.
    while (millis()<wire_tx_time); // Wait for millis() > wire_tx_time
    wire_tx_time = millis()+WIREUpdateTime;

    if (I2Cready){                                   //Check for I2C SDA pull-up
      Wire.beginTransmission(I2C_SLAVE_ADDR);        // Transmit to device #8
      Wire.write((const uint8_t*)packet,packetSize);
      state = Wire.endTransmission();                // Stop transmitting
    }else{
      state = 5; 
    }
    
#ifdef DEBUG
    switch (state){
      case 0:
         Serial.println(F("ACK!"));
      break;
      case 1:
         Serial.println(F("Error! Data too long"));
      break;
      case 2:
         Serial.println(F("Error! Received NACK on transmit of address"));
      break;
      case 3:
         Serial.println(F("Error! Received NACK on transmit of data"));
      break;
      case 5:
         Serial.println(F("Error! no SCL pull-up"));
      break;
      default:
         Serial.print(F("Error "));
         Serial.println(state,DEC);
      break;
    }
#endif    
}

// -------------------------------------------- //
// Compose a tx packet from data and code provided and call rf_tx_packet() replace to serial_tx_packet()
void tx_data_code(uint16_t data, uint8_t code){  
  
    packet_t payload; 

    memset(&payload, 0, packetSize);

    payload.data  = data;
    payload.code  = code;

    tx_packet(&payload); //replace to generic tx_packet()
}

/* CODE VALUE  CRC8      CODE         VALUE SAMPLE
--------------------------------------------------
TX 0x00 0x0027 0xAF  --> Boot ver     v0.39
TX 0x08 0x0018 0xBF  --> Steps rev    24
TX 0x04 0x0040 0x05  --> Queue len    64
TX 0x02 0x0149 0x38  --> Free mem     329
TX 0x06 0xDD62 0xB0  --> Fuse high    0xDD low 0x62
TX 0x07 0xFFFF 0xCE  --> Fule lock    0xFF ext 0xFF
TX 0x03 0x1026 0x12  --> Battery      4.134v
TX 0x01 0x0000 0xAB  --> Ready        0 (first boot)
TX 0x11 0x0002 0x70  --> Sleep        2 (counter)
TX 0x12 0x0000 0x05  --> Wake-Up      0 (turnIndex)
TX 0x91 0x0A9E 0xE2  --> Batt Error   2718 mV
TX 0x05 0x001C 0x94  --> SoftReset    28 (turnIndex)
 */

// ----------------------------------------------- //
// EOF