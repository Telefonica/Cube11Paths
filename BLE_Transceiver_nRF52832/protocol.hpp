/*
   Cube11Paths BLE Transceiver - Nordic nRF52832
   (c)ElevenPaths 2018 jorge.rivera@11paths.com 

   File "protocol.hpp" --> Comunication Protocol definitons and helper functions.
   
*/

#define  SIDES                6    // CUBE SIDES
#define  TURN_SHIFTS          4    // CW,2CW,CCW and 2CCW
#define  FIRST_TURN_CODE   0xA0    // First turn code (rx use)
#define   LAST_TURN_CODE   0xF3    // First turn code (rx use)

#define BOOT_CODE 0x00
#define REDY_CODE 0x01 
#define FMEM_CODE 0x02 
#define BATT_CODE 0x03
#define QLEN_CODE 0x04  
#define SRST_CODE 0x05 // Software reset 
#define FUSE_CODE 0x06 // Fuse Byte High and Low
#define FUSL_CODE 0x07 // Fuse Byte Extend and Lock
#define STEP_CODE 0x08 // Steps per revolution
#define BEAM_CODE 0x09 // Beam code for debug

#define SLEP_CODE 0x11 // Sleep  code 
#define WAKE_CODE 0x12 // WakeUp code 

#define ERROR_QUEUE_CODE 0x90 // Overflow.
#define ERROR_BATT_CODE  0x91 // Batt critital, unable to boot.
#define ERROR_QINIT_CODE 0x99 // Critial! Not enough memory to queue init. Boot stop.

#define BATT_MIN             3.000   // (rx use) 
#define BATT_MAX             4.200   // (rx use) 
#define BATT_LEVEL_INTERVAL     20   // (tx use)

#define BATT_LOW             3.200   // 
#define BATT_FULL            4.100   // 

const uint8_t turnCode[SIDES][TURN_SHIFTS] = {   // Matrix index = (  0 ,                     1 ,             2 ,                     3 ) //
  {0xA0,0xA1,0xA2,0xA3} ,   /*  SIDE 0 (U) Upper / White  {TurnRight (CW), Double TurnRight (2CW), TurnLeft (CCW), Double TurnLeft (2CCW)} */
  {0xB0,0xB1,0xB2,0xB3} ,   /*  SIDE 1 (L) Left  / Orange {TurnRight (CW), Double TurnRight (2CW), TurnLeft (CCW), Double TurnLeft (2CCW)} */
  {0xC0,0xC1,0xC2,0xC3} ,   /*  SIDE 2 (F) Front / Green  {TurnRight (CW), Double TurnRight (2CW), TurnLeft (CCW), Double TurnLeft (2CCW)} */
  {0xD0,0xD1,0xD2,0xD3} ,   /*  SIDE 3 (R) Rigth / Red    {TurnRight (CW), Double TurnRight (2CW), TurnLeft (CCW), Double TurnLeft (2CCW)} */
  {0xE0,0xE1,0xE2,0xE3} ,   /*  SIDE 4 (B) Back  / Blue   {TurnRight (CW), Double TurnRight (2CW), TurnLeft (CCW), Double TurnLeft (2CCW)} */
  {0xF0,0xF1,0xF2,0xF3}     /*  SIDE 5 (D) Down  / Yellow {TurnRight (CW), Double TurnRight (2CW), TurnLeft (CCW), Double TurnLeft (2CCW)} */
};

const char* turnShift[] = { "CW","2CW","CCW","2CCW", "beam" }; // use as (char*)turnShift[(turn.code & 0x0F)] // beam for debug

/* CODE VALUE  CRC8      CODE       VALUE SAMPLE
--------------------------------------------------
TX 0x00 0x0027 0xAF  --> Boot ver    v0.39
TX 0x08 0x0018 0xBF  --> Steps rev   24
TX 0x04 0x0040 0x05  --> Queue len   64
TX 0x02 0x0149 0x38  --> Free mem    329
TX 0x06 0xDD62 0xB0  --> Fuse high   0xDD low 0x62
TX 0x07 0xFFFF 0xCE  --> Fule lock   0xFF ext 0xFF
TX 0x03 0x1026 0x12  --> Battery     4.134v
TX 0x01 0x0000 0xAB  --> Ready       0 (first boot)
TX 0x11 0x0002 0x70  --> Sleep       2 (counter)
TX 0x12 0x0000 0x05  --> Wake-Up     0 (turnIndex)
TX 0x91 0x0A9E 0xE2  --> Batt Error  2718 mV
TX 0x05 0x001C 0x94  --> SoftReset   28 (turnIndex)
 */
// -------------------------------------------- //
// EOF
