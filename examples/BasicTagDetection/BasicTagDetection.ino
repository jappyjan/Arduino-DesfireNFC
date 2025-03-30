/**
 * BasicTagDetection.ino - Example for detecting and reading MIFARE DESFire cards
 *
 * This example shows how to:
 * 1. Detect a DESFire card
 * 2. Read its UID
 * 3. Get version information
 *
 * Circuit:
 * - ESP32 board
 * - PN532 NFC reader connected via I2C, SPI, or UART
 */

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>
#include <DesfireNFC.h>
#include <PN532Reader.h>

// Uncomment the type of connection you're using
// For I2C
#define PN532_I2C

// For SPI
//#define PN532_SPI

// For HSU (Serial)
//#define PN532_HSU

// Global pointers for the reader and DESFire instance
PN532Reader* nfcReader = nullptr;
DesfireNFC* desfire = nullptr;

void setup() {
  Serial.begin(115200);
  delay(2000);
  while (!Serial) delay(10);  // For boards like ESP32-C3 with native USB
  
  Serial.println("Arduino-DesfireNFC Basic Tag Detection Example");
  
  // Initialize appropriate reader based on connection type
  #ifdef PN532_I2C
    // For ESP32 boards with I2C
    #define PN532_IRQ   (4)
    #define PN532_RESET (5)  // Not connected by default on the NFC Shield
    nfcReader = new PN532Reader(PN532_IRQ, PN532_RESET, Wire);
  #endif

  #ifdef PN532_SPI
    // For SPI connection
    #define PN532_SS    (10)
    nfcReader = new PN532Reader(PN532_SS);
  #endif

  #ifdef PN532_HSU
    // For UART connection
    #define PN532_RX    (2)
    #define PN532_TX    (3)
    nfcReader = new PN532Reader(PN532_TX, PN532_RX);
  #endif
  
  // Initialize the DESFire library with the reader
  desfire = new DesfireNFC(*nfcReader);
  
  nfcReader->begin();
  
  uint32_t versiondata = nfcReader->getFirmwareVersion();
  if (!versiondata) {
    Serial.println("Didn't find PN53x board");
    while (1) delay(10);  // halt
  }
  
  // Print out the PN532 firmware version
  Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX); 
  Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC); 
  Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);
  
  // Configure board to read RFID tags
  nfcReader->configure();
  
  Serial.println("Waiting for a DESFire card...");
}

void loop() {
    Serial.print('.');
  // Try to detect a card
  if (desfire->detectCard()) {
    Serial.println("DESFire card detected!");
    
    // Read and display the card's UID
    uint8_t uid[7];  // DESFire UIDs are 7 bytes
    uint8_t uidLength;
    if (desfire->getCardUID(uid, &uidLength)) {
      Serial.print("UID: ");
      for (uint8_t i = 0; i < uidLength; i++) {
        if (uid[i] < 0x10) Serial.print("0");  // Leading zero for proper formatting
        Serial.print(uid[i], HEX);
        Serial.print(" ");
      }
      Serial.println();
      
      // Get card version information
      DESFireCardVersion version;
      if (desfire->getVersion(&version) == DesfireStatus::DFST_SUCCESS) {
        Serial.println("Card Version Information:");
        Serial.print("Card Type: ");
        Serial.println(version.getCardTypeName());
        
        Serial.print("Hardware Version: ");
        Serial.print(version.hardwareVersionMajor);
        Serial.print(".");
        Serial.println(version.hardwareVersionMinor);
        
        Serial.print("Software Version: ");
        Serial.print(version.softwareVersionMajor);
        Serial.print(".");
        Serial.println(version.softwareVersionMinor);
        
        Serial.print("Storage Size: ");
        Serial.print(version.getStorageSize());
        Serial.println(" bytes");
        
        Serial.print("Hardware Protocol: 0x");
        Serial.println(version.hardwareProtocol, HEX);
      } else {
        Serial.println("Failed to read card version information");
      }
    } else {
      Serial.println("Failed to read card UID");
    }
    
    // Wait before trying to detect another card
    Serial.println("Waiting 2 seconds before next detection...");
    delay(2000);
  }
  
  delay(500);  // Short delay between detection attempts
} 