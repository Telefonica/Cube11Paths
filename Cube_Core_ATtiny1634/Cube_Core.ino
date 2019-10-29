/*
   Cube11Paths Core Encoder Transmitter - AVR ATtiny1634
   (c)ElevenPaths 2017 jorge.rivera@11paths.com 

   See https://github.com/ElevenPaths/Cube11Paths for details.

*/

#include "Cube_Core.h"

// --------------------------------------------------------------------------------------------------------------------------------------------------- //
void setup() {
  
  // Set Clock PreScaler to 1 divider for 8 Mhz CPU Clock. Internal RC Clock = 8 Mhz but default PreScaler = 8 divider for 1 Mhz CPU Clock.
                                       // https://github.com/fschaefer/Prescaler  MODIFY TO SUPPORT ATtiny1634 
  setClockPrescaler(CLOCK_PRESCALER_1);  

  // Save MCUSR – MCU Status Register
  cli(); 
    uint8_t resetCause = MCUSR;
    bitClear(MCUSR,WDRF); // Clear WDT reset flag
    bitClear(WDTCSR,WDE); // WDE: Watchdog disable
  sei();
   
  // Prevent enable PTT
       pinMode(RFPTT_PIN,OUTPUT);                             
  digitalWrite(RFPTT_PIN,   LOW);

  // Get FUSES configuration
  cli(); 
    uint8_t lowBits      = boot_lock_fuse_bits_get(GET_LOW_FUSE_BITS);
    uint8_t highBits     = boot_lock_fuse_bits_get(GET_HIGH_FUSE_BITS);
    uint8_t extendedBits = boot_lock_fuse_bits_get(GET_EXTENDED_FUSE_BITS);
    uint8_t lockBits     = boot_lock_fuse_bits_get(GET_LOCK_BITS);
  sei();

  // Load encoder instances on side array // See file "encoders.hpp"
  side[0].encoder = &encoder0;
  side[1].encoder = &encoder1;
  side[2].encoder = &encoder2;
  side[3].encoder = &encoder3;
  side[4].encoder = &encoder4;
  side[5].encoder = &encoder5;

// Set Power Save Mode Configurations
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------//
                            // ADCSRA – ADC Control and Status Register A // ADCSRA_saved = ADCSRA; -> 0b10000110 = 0x86 = 134
  bitSet(ADCSRA,4);         // Bit 4 – ADIF ADC Interrupt Flag cleared by writing a logical one to the flag
  ADCSRA = 0;               // power_adc_disable();  Important note! You must use the PRR after setting ADCSRA to zero, otherwise the ADC is "frozen" in an active state.
                            // Thank you!!! http://www.gammon.com.au/power
  
  power_usart1_disable();   // Consumition 4.20mA
  power_timer1_disable();   // Disable, former timer1 needed by VirtaulWire
  wdt_disable();            // Disable via #include <avr/wdt.h> and FUSES
 
  //Setting UART0, if DEBUG disable RX and RX/TX interrupts to prevent wakeups
  #ifdef DEBUG 
                                // UCSRnB – USART Control and Status Register B
      bitClear(UCSR0B,4);       // Bit 4 – RXENn: Receiver Enable. Writing this bit to zero disables the receiver. Disabling the receiver will flush the receive buffer.
                                // Maybe UCSR0B &=~(1<<RXEN0); // UART0 disable RX

      bitClear(UCSR0B,UDRIE0);  // Bit 5 – UDRIE: USART Data Register Empty Interrupt Enable. Writing this bit to one enables interrupt on the UDREn flag.
                                // A Data Register Empty interrupt will be generated only if the UDRIEn bit is written to one, 
                                // the Global Interrupt Flag in SREG is written to one and the UDREn bit is set.

      bitClear(UCSR0B,TXCIE0);  // Bit 6 - TXCIEn: TX Complete Interrupt Enable. Writing this bit to one enables interrupt on the TXCn flag. 
                                // A USART Transmit Complete interrupt will be generated only if the TXCIEn bit is written to one, 
                                // the Global Interrupt Flag in SREG is written to one and the TXCn bit is set.
  #else 
      power_usart0_disable();   // UART disable if no debug
  #endif

  set_sleep_mode (SLEEP_MODE_PWR_DOWN);   // Other modes SLEEP_MODE_STANDBY, SLEEP_MODE_ADC and SLEEP_MODE_IDLE

  //-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------//
  // Configure and Enable Pin Change Interrupts vectors (0,1,2) for wakeup and process side turn. See file "encoders.hpp"

  // Registers PCMSK0, PCMSK1, and PCMSK2 control which pins contribute to the pin change interrupts.
  // GIMSK – General Interrupt Mask Registers
  
  PCMSK0 = PCMSK0_MASK; 
  PCMSK1 = PCMSK1_MASK;
  PCMSK2 = PCMSK2_MASK; 

  bitSet(GIMSK,PCIE2); // bit 5  // GIMSK int mast (INT0*, PCINT2, PCINT1, PCINT0) bit x,6,5,4,3,x,x,x  
  bitSet(GIMSK,PCIE1); // bit 4 
  bitSet(GIMSK,PCIE0); // bit 3 

  //-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------//
  //If debug enable show boot params info and check for TxQueue ready, if no debug enable, disable UART0
#ifdef DEBUG

  Serial.begin(UART_SPEED); 

  Serial.println(); 
  Serial.print(F(CODE_NAME " v0.")); Serial.print(MINOR_VERSION);                      
  Serial.println(F(" (c) ElevenPaths 2017-2019"));                                     
  Serial.println(F("Firmware " __FILE__)); 
  Serial.print(F("Compiled " __DATE__ " " __TIME__ " IDE ")); Serial.print(ARDUINO);  
  Serial.print(F(" AVR GCC v")); Serial.print(F(__VERSION__));
  Serial.print(F(" AVR libc v")); Serial.println(F(__AVR_LIBC_VERSION_STRING__));

  /*
  Serial.print(F("ATtiny1634 Signature: 0x")); // Signature = 0x1E, 0x94, 0x12
  Serial.print(SIGNATURE_0,HEX);   Serial.print(F(" 0x"));
  Serial.print(SIGNATURE_1,HEX);   Serial.print(F(" 0x"));
  Serial.println(SIGNATURE_2,HEX);
  */

  Serial.print(F("OCS Clock: "));
  Serial.print(F_CPU/1000000,DEC);
  Serial.print(F(" MHz with perscaler divisor "));
  Serial.print(getClockDivisionFactor(), DEC);
  Serial.print(F(" => "));
  Serial.print((F_CPU/getClockDivisionFactor())/1000000,DEC);
  Serial.println(F(" MHz"));
  
  // FUSES
  Serial.print(F("Fuse High: 0x"));  Serial.print(highBits, HEX);
  Serial.print(F(" Fuse  Low: 0x")); Serial.println(lowBits, HEX); 
  Serial.print(F("Fuse  Ext: 0x"));  Serial.print(extendedBits, HEX);
  Serial.print(F(" Fuse Lock: 0x")); Serial.println(lockBits, HEX);

  // Show RESET CAUSE from MCUSR – MCU Status Register
  Serial.print(F("Reset MCU: 0b")); Serial.print(resetCause, BIN); 
  Serial.println( bitRead(resetCause, WDRF ) ? " (WDT Reset)" : "" ); // Check for Bit 3 – WDRF: Watchdog Reset Flag. This bit is set if a Watchdog Reset occurs

  // MEMORY
  Serial.print(F("Free  Mem: ")); Serial.print(freeRam(),DEC); Serial.println(F(" bytes"));

  // TX QUEUE
  Serial.print(F("QueueSize: "));
  Serial.print(TX_QUEUE_SIZE * packetSize,DEC);
  Serial.print(F(" bytes ("));
  Serial.print(TX_QUEUE_SIZE,DEC);
  Serial.println(F(" packets)"));
  
  if (!TxQueue){
    Serial.println(F("ERROR! Not enough memory"));
    while (1); // STOP FOREVER // 
  }

  // Show time, wait and delays values
  Serial.print(F("Time to go sleep: ")); Serial.print(STANDBY_TIME/1000,DEC);Serial.println(F(" seconds"));
  Serial.print(F("Time to wait BLE: ")); Serial.print(WAIT_NRF52,DEC);Serial.println(F(" ms"));
  Serial.print(F("Time inter I2Ctx: ")); Serial.print(WIREUpdateTime,DEC);Serial.println(F(" ms"));
  Serial.print(F("WatchDogOverflow: ")); Serial.print(250,DEC);Serial.println(F(" ms (manual)"));
  
#endif

  //--------------------------------------------------------------------------------//
  // Check for battery status
  
  uint16_t min_vccRead = vccReadMin(); 

  #ifdef DEBUG
  Serial.print(F("Batt Test: "));
  Serial.print(min_vccRead/1000.0,3);
  Serial.print(F("v "));
  #endif

  while ((uint16_t)min_vccRead < (uint16_t)(((BATT_MIN+BATT_OFFSET)*1000)) ){  

     #ifdef DEBUG
      Serial.print(F("Error! (minimal for boot: "));
      Serial.print(BATT_MIN+BATT_OFFSET,3);
      Serial.print(F("v) Going to sleep... "));
      wait_serial();
     #endif

     sleep_enable();
     sleep_cpu();        // Sleep Mode: Power Down 

     //----  SLEEP  ----// 
     // zzZZ...Zzz.ZzZz //
     //---- WAKE UP ----// 
        
     sleep_disable();   

     min_vccRead = vccReadMin();
     
     #ifdef DEBUG
      Serial.println(F("Wake-Up!!!"));
      Serial.print(F("Batt Init: "));
      Serial.print(min_vccRead/1000.0,3);
      Serial.print(F("v "));
     #endif
     
  }

  #ifdef DEBUG
    Serial.print(F("OK! (minimal for boot: "));  
    Serial.print(BATT_MIN+BATT_OFFSET,3);       
    Serial.println(F("v)"));
  #endif

  //--------------------------------------------------------------------------------//
  // PPT going up to start IMU&nRF52
  
  #ifdef DEBUG
    Serial.print(F("BLE Start: "));
    wait_serial();
  #endif
  
  digitalWrite(RFPTT_PIN, HIGH);  // Set high ppt pin
  delay(WAIT_NRF52);              // Wait for nRF52 boot

  #ifdef DEBUG
    Serial.println(F("OK!"));
  #endif

  //--------------------------------------------------------------------------------// 
  // Check for I2C SCL

  #ifdef DEBUG
  Serial.print(F("I2C Check: "));
  wait_serial();
  #endif
  
  pinMode(PIN_I2C_SCL, OUTPUT);
  digitalWrite(PIN_I2C_SCL,LOW);
  pinMode(PIN_I2C_SCL, INPUT);

  I2Cready = digitalRead(PIN_I2C_SCL);

  if (I2Cready){
    
    Wire.begin();

    #ifdef DEBUG
      Serial.println(F("OK! SCL pull-up detected"));
      Serial.print(F("IC2 Slave: 0x"));
      Serial.println(I2C_SLAVE_ADDR,HEX);
      Serial.print(F("I2CBuffer: "));
      Serial.println(WIRE_BUFFER_LENGTH,DEC); 
    #endif 
  }else{
    #ifdef DEBUG
      Serial.print(F("Error! no SCL pull-up. Disable PTT and going to sleep... "));
      wait_serial();
    #endif 

    digitalWrite(RFPTT_PIN, LOW);   // Set lowh ppt pin
    delay(WAIT_NRF52);              // wait for nRF52 stop
    
    sleep_enable();
    sleep_cpu();                    // Sleep Mode: Power Down 

    //----  SLEEP  ----// 
    // zzZZ...Zzz.ZzZz //
    //---- WAKE UP ----// 
        
    sleep_disable();   
     
    #ifdef DEBUG
      Serial.println(F("Wake-Up!!! and soft reboot!!!"));
      wait_serial();
    #endif   
    
    asm volatile ("  jmp 0"); //  Restarts program from beginning but does not reset the peripherals and registers
    
  }

//----------------------------------------------------------------------------------------------------------------//

   min_vccRead = vccReadAvg();           // Read battery average again after IMU&BLE started
   
   last_vccRead = min_vccRead;          
     
   #ifdef DEBUG
    Serial.print(F("Batt Last: "));
    Serial.print(min_vccRead/1000.0,3);
    Serial.println(F("v (average)"));
    Serial.print(F("Boot Time: "));
    Serial.print(millis());
    Serial.println(F("ms"));
    Serial.println(F("WDT Reset: Enable!"));
    Serial.println();
    wait_serial();
  #endif

  // --------------------------------------------------------------------------//
  // WDT as WDTCSR – Watchdog Timer Control and Status Register
  //                   Set WDT interval WDP 3 2 1 0
  //                                        1 0 0 1 = 256 Kcycles = 8.000 seconds
  //                                        1 0 0 0 = 128 Kcycles = 4.000 seconds
  //                                        0 1 0 0 =   8 Kcycles = 0.250 seconds
  bitSet(WDTCSR,WDP2);

  // WDIE: Watchdog Timeout Interrupt Enable. In this mode the corresponding interrupt 
  //       is executed instead of a reset if a timeout in the Watchdog Timer occurs
  bitSet(WDTCSR,WDIE); 
  
  
  // BOOT SEQUENCE FINISH, INIT BLE TRANSMISSIONS //
  // ---------------------------------------------------------------------------//
  // tx_data_code() send code to nRF52 by I2C, max at WIREUpdateTime 
  #ifdef DEBUG
    Serial.print(F("DEBUG: Sending battery status ("));
    Serial.print(min_vccRead/1000.0,3);
    Serial.print(F("v) --> "));
  #endif

  tx_data_code((uint16_t)min_vccRead,(uint8_t)BATT_CODE);                       // Send battery min
  
  if (!TxQueue){        
    tx_data_code((uint16_t)TX_QUEUE_SIZE,(uint8_t)ERROR_QINIT_CODE);            // If not queue, send error code only
    bitClear(WDTCSR,WDIE);    // Watchdog Disable
    while(1); // STOP FOREVER //
  }

  #ifdef DEBUG         
    Serial.print(F("DEBUG: Sending ready code by queue tx  --> "));             // Send battery status 
  #endif

  // sendPacket() add packet to Tx Queue and it will be sent at loop() 
  sendPacket((uint16_t)(freeRam()),(uint8_t)REDY_CODE);                         // Send ready packet to queue
  
  wake_time = millis()+STANDBY_TIME; 
 
} //end of setup()

// --------------------------------------------------------------------------------------------------------------------------------------------------- //
void loop() {

  uint16_t tmp_vccRead; 

  wdt_reset(); 

  // If TxQueue not emply
  // -------------------------------------------------------------------------------------------------------------------//
  if (not TxQueue->isEmpty(TxQueue)){           // if TxQueue not emply

    while(TxQueue->numElements(TxQueue)>0){     // while TxQueue not emply 
       
        if (TxQueue->isFull(TxQueue)){          // if TxQueue is full then send queue error code.
            #ifdef DEBUG
              Serial.println(F("DEBUG: Sending ERROR_QUEUE_CODE --> "));
            #endif
            tx_data_code((uint16_t)TxQueue->numElements(TxQueue),ERROR_QUEUE_CODE);   
            wdt_reset();                        // WDT reset to prevent overflow        
        }


        TxQueue->pull(TxQueue, &payload);       // Get next packet from TxQueue
        tx_packet(&payload);                    // Send packet
        wdt_reset();                            // WDT reset to prevent overflow 

        // Check for reset sequence 
        // ------------------------------------------------------------------------//
        if (payload.code == RST_SEQ[resetIndex]){ 
          if (resetIndex==sizeof(RST_SEQ)-1){
             payload.code = SRST_CODE;
             payload.data++; 

             #ifdef DEBUG        //  Sending battery status (3.289v) --> 
              Serial.print(F("DEBUG: Sending reset sequence detected --> "));
             #endif
         
             tx_packet(&payload); 
             wdt_reset(); // WDT reset to prevent overflow
             wait_tx(6);  // Increase to ensure RX and BLE update reception. Wait time must be < than wdt overflow time (6 x 25 < 250)
             
             #ifdef DEBUG
              Serial.println(F("DEBUG: BLE Stop"));
             #endif

             wdt_reset();       // WDT reset to prevent overflow
             delay(WAIT_NRF52); // Wait for nRF52 TXCn
             wdt_reset();       // WDT reset to prevent overflow
             
             digitalWrite(RFPTT_PIN,  LOW);         
             
             delay(WAIT_NRF52); // Wait for nRF52 TX 
             wdt_reset();       // WDT reset to prevent overflow

             #ifdef DEBUG
              Serial.print(F("DEBUG: Force hard reset... "));
             #endif
      
             while(1);          // Force hardware reset by WDT
             
          }else{
            resetIndex++;
            if (resetIndex==sizeof(RST_SEQ)) resetIndex=0;  
          }
        }else{
            if (payload.code != BATT_CODE) resetIndex=0;   
        }
        // ------------------------------------------------------------------------//

        // Check for battery interval sents
        // ------------------------------------------------------------------------//
        if ((payload.data>0) and (payload.data % BATT_LEVEL_INTERVAL == 0)){                

            tmp_vccRead = vccReadAvg(); 
            last_vccRead = tmp_vccRead; 
            
            #ifdef DEBUG
              Serial.print(F("DEBUG: Sending battery status ("));
              Serial.print(tmp_vccRead/1000.0,3);
              Serial.print(F("v) --> "));
            #endif

            tx_data_code((uint16_t)tmp_vccRead,(uint8_t)BATT_CODE); 
            wdt_reset(); // WDT reset to prevent overflow
        }
       // ------------------------------------------------------------------------//
     }// end while
   } // end if not empty
   // -------------------------------------------------------------------------------------------------------------------// 
  
  if ((not tx_active()) and (GIFR==0)) { // GIFR check GIFR – General Interrupt Flag Register ( INTF0, PCIF2, PCIF1, PCIF0 ) 
  

    if ( millis() > wake_time ) {        // Going to Sleep

        #ifdef DEBUG           // Sending battery status (3.289v) --> 
          Serial.print(F("DEBUG: Sending prepare for go to sleep --> "));
        #endif 

        // Send sleep code
        tx_data_code(uint16_t(++sleep_count),(uint8_t)SLEP_CODE); 
        wdt_reset(); // WDT reset to prevent overflow
        wait_tx(6);  // Increase to ensure RX and BLE update reception. Wait time must be < than wdt overflow time  (6 x 25 < 250)

        #ifdef DEBUG
          Serial.println(F("DEBUG: BLE Stop"));
        #endif
             
        // TURN OFF nRF52
        digitalWrite(RFPTT_PIN,  LOW);    
        wdt_reset();          // WDT reset to prevent overflow
        delay(WAIT_NRF52);    // Wait for nRF52
        wdt_reset();          // WDT reset to prevent overflow

        #ifdef DEBUG
          Serial.println(F("DEBUG: I2C SLC pull-down"));
        #endif
                
        Wire.end();                       
        pinMode(PIN_I2C_SCL, OUTPUT);     // Force SCL LOW
        digitalWrite(PIN_I2C_SCL,LOW);    // Force SCL LOW

        #ifdef DEBUG
          Serial.println(F("DEBUG: Sleep..."));
          wait_serial();
        #endif

        bitClear(WDTCSR,WDIE);  // Watchdog Disable
         
        // Go to sleep
        sleep_enable();
        sleep_cpu();            // Sleep Mode: Power Down 

        //----  SLEEP  ----// 
        // zzZZ...Zzz.ZzZz //
        //---- WAKE UP ----// 
        
        sleep_disable();
        bitSet(WDTCSR,WDIE);   // Watchdog Enable

        #ifdef DEBUG
          Serial.println(F("DEBUG: Wake-Up! "));
        #endif
        
        wake_time = millis()+STANDBY_TIME; 

        // Check for battery status after wake up
        // ---------------------------------------------------------------- //
        tmp_vccRead = vccReadMin(); 

        #ifdef DEBUG
          Serial.print(F("DEBUG: Check for battery status... "));
          Serial.print(tmp_vccRead/1000.0,3);
          Serial.print(F("v "));
        #endif

        if ((uint16_t)tmp_vccRead < (uint16_t)(BATT_MIN*1000)){
            #ifdef DEBUG
              Serial.print(F("Error! (minimal for wake-up: "));
              Serial.print(BATT_MIN,3);
              Serial.print(F("v) Forcing hard reset... "));
            wait_serial();
            #endif

            while(1); // Force hardware reset by WDT
            
        } 

        #ifdef DEBUG
          Serial.print(F("OK! (minimal for wake-up: "));
          Serial.print(BATT_MIN,3);
          Serial.println(F("v)"));
        #endif

        #ifdef DEBUG
          Serial.print(F("DEBUG: Check for BLE Start... "));
        #endif

        // Turn nRF52 ON
        // ---------------------------------------------------------------- //
        wdt_reset();                      // WDT reset to prevent overflow
        digitalWrite(RFPTT_PIN,  HIGH);   
        delay(WAIT_NRF52);                // Wait for nRF52 boot 
        wdt_reset();                      // WDT reset to prevent overflow

        #ifdef DEBUG
          Serial.println(F("OK!"));
        #endif


        // Check for I2C again
        // ---------------------------------------------------------------- //
        #ifdef DEBUG
          Serial.print(F("DEBUG: Check for I2C SLC... "));
          wait_serial();
        #endif
        
        pinMode(PIN_I2C_SCL, OUTPUT);
        digitalWrite(PIN_I2C_SCL,LOW);
        pinMode(PIN_I2C_SCL, INPUT);
      
        I2Cready = digitalRead(PIN_I2C_SCL);
      
        if (I2Cready){
          
          Wire.begin();
      
          #ifdef DEBUG
            Serial.println(F("OK! pull-up detected"));
          #endif 

        }else{

          #ifdef DEBUG
            Serial.print(F("Error! no SCL pull-up. Forcing hard reset... "));
            wait_serial();
          #endif
          
          wdt_reset();                    // WDT reset to prevent overflow

          // Force TURN OFF nRF52
          digitalWrite(RFPTT_PIN, LOW);   // Set low ppt pin
          delay(WAIT_NRF52);              // Wait for nRF52 stop
    
          while(1);                       // Force hardware reset by WDT
        }

        // Send Wake and batt code
        // ---------------------------------------------------------------- //
        #ifdef DEBUG        // Sending prepare for go to sleep --> 
          Serial.print(F("DEBUG: Sending prepare for wake-up     --> "));
        #endif

        tx_data_code(uint16_t(turnIndex),(uint8_t)WAKE_CODE); 
        wdt_reset(); // WDT reset to prevent overflow

        tmp_vccRead = vccReadAvg();           // Read battery again, after IMU&BLE start
        last_vccRead = tmp_vccRead;           

        #ifdef DEBUG
          Serial.print(F("DEBUG: Sending battery status ("));
          Serial.print(tmp_vccRead/1000.0,3);
          Serial.print(F("v) --> "));
        #endif

        tx_data_code((uint16_t)tmp_vccRead,(uint8_t)BATT_CODE);  
        wdt_reset(); // WDT reset to prevent overflow
        
        // ---------------------------------------------------------------- //
        
    }else{

      tmp_vccRead = vccReadAvg(); 
      
      if ( abs(last_vccRead-(int16_t)tmp_vccRead ) >= 100 ){ 

         last_vccRead = tmp_vccRead;

         #ifdef DEBUG
          Serial.print(F("DEBUG: Sending battery change ("));
          Serial.print(tmp_vccRead/1000.0,3);
          Serial.print(F("v) --> "));     
         #endif
         
         tx_data_code((uint16_t)tmp_vccRead ,(uint8_t)BATT_CODE);  
         wdt_reset(); // WDT reset to prevent overflow

      }

      if ((uint16_t)tmp_vccRead < (uint16_t)(BATT_MIN*1000)){
          #ifdef DEBUG
          Serial.print(F("DEBUG: Error! Battery too low. Forcing hard reset... "));
          wait_serial();
          #endif

          while(1);  // Force hardware reset by WDT
      } 
    }
  }
}// END LOOP

// ----------------------------------------------- //
// SAMPLE DEBUG OUTPUT
/*
Using board 'attiny1634' from platform in folder: /Users/jrivera/Library/Arduino15/packages/ATTinyCore/hardware/avr/1.2.3
Using core 'tinymodern' from platform in folder: /Users/jrivera/Library/Arduino15/packages/ATTinyCore/hardware/avr/1.2.3
Compiling libraries...
Compiling library "Wire"
Compiling library "RingBuf"
Usando librería Wire con versión 1.0 en la carpeta: /Users/jrivera/Library/Arduino15/packages/ATTinyCore/hardware/avr/1.2.3/libraries/Wire 
Usando librería RingBuf con versión 2.0 en la carpeta: /Users/jrivera/Documents/Arduino/libraries/RingBuf 

El Sketch usa 16324 bytes (99%) del espacio de almacenamiento de programa. El máximo es 16384 bytes.
Las variables Globales usan 486 bytes (47%) de la memoria dinámica, dejando 538 bytes para las variables locales. El máximo es 1024 bytes.

Cube11Paths v0.48 (c) ElevenPaths 2017-2019
Firmware /Users/jrivera/Documents/Arduino/Cube_Transmitter_48/Cube_Transmitter_48.ino
Compiled Feb 19 2019 13:19:42 IDE 10805 AVR GCC v4.9.2 AVR libc v2.0.0
OCS Clock: 8 MHz with perscaler divisor 1 => 8 MHz
Fuse High: 0xDD Fuse  Low: 0x62
Fuse  Ext: 0xEB Fuse Lock: 0xFF
Reset MCU: 0b111
Free  Mem: 358 bytes
QueueSize: 128 bytes (32 packets)
Time to go sleep: 180 seconds
Time to wait BLE: 25 ms
Time inter I2Ctx: 25 ms
WatchDogOverflow: 250 ms (manual)
Batt Test: 3.851v OK! (minimal for boot: 3.200v)
BLE Start: OK!
I2C Check: OK! SCL pull-up detected
IC2 Slave: 0x8
I2CBuffer: 31
Batt Last: 3.756v (average)
Boot Time: 223ms
WDT Reset: Enable!

DEBUG: Sending battery status (3.756v) --> TX 0x03 0x0EAC 0x58 ACK!
DEBUG: Sending ready code by queue tx  --> TX 0x01 0x0166 0x05 ACK!
*/
// ----------------------------------------------- //
// EOF

