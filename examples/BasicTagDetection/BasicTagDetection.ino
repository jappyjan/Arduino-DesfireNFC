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

// Uncomment the type of connection you're using
// For I2C
#define PN532_I2C

// For SPI
//#define PN532_SPI

// For HSU (Serial)
//#define PN532_HSU

#ifdef PN532_I2C
  // For ESP32 boards
  #define PN532_IRQ   (2)
  #define PN532_RESET (3)  // Not connected by default on the NFC Shield
  Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);
#endif

#ifdef PN532_SPI
  #define PN532_SS    (10)
  #define PN532_IRQ   (2)
  #define PN532_RESET (3)  // Not connected by default on the NFC Shield
  Adafruit_PN532 nfc(PN532_SS);
#endif

#ifdef PN532_HSU
  #define PN532_RX    (2)
  #define PN532_TX    (3)
  Adafruit_PN532 nfc(PN532_TX, PN532_RX);
#endif

// Initialize DESFire library with the hardware abstraction
DesfireNFC desfire(nfc);

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);  // For boards like ESP32-C3 with native USB
  
  Serial.println("Arduino-DesfireNFC Basic Tag Detection Example");
  
  nfc.begin();
  
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.println("Didn't find PN53x board");
    while (1) delay(10);  // halt
  }
  
  // Print out the PN532 firmware version
  Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX); 
  Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC); 
  Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);
  
  // Configure board to read RFID tags
  nfc.SAMConfig();
  
  Serial.println("Waiting for a DESFire card...");
}

void loop() {
  // Try to detect a card
  if (desfire.detectCard()) {
    Serial.println("DESFire card detected!");
    
    // Read and display the card's UID
    uint8_t uid[7];  // DESFire UIDs are 7 bytes
    uint8_t uidLength = desfire.getCardUID(uid, sizeof(uid));
    
    Serial.print("UID: ");
    for (uint8_t i = 0; i < uidLength; i++) {
      if (uid[i] < 0x10) Serial.print("0");  // Leading zero for proper formatting
      Serial.print(uid[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
    
    // Get card version information
    DESFireCardVersion version;
    if (desfire.getVersion(&version)) {
      Serial.println("Card Version Information:");
      Serial.print("Hardware Type: ");
      Serial.println(version.hardwareType);
      
      Serial.print("Software Version: ");
      Serial.print(version.softwareVersionMajor);
      Serial.print(".");
      Serial.println(version.softwareVersionMinor);
      
      Serial.print("Storage Size: ");
      Serial.println(version.storageSize);
      
      Serial.print("Protocol: ");
      Serial.println(version.protocolType);
    } else {
      Serial.println("Failed to read card version information");
    }
    
    // Wait before trying to detect another card
    Serial.println("Waiting 2 seconds before next detection...");
    delay(2000);
  }
  
  delay(100);  // Short delay between detection attempts
} 