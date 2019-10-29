/*
   Cube11Paths Core Encoder Transmitter - AVR ATtiny1634
   (c)ElevenPaths 2017 jorge.rivera@11paths.com 

   FILE: "queue.hpp" -> Queue definitons and helper functions. Require standart library <RingBuf.h> // https://github.com/wizard97/ArduinoRingBuffer
   
*/

#if defined(DEBUG) 
  #define  TX_QUEUE_SIZE       32    // Transmitter queue size in packets (4 bytes => 32 bits)
#else
  #define  TX_QUEUE_SIZE       64    // Transmitter queue size in packets (4 bytes => 32 bits)
#endif

#include <RingBuf.h>       // https://github.com/wizard97/ArduinoRingBuffer

RingBuf *TxQueue = RingBuf_new(packetSize,TX_QUEUE_SIZE); //extern class RingBuf *TxQueue;  

// -------------------------------------------- //
// compose packet and add to TX queue
//
void sendPacket(uint16_t data,uint8_t code){   

    packet_t payload; 

    memset(&payload, 0, packetSize);

    payload.data  = data;
    payload.code  = code;

    TxQueue->add(TxQueue,&payload);
}

// -------------------------------------------- //
// compute turnCode and call sendPacket() 
//
void sendTurnCode(uint8_t side, int8_t turn){                  

  if ( (turn>=0) and (turn<TURN_SHIFTS) and (side < SIDES)  ) {
    
    turnIndex++;

    if (not (TxQueue->isFull(TxQueue))){
      
        sendPacket(turnIndex,turnCode[side][turn]);
        
    }else{}   // do nothing // only for debug

  }else{}     // do nothing // only for debug

}
// -------------------------------------------- //
// EOF

