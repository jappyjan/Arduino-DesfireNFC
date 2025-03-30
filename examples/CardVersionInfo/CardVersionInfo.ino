/**
 * @file CardVersionInfo.ino
 * @brief DESFire Card Version Information Example
 * 
 * This example demonstrates how to retrieve and display detailed
 * version information from a MIFARE DESFire card using the DesfireNFC library.
 * 
 * The version information includes hardware/software details, storage size, 
 * card type identification, production information, and unique identifier.
 * 
 * Circuit:
 * - PN532 NFC module connected via SPI, I2C, or UART
 * - DESFire EV1/EV2 card
 * 
 * Created [current date]
 */

#include <SPI.h>
#include <Wire.h>
#include "DesfireNFC.h"
#include "NFCReaderInterface.h"
#include "PN532Reader.h"

// Configure the appropriate reader implementation
// Uncomment one of these depending on your hardware setup:

// For SPI connection
#define PN532_SCK  (13)
#define PN532_MISO (12)
#define PN532_MOSI (11)
#define PN532_SS   (10)
PN532Reader nfcReader(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);

// For I2C connection
// PN532Reader nfcReader(PN532_I2C);

// For UART connection
// PN532Reader nfcReader(PN532_UART);

// Create DESFire instance
DesfireNFC desfire(nfcReader);

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  while (!Serial) {
    ; // Wait for serial port to connect
  }
  
  Serial.println("DESFire Card Version Information Example");
  Serial.println("-----------------------------------------");
  
  // Initialize NFC reader
  if (!desfire.initialize()) {
    Serial.println("Error: Failed to initialize NFC reader!");
    while (1); // Halt
  }
  
  Serial.println("NFC reader initialized successfully.");
  Serial.println("Place a DESFire card on the reader...");
}

void loop() {
  // Try to detect a card
  if (desfire.detectCard()) {
    Serial.println("\n--- DESFire card detected! ---");
    
    // Read and display the card's UID
    uint8_t uid[7];  // DESFire UIDs are 7 bytes
    uint8_t uidLength;
    if (desfire.getCardUID(uid, &uidLength)) {
      Serial.print("Card UID: ");
      for (uint8_t i = 0; i < uidLength; i++) {
        if (uid[i] < 0x10) Serial.print("0");  // Leading zero for proper formatting
        Serial.print(uid[i], HEX);
        if (i < uidLength - 1) Serial.print(":");
      }
      Serial.println();
    } else {
      Serial.println("Failed to read card UID!");
    }
    
    // Get and display comprehensive card version information
    DESFireCardVersion version;
    if (desfire.getVersion(&version)) {
      Serial.println("\n=== CARD VERSION INFORMATION ===");
      
      // Card type identification
      Serial.print("Card Type: ");
      Serial.println(version.getCardTypeName());
      
      // Hardware information
      Serial.println("\n-- Hardware Information --");
      Serial.print("Vendor ID: 0x");
      Serial.println(version.hardwareVendor, HEX);
      Serial.print("Type: 0x");
      Serial.println(version.hardwareType, HEX);
      Serial.print("Subtype: 0x");
      Serial.println(version.hardwareSubtype, HEX);
      Serial.print("Version: ");
      Serial.print(version.hardwareVersionMajor);
      Serial.print(".");
      Serial.println(version.hardwareVersionMinor);
      Serial.print("Protocol: 0x");
      Serial.println(version.hardwareProtocol, HEX);
      
      // Software information
      Serial.println("\n-- Software Information --");
      Serial.print("Vendor ID: 0x");
      Serial.println(version.softwareVendor, HEX);
      Serial.print("Type: 0x");
      Serial.println(version.softwareType, HEX);
      Serial.print("Subtype: 0x");
      Serial.println(version.softwareSubtype, HEX);
      Serial.print("Version: ");
      Serial.print(version.softwareVersionMajor);
      Serial.print(".");
      Serial.println(version.softwareVersionMinor);
      Serial.print("Protocol: 0x");
      Serial.println(version.softwareProtocol, HEX);
      
      // Storage information
      Serial.println("\n-- Storage Information --");
      Serial.print("Storage Size: ");
      Serial.print(version.getStorageSize());
      Serial.println(" bytes");
      
      // Production information
      Serial.println("\n-- Production Information --");
      Serial.print("Batch Number: ");
      for (int i = 0; i < 5; i++) {
        if (version.batchNumber[i] < 0x10) Serial.print("0");
        Serial.print(version.batchNumber[i], HEX);
      }
      Serial.println();
      Serial.print("Production Week: ");
      Serial.println(version.productionWeek, HEX);
      Serial.print("Production Year: 20");
      Serial.println(version.productionYear, HEX);
      
      Serial.println("\n==================================");
    } else {
      Serial.println("Failed to read card version information!");
    }
    
    // Wait before trying to detect another card
    Serial.println("\nRemove card and place a new one...");
    delay(3000);  // Debounce to prevent repeated detection of the same card
  }
  
  delay(100);  // Short delay between detection attempts
} 