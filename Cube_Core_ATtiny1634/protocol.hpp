/*
   Cube11Paths Core Encoder Transmitter - AVR ATtiny1634
   (c)ElevenPaths 2017 jorge.rivera@11paths.com 

   FILE: "protocol.hpp" -> Comunication Protocol definitons and helper functions.

*/

#define  SIDES                6       // CUBE SIDES
#define  TURN_SHIFTS          4       // CW,2CW,CCW and 2CCW
#define  FIRST_TURN_CODE   0xA0       // First turn code 

#define BOOT_CODE 0x00                // Boot start (version)
#define REDY_CODE 0x01                // Firmware ready (boot counts)
#define FMEM_CODE 0x02                // Free memory
#define BATT_CODE 0x03                // Battery charge mV
#define QLEN_CODE 0x04                // Queue lenght
#define SRST_CODE 0x05                // Software reset 
#define FUSE_CODE 0x06                // Fuse Byte High and Low
#define FUSL_CODE 0x07                // Fuse Byte Extend and Lock
#define STEP_CODE 0x08                // Steps per revolution
#define BEAM_CODE 0x09                // Beam code 

#define SLEP_CODE 0x11                // Sleep  code 
#define WAKE_CODE 0x12                // WakeUp code 

#define ERROR_QUEUE_CODE 0x90         // Overflow
#define ERROR_BATT_CODE  0x91         // Batt critital, unable to boot
#define ERROR_QINIT_CODE 0x99         // Critial! Not enough memory to queue init. Boot stop.

#define BATT_MIN             3.000    // 
#define BATT_OFFSET          0.200    // Battery offset over voltaje from minimal to enable boot.
#define BATT_MAX             4.200    // 
#define BATT_LEVEL_INTERVAL     20    // 

const uint8_t turnCode[SIDES][TURN_SHIFTS] = {   // Matrix index = (  0 ,                     1 ,             2 ,                     3 ) //
  {0xA0,0xA1,0xA2,0xA3} ,   /*  SIDE 0 (U) Upper / White  {TurnRight (CW), Double TurnRight (2CW), TurnLeft (CCW), Double TurnLeft (2CCW)} */
  {0xB0,0xB1,0xB2,0xB3} ,   /*  SIDE 1 (L) Left  / Orange {TurnRight (CW), Double TurnRight (2CW), TurnLeft (CCW), Double TurnLeft (2CCW)} */
  {0xC0,0xC1,0xC2,0xC3} ,   /*  SIDE 2 (F) Front / Green  {TurnRight (CW), Double TurnRight (2CW), TurnLeft (CCW), Double TurnLeft (2CCW)} */
  {0xD0,0xD1,0xD2,0xD3} ,   /*  SIDE 3 (R) Rigth / Red    {TurnRight (CW), Double TurnRight (2CW), TurnLeft (CCW), Double TurnLeft (2CCW)} */
  {0xE0,0xE1,0xE2,0xE3} ,   /*  SIDE 4 (B) Back  / Blue   {TurnRight (CW), Double TurnRight (2CW), TurnLeft (CCW), Double TurnLeft (2CCW)} */
  {0xF0,0xF1,0xF2,0xF3}     /*  SIDE 5 (D) Down  / Yellow {TurnRight (CW), Double TurnRight (2CW), TurnLeft (CCW), Double TurnLeft (2CCW)} */
};

/* TX unused
const char* turnShift[] = { "CW","2CW","CCW","2CCW" }; // use as (char*)turnShift[(turn.code & 0x0F)]
uint8_t side = (turn.code >> 4) - 0x0A;
uint8_t turn = (turn.code & 0x0F);
#define TurnCW    0 // TurnRigth: clockwise (typically abbreviated as CW)
#define Turn2CW   1 // Double TurnRigth: two clockwise (typically abbreviated as 2CW)
#define TurnCCW   2 // TurnLeft : (in North American English) counterclockwise (CCW) or (in Commonwealth English) anticlockwise (ACW)
#define Turn2CCW  3 // Double TurnLeft : (in North American English) two counterclockwise (2CCW) or (in Commonwealth English) two anticlockwise (2ACW)
*/

// Global vars
// -------------------------------------------- //
uint16_t turnIndex = 0; 

// SOFTWARE RESET SEQUENCE DEFINITION AND INDEX //
const uint8_t  RST_SEQ[] = { 0xA0,0xA0,0xA0,0xA0,0xA0,0xA0,0xA0,0xA0,0xA0,0xA0,0xA0,0xA0,0xA2,0xA2,0xA2,0xA2 };
uint8_t       resetIndex = 0;

// -------------------------------------------- //
// EOF

