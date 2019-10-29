/*
   Cube11Paths BLE Transceiver - Nordic nRF52832
   (c)ElevenPaths 2018 jorge.rivera@11paths.com 

   File "BLEconfig.hpp" --> Bluetooth LE configurations and helper functions.
   
*/

#define    DEVICE_NAME  "GiC11Paths"      // Giiker Named: GiC64839 - UUID: E91400AD-D3BE-4125-B28C-6966F7A24A01 
#define     LOCAL_NAME  DEVICE_NAME
#define  DEVICE_VENDOR "ElevenPaths"
#define   BLE_TX_POWER            4       // TX power in dBm, default value is 0 dBm, values  -40, -30, -20, -16, -12, -8, -4, 0, 4, must be called after begin.

#define DEVICE_MODEL "CubeAuth"       

extern  uint16_t IMUUpdateEvery; 
extern  uint16_t turnIndex;      
extern  uint16_t WIREErrors;     
extern   int16_t turnLosts;      
extern   uint8_t  lastCode;      
extern  uint16_t  lastData;      

extern  uint16_t IMUerror1;      // IMU frame not start as IMUFrameStart
extern  uint16_t IMUerror2;      // IMU frame not end as IMUFrameEnd 
extern  uint16_t IMUerror3;      // UART RESTARTS COUNTER
extern  uint16_t IMUerror4;      // UART ERROR REGISTER (NRF_UART0->ERRORSRC)

unsigned char giikerResponse[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0} ; // 14 bytes // sample: UUID "AAAB" Notified Value: 0x B8 A1 03 64 7A A4 00 00 00 00 00 00 00 00

const uint8_t giikerResponseSize = sizeof(giikerResponse);

//    Giiker Services and Caracteritics
//    cubeService:        "0000aadb-0000-1000-8000-00805f9b34fb",
//    cubeCharacteristic: "0000aadc-0000-1000-8000-00805f9b34fb" (Read) (Suscribe) Sample value (20 bytes): 0x1234567833333333123456789ABC0000 11 13 11 13 

#include <BLEPeripheral.h> // https://github.com/sandeepmistry/arduino-BLEPeripheral/blob/master/API.md

    BLEPeripheral   BLE;

    // BLE VENDOR SERVICE (0x180A) AND CHARACTERISTIC (0x2A29)
    BLEService                vendorService = BLEService("180A");                                                          // 0x180A => "org.bluetooth.service.device_information" GATT Sercices https://www.bluetooth.com/specifications/gatt/services                                                        
    BLECharacteristic  vendorCharacteristic = BLECharacteristic("2A29", BLERead, BLE_ATTRIBUTE_MAX_VALUE_LENGTH);          // 0x2A29 => "org.bluetooth.characteristic.manufacturer_name_string" GATT Characteristics https://www.bluetooth.com/specifications/gatt/characteristics
    BLECharacteristic   modelCharacteristic = BLECharacteristic("2A24", BLERead, BLE_ATTRIBUTE_MAX_VALUE_LENGTH);          // 0x2A24 => "org.bluetooth.characteristic.model_number_string" 
    BLECharacteristic    dateCharacteristic = BLECharacteristic("2A0A", BLERead, BLE_ATTRIBUTE_MAX_VALUE_LENGTH);          // 0x2A0A => "org.bluetooth.characteristic.day_date_time" 
    
    // BLE BATTERY LEVEL SERVICE (0x180F) AND CHARACTERISTIC (0x2A19) 
    BLEService                    batteryService = BLEService("180F");                                                      // 0x180F => "org.bluetooth.service.battery_service" 
    BLECharCharacteristic  batteryCharacteristic = BLECharCharacteristic("2A19", BLERead|BLENotify);                        // 0x2A19 => "org.bluetooth.characteristic.battery_level"  GATT Descriptors https://www.bluetooth.com/specifications/gatt/descriptors org.bluetooth.unit.percentage

    BLEUnsignedShortCharacteristic         batteryStateChar =   BLEUnsignedShortCharacteristic("2A1B", BLERead|BLENotify);  // 0x2A1B => "org.bluetooth.characteristic.battery_level_state"  GATT Descriptors https://www.bluetooth.com/specifications/gatt/descriptors
    
    // BLE CUBE MOVE SERVICE (0xAADB) AND CHARACTERISTIC (0xAADC)
    BLEService               moveService =        BLEService("0000aadb-0000-1000-8000-00805f9b34fb");   
    BLECharacteristic moveCharacteristic = BLECharacteristic("0000aadc-0000-1000-8000-00805f9b34fb", BLERead|BLENotify, BLE_ATTRIBUTE_MAX_VALUE_LENGTH); // BLE_ATTRIBUTE_MAX_VALUE_LENGTH = 20 

    // BLE SERVICE (0xAAAA) AND CHARACTERISTICS (0xAAAB) AND (0xAAAC) FOR GIIKER EMULATION
    BLEService            giikerAAAAService =                   BLEService("0000aaaa-0000-1000-8000-00805f9b34fb"); 
    BLECharacteristic     giikerAAABCharacteristic =     BLECharacteristic("0000aaab-0000-1000-8000-00805f9b34fb",  BLENotify, giikerResponseSize);
    BLECharCharacteristic giikerAAACCharacteristic = BLECharCharacteristic("0000aaac-0000-1000-8000-00805f9b34fb",  BLEWrite | BLEWriteWithoutResponse );


    // BLE CUBE IMU SERVICE (NORDIC UART) AND CHARACTERISTIC (NORDIC UART TX)
    BLEService               imuService =        BLEService("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");   // The Nordic UART Service. The 128-bit vendor-specific UUID of the Nordic UART Service
    BLEDescriptor  imuServiceDescriptor =     BLEDescriptor("2901", "IMU SERVICE");
    BLECharacteristic         imuCharTX = BLECharacteristic("6E400003-B5A3-F393-E0A9-E50E24DCCA9E", BLERead|BLENotify, BLE_ATTRIBUTE_MAX_VALUE_LENGTH);
    BLEDescriptor     imuCharDescriptor =     BLEDescriptor("2901", "IMU DATA");

    // BLE CUBE OWN SERVICE 
    BLEService                                  cubeService = BLEService("C11B");  
    BLEDescriptor                     cubeServiceDescriptor = BLEDescriptor("2904", "Cube11paths"); // Change 2901 to C11B or 2902 or 2A3D String org.bluetooth.characteristic.string 0x2A3D
    
    BLEUnsignedShortCharacteristic            turnIndexChar = BLEUnsignedShortCharacteristic("AA00", BLERead|BLENotify); // BLE Characteristic to read turnIndex  (uint16_t)
    BLEShortCharacteristic                    turnLostsChar =         BLEShortCharacteristic("AA01", BLERead|BLENotify); // BLE Characteristic to read turn losts (int16_t)
    BLEUnsignedShortCharacteristic           WIREErrorsChar = BLEUnsignedShortCharacteristic("AA02", BLERead|BLENotify); // BLE Characteristic to read crc error count (uint16_t)
    BLECharCharacteristic                      lastCodeChar =          BLECharCharacteristic("AA03", BLERead|BLENotify); // BLE Characteristic to last code  (uint8_t)
    BLEUnsignedShortCharacteristic             lastDataChar = BLEUnsignedShortCharacteristic("AA04", BLERead|BLENotify); // BLE Characteristic to last data (uint16_t)
    BLEUnsignedShortCharacteristic       IMUUpdateEveryChar = BLEUnsignedShortCharacteristic("AA05", BLERead|BLEWrite | BLEWriteWithoutResponse);  // BLE Characteristic to manage IMUUpdateEvery

    BLEUnsignedShortCharacteristic            IMUerror1Char = BLEUnsignedShortCharacteristic("AA06", BLERead|BLENotify); // BLE Characteristic to read IMUerror1  (uint16_t)
    BLEUnsignedShortCharacteristic            IMUerror2Char = BLEUnsignedShortCharacteristic("AA07", BLERead|BLENotify); // BLE Characteristic to read IMUerror2  (uint16_t)
    BLEUnsignedShortCharacteristic            IMUerror3Char = BLEUnsignedShortCharacteristic("AA08", BLERead|BLENotify); // BLE Characteristic to read IMUerror3  (uint16_t) 
    BLEUnsignedShortCharacteristic            IMUerror4Char = BLEUnsignedShortCharacteristic("AA09", BLERead|BLENotify); // BLE Characteristic to read IMUerror4  (uint16_t) 
        
// ---------------------------------------------------------------------------- //


#ifdef DEBUG
void blePeripheralConnectHandler(BLECentral& central) {
  // central connected event handler
  Serial.print(F("Connected event, central: "));
  Serial.println(central.address());
}

void blePeripheralDisconnectHandler(BLECentral& central) {
  // central disconnected event handler
  Serial.print(F("Disconnected event, central: "));
  Serial.println(central.address());
}

void characteristicSubscribed(BLECentral& central, BLECharacteristic& characteristic) {
  // characteristic subscribed event handler
  Serial.print(F("Characteristic event, subscribed: "));
  Serial.println(characteristic.uuid());
}

void characteristicUnsubscribed(BLECentral& central, BLECharacteristic& characteristic) {
  // characteristic unsubscribed event handler
  Serial.print(F("Characteristic event, unsubscribed: "));
  Serial.println(characteristic.uuid());
}
#endif

// ---------------------------------------------------------------------------- //
// characteristic value written event handler
void IMUUpdateEveryCharUpdate(BLECentral& central, BLECharacteristic& characteristic) {
    
    uint16_t readValue = 0x0000;    
    memcpy(&readValue,characteristic.value(),characteristic.valueLength());
    readValue = (readValue << 8) | (readValue >> 8 );

    IMUUpdateEvery = readValue;    
    
    IMUUpdateEveryChar.setValue(IMUUpdateEvery);

    #ifdef DEBUG
      Serial.print(F("Characteristic: "));
      Serial.print(characteristic.uuid());
      Serial.print(F(" event, writen value: "));
      Serial.println(IMUUpdateEvery,DEC);  
    #endif
}

// ---------------------------------------------------------------------------- //
// CHARACTERISTIC MANAGE (0xAAAC) FOR GIIKER EMULATION
void giikerAAACCharUpdate(BLECentral& central, BLECharacteristic& characteristic) {

  uint8_t readValue = 0;
  memcpy(&readValue,characteristic.value(),1 ); // characteristic.valueLength()

  switch (readValue){

      case 0xB5:      // battery read
          giikerResponse[0] = 0xB5;
          giikerResponse[1] = batteryCharacteristic.value();
          giikerAAABCharacteristic.setValue(giikerResponse,giikerResponseSize);
      break;

      case 0xA1:      // Reset
          giikerResponse[0] = 0xA1;
          giikerResponse[1] = 0xA1;
          giikerAAABCharacteristic.setValue(giikerResponse,giikerResponseSize);
      break; 
  }
  
}

// ---------------------------------------------------------------------------- //

   // custom services and characteristics can be added as well
   // const uint8_t manufacturerData[4] = {0x12, 0x34, 0x56, 0x78};
   // blePeripheral.setManufacturerData(manufacturerData, sizeof(manufacturerData));

void BLEinit(){
  
  BLE.setDeviceName(DEVICE_NAME);    // device name, up to: 20 characters on nRF8001 and nRF51822 - default value is "Arduino"
  BLE.setAppearance(0x0000);         // appearance, default value is 0x0000
  BLE.setLocalName(LOCAL_NAME);

  #ifdef DEBUG
    BLE.setEventHandler(BLEConnected, blePeripheralConnectHandler);
    BLE.setEventHandler(BLEDisconnected, blePeripheralDisconnectHandler);
  #endif
 
  // BLE VENDOR SERVICE (0x180A) AND CHARACTERISTIC (0x2A29)
  BLE.addAttribute(vendorService);
  BLE.setAdvertisedServiceUuid(vendorService.uuid()); 
  BLE.addAttribute(vendorCharacteristic);
  vendorCharacteristic.setValue(DEVICE_VENDOR);

  BLE.addAttribute(modelCharacteristic);  

  String _model = DEVICE_MODEL;   
  _model += " v0.";               
  _model += String(MINOR_VERSION);
  
  modelCharacteristic.setValue(_model.c_str()); 

  BLE.addAttribute(dateCharacteristic);  
  dateCharacteristic.setValue(__DATE__ " " __TIME__ );
  
  // BLE BATTERY LEVEL SERVICE (0x180F) AND CHARACTERISTIC (0x2A19)
  BLE.addAttribute(batteryService);
  BLE.setAdvertisedServiceUuid(batteryService.uuid()); 
  BLE.addAttribute(batteryCharacteristic);

  BLE.addAttribute(batteryStateChar);

  batteryCharacteristic.setValue(0x00);
  batteryStateChar.setValue(0x0000);

  #ifdef DEBUG
    batteryCharacteristic.setEventHandler(BLESubscribed, characteristicSubscribed);     
    batteryCharacteristic.setEventHandler(BLEUnsubscribed, characteristicUnsubscribed);  
    batteryStateChar.setEventHandler(BLESubscribed, characteristicSubscribed);      
    batteryStateChar.setEventHandler(BLEUnsubscribed, characteristicUnsubscribed);  
  #endif

  // BLE CUBE MOVE SERVICE (0xAADF) AND CHARACTERISTIC (0xAADC)
  BLE.addAttribute(moveService);
  BLE.setAdvertisedServiceUuid(moveService.uuid()); 
  BLE.addAttribute(moveCharacteristic);

  #ifdef DEBUG
    moveCharacteristic.setEventHandler(BLESubscribed, characteristicSubscribed);
    moveCharacteristic.setEventHandler(BLEUnsubscribed, characteristicUnsubscribed);
  #endif

  // BLE CUBE IMU SERVICE (NORDIC UART) AND CHARACTERISTIC (NORDIC UART TX)
  BLE.addAttribute(imuService);
  BLE.setAdvertisedServiceUuid(imuService.uuid()); 
  BLE.addAttribute(imuServiceDescriptor);  // attribute - attribute to add, can be BLEService, BLECharacteristic, or BLEDescriptor
  BLE.addAttribute(imuCharTX); 
  BLE.addAttribute(imuCharDescriptor);  


  // BLE CUBE OWN SERVICE AND CHARACTERISTICS 
  BLE.addAttribute(cubeService);
  BLE.setAdvertisedServiceUuid(cubeService.uuid()); 
  
  BLE.addAttribute(turnIndexChar);
  BLE.addAttribute(turnLostsChar);
  BLE.addAttribute(WIREErrorsChar);
  BLE.addAttribute(lastCodeChar);
  BLE.addAttribute(lastDataChar);
  BLE.addAttribute(IMUUpdateEveryChar);

  BLE.addAttribute(IMUerror1Char);
  BLE.addAttribute(IMUerror2Char);
  BLE.addAttribute(IMUerror3Char);
  BLE.addAttribute(IMUerror4Char);

  #ifdef DEBUG
    turnIndexChar.setEventHandler(BLESubscribed, characteristicSubscribed);
    turnIndexChar.setEventHandler(BLEUnsubscribed, characteristicUnsubscribed);
    turnLostsChar.setEventHandler(BLESubscribed, characteristicSubscribed);
    turnLostsChar.setEventHandler(BLEUnsubscribed, characteristicUnsubscribed);
    WIREErrorsChar.setEventHandler(BLESubscribed, characteristicSubscribed);
    WIREErrorsChar.setEventHandler(BLESubscribed, characteristicSubscribed);
    lastCodeChar.setEventHandler(BLESubscribed, characteristicSubscribed);
    lastCodeChar.setEventHandler(BLEUnsubscribed, characteristicUnsubscribed);
    lastDataChar.setEventHandler(BLESubscribed, characteristicSubscribed);
    lastDataChar.setEventHandler(BLEUnsubscribed, characteristicUnsubscribed);
  #endif

  IMUUpdateEveryChar.setEventHandler(BLEWritten, IMUUpdateEveryCharUpdate);

  // BLE SERVICE (0xAAAA) AND CHARACTERISTICS (0xAAAB) AND (0xAAAC) FOR GIIKER EMULATION
  BLE.addAttribute(giikerAAAAService);
  BLE.setAdvertisedServiceUuid(giikerAAAAService.uuid());
  BLE.addAttribute(giikerAAABCharacteristic);
  BLE.addAttribute(giikerAAACCharacteristic);
   
  giikerAAACCharacteristic.setEventHandler(BLEWritten,giikerAAACCharUpdate);

  turnIndexChar.setValue(turnIndex);
  turnLostsChar.setValue(turnLosts);
  WIREErrorsChar.setValue(WIREErrors);
  lastCodeChar.setValue(lastCode);
  lastDataChar.setValue(lastData);
  IMUUpdateEveryChar.setValue(IMUUpdateEvery);

  IMUerror1Char.setValue(IMUerror1);
  IMUerror2Char.setValue(IMUerror2);
  IMUerror3Char.setValue(IMUerror3);
  IMUerror4Char.setValue(IMUerror4);

  #ifdef DEBUG
    IMUerror1Char.setEventHandler(BLESubscribed, characteristicSubscribed);     
    IMUerror1Char.setEventHandler(BLEUnsubscribed, characteristicUnsubscribed); 
    IMUerror2Char.setEventHandler(BLESubscribed, characteristicSubscribed);     
    IMUerror2Char.setEventHandler(BLEUnsubscribed, characteristicUnsubscribed); 
    IMUerror3Char.setEventHandler(BLESubscribed, characteristicSubscribed);     
    IMUerror3Char.setEventHandler(BLEUnsubscribed, characteristicUnsubscribed); 
    IMUerror4Char.setEventHandler(BLESubscribed, characteristicSubscribed);     
    IMUerror4Char.setEventHandler(BLEUnsubscribed, characteristicUnsubscribed); 
    imuCharTX.setEventHandler(BLESubscribed, characteristicSubscribed);
    imuCharTX.setEventHandler(BLEUnsubscribed, characteristicUnsubscribed);
  #endif

  BLE.begin();

  BLE.setTxPower(BLE_TX_POWER); 

}
// -------------------------------------------- //
// EOF

