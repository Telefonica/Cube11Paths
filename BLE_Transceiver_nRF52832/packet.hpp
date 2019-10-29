 /*
   Cube11Paths BLE Transceiver - Nordic nRF52832
   (c)ElevenPaths 2018 jorge.rivera@11paths.com 

   File "packet.hpp" -> Packet definitons and helper functions.
   
*/

// --------------------------------------------------------------- //

typedef union {
  uint32_t full = 0;
  uint8_t  bytes[];  
  struct __attribute__((__packed__)){ // It needed by ARM arch like as Tennsy if packet size must be 4 bytes
    uint8_t  code;
    uint16_t data;
    uint8_t   crc; 
  };
}packet_t;

#define packetSize (int8_t)sizeof(packet_t) // Size of packet (4 bytes => 32 bits)
#define COREPacketSize packetSize

packet_t payload;

// Functions
// -------------------------------------------- //
// CRC8 (polynomial X^8+X^5+X^4+X^0) - MAXIM/DALLAS  1-Wire/SMBus 
// https://www.maximintegrated.com/en/app-notes/index.mvp/id/27 https://crccalc.com 
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
// EOF