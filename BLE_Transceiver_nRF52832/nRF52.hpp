/*
   Cube11Paths BLE Transceiver - Nordic nRF52832
   (c)ElevenPaths 2018 jorge.rivera@11paths.com 

   File "nRF52.hpp" --> Nordic nRF52832 helper functions.
   
*/

typedef volatile uint32_t REG32;
#define pREG32 (REG32 *)

#define DEVICE_ID_HIGH    (*(pREG32 (0x10000060)))
#define DEVICE_ID_LOW     (*(pREG32 (0x10000064)))
#define MAC_ADDRESS_HIGH  (*(pREG32 (0x100000a8)))
#define MAC_ADDRESS_LOW   (*(pREG32 (0x100000a4)))

// MAC Address
uint32_t addr_high = ((MAC_ADDRESS_HIGH) & 0x0000ffff) | 0x0000c000;
uint32_t addr_low  = MAC_ADDRESS_LOW;

#ifdef __arm__
  // should use uinstd.h to define sbrk but Due causes a conflict https://github.com/eliotstock/memory
  extern "C" char* sbrk(int incr);
#else  // __ARM__
  extern char *__brkval;
#endif  // __arm__

// ----------------------------------------------- //
//
int freeMemory() {
  char top;
#ifdef __arm__
  return &top - reinterpret_cast<char*>(sbrk(0));
#elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
  return &top - __brkval;
#else  // __arm__
  return __brkval ? &top - __brkval : &top - __malloc_heap_start;
#endif  // __arm__
}

// ----------------------------------------------- //
//
void nRF52info(){
    
    Serial.println();
    Serial.print(F("Cube11Paths BLE v0.")); Serial.print(MINOR_VERSION,DEC); Serial.print(" (c) ElevenPaths 2017-2019 ("); 
    Serial.print(F( __DATE__ " " __TIME__ ));    Serial.println(F(")"));            
    Serial.print(F("FirmwareSRC:  ")); Serial.println(firmFile);
    Serial.print(F("GCC version:  ")); Serial.print(F(__VERSION__));Serial.print(F(" IDE (")); Serial.print(ARDUINO); Serial.println(F(")"));            

    // Requiere SoftDevice compilation //
    Serial.print(F("CPU variant:  nRF")); Serial.print(NRF_FICR->INFO.PART,HEX);Serial.print(" 0x");Serial.print(NRF_FICR->INFO.VARIANT,HEX); Serial.print(" Package: 0x"); Serial.print(NRF_FICR->INFO.PACKAGE,HEX);Serial.println();   
    Serial.print(F("Memory:       Flash = ")); Serial.print(NRF_FICR->INFO.FLASH,DEC); Serial.print(" Kbytes, RAM = "); Serial.print(NRF_FICR->INFO.RAM,DEC);Serial.print(" Kbytes, ");
    Serial.print("Free = "); Serial.print(freeMemory()); Serial.print(" bytes \n");  
    
    Serial.print(F("MAC Address:  "));
    Serial.print((addr_high >> 8) & 0xFF, HEX); Serial.print(":");
    Serial.print((addr_high) & 0xFF, HEX); Serial.print(":");
    Serial.print((addr_low >> 24) & 0xFF, HEX); Serial.print(":");
    Serial.print((addr_low >> 16) & 0xFF, HEX); Serial.print(":");
    Serial.print((addr_low >> 8) & 0xFF, HEX); Serial.print(":");
    Serial.print((addr_low) & 0xFF, HEX); Serial.print(" ");

    // Unique Device ID
    Serial.print(F("Device ID: ")); Serial.print(DEVICE_ID_HIGH, HEX); Serial.println(DEVICE_ID_LOW, HEX);
    
    Serial.print("PIN_SERIAL_RX: "); Serial.println(PIN_SERIAL_RX,DEC);
    Serial.print("PIN_SERIAL_TX: "); Serial.println(PIN_SERIAL_TX,DEC);
    Serial.print("PIN_WIRE_SDA:  "); Serial.println(PIN_WIRE_SDA,DEC);
    Serial.print("PIN_WIRE_SCL:  "); Serial.println(PIN_WIRE_SCL,DEC);
    Serial.print("NRF_TWI0->ADDRESS: 0x"); Serial.println(uint16_t(NRF_TWI1->ADDRESS),HEX); // (I2C_SLAVE_ADDR)
    Serial.print("SERIAL_BUFFER_SIZE: "); Serial.println(uint16_t(SERIAL_BUFFER_SIZE),DEC); 
    Serial.print("NRF_GPIO->PIN_CNF[PIN_WIRE_SDA]: 0b"); Serial.println(uint32_t(NRF_GPIO->PIN_CNF[PIN_WIRE_SDA]),BIN);
    Serial.print("NRF_GPIO->PIN_CNF[PIN_WIRE_SCL]: 0b"); Serial.println(uint32_t(NRF_GPIO->PIN_CNF[PIN_WIRE_SCL]),BIN);

    //Serial.print("UICR->APPROTECT: 0x"); Serial.println(NRF_UICR->APPROTECT,HEX);
    //Serial.print("NRF_NVMC->READY: 0x"); Serial.println(NRF_NVMC->READY,HEX);
    //Serial.print("NRF_NVMC->CONFIG: 0x"); Serial.println(NRF_NVMC->CONFIG,HEX);
        
    Serial.print("IMU update every: ");
    Serial.print(IMUupdate,DEC); 
    Serial.println(" ms");   

    Serial.print("UART re-start at: ");
    Serial.print(UARTrestart,DEC); 
    Serial.println(" ms");   
        
    Serial.println();
}

// -------------------------------------------- //
// EOF

