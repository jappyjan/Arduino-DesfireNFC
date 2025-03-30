/**
 * @file DesfireDebuggingDemo.ino
 * @brief DESFire Debug Logging Demonstration
 * 
 * This example shows how to use the debug logging functionality in the DesfireNFC 
 * library to help troubleshoot communication issues with DESFire cards.
 * 
 * Features demonstrated:
 * - Setting debug levels (NONE, ERROR, INFO, VERBOSE)
 * - Reading card version information with detailed debug output
 * - Examining NFC communication problems 
 * 
 * Circuit:
 * - PN532 NFC module connected via SPI, I2C, or UART
 * - DESFire EV1/EV2 card
 */

#include <SPI.h>
#include <Wire.h>
#include "DesfireNFC.h"
#include "NFCReaderInterface.h"
#include "PN532Reader.h"
#include "DesfireErrorHandler.h"

// Configure the appropriate reader implementation
// Uncomment one of these depending on your hardware setup:

// For I2C connection with default pins (IRQ=4, RESET=5)
PN532Reader nfcReader; 

// For SPI connection
// #define PN532_SS   (10)
// PN532Reader nfcReader(PN532_SS);

// For I2C connection with custom pins
// #define PN532_IRQ   (2)
// #define PN532_RESET (3)
// PN532Reader nfcReader(PN532_IRQ, PN532_RESET);

// For UART connection
// #define PN532_TX (8)
// #define PN532_RX (9)
// PN532Reader nfcReader(PN532_TX, PN532_RX);

// Create DESFire instance with verbose debugging enabled
DesfireNFC desfire(nfcReader, DesfireNFC::DEBUG_VERBOSE);

// Flag to track if detection is in progress
bool detectingCard = false;

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  while (!Serial) {
    ; // Wait for serial port to connect
  }
  
  Serial.println(F("DESFire Debugging Demonstration"));
  Serial.println(F("------------------------------"));
  
  // Initialize NFC reader
  if (!desfire.initialize()) {
    Serial.println(F("Error: Failed to initialize NFC reader!"));
    Serial.println(F("Check connections and try again."));
    while (1); // Halt
  }
  
  Serial.println(F("NFC reader initialized successfully."));
  printHelp();
}

void loop() {
  // Process serial commands when available
  if (Serial.available()) {
    char cmd = Serial.read();
    processCommand(cmd);
  }

  // If in card detection mode, try to detect and print card info
  if (detectingCard) {
    if (desfire.detectCard()) {
      // Read and display the card's UID
      uint8_t uid[7];  // DESFire UIDs are 7 bytes
      uint8_t uidLength;
      
      Serial.println(F("\nDESFire card detected!"));
      
      if (desfire.getCardUID(uid, &uidLength)) {
        Serial.print(F("UID: "));
        for (uint8_t i = 0; i < uidLength; i++) {
          if (uid[i] < 0x10) Serial.print("0");
          Serial.print(uid[i], HEX);
          Serial.print(" ");
        }
        Serial.println();
      } else {
        Serial.println(F("Failed to read card UID!"));
      }
      
      // Get card version with current debug level
      DESFireCardVersion version;
      DesfireStatus status = desfire.getVersion(&version);
      
      if (status == DesfireStatus::DFST_SUCCESS) {
        Serial.println(F("Successfully read card version information!"));
        Serial.print(F("Card Type: "));
        Serial.println(version.getCardTypeName());
      } else {
        Serial.println(F("Failed to read card version information"));
        Serial.println(F("DESFire Error:"));
        Serial.print(F("  Operation: getVersion"));
        Serial.print(F("\n  Status code: 0x"));
        Serial.println(static_cast<uint8_t>(status), HEX);
        
        // Print detailed error information
        DesfireErrorHandler::printErrorReport(status, "getVersion");
      }
      
      // Wait before next detection cycle to avoid repeated reads
      Serial.println(F("Waiting 2 seconds before next detection..."));
      delay(2000);
    }
    delay(100);  // Short delay between detection attempts
  }
}

void processCommand(char cmd) {
  switch (cmd) {
    case '0':
      desfire.setDebugLevel(DesfireNFC::DEBUG_NONE);
      Serial.println(F("\nDebug level set to NONE"));
      break;
      
    case '1':
      desfire.setDebugLevel(DesfireNFC::DEBUG_ERROR);
      Serial.println(F("\nDebug level set to ERROR"));
      break;
      
    case '2':
      desfire.setDebugLevel(DesfireNFC::DEBUG_INFO);
      Serial.println(F("\nDebug level set to INFO"));
      break;
      
    case '3':
      desfire.setDebugLevel(DesfireNFC::DEBUG_VERBOSE);
      Serial.println(F("\nDebug level set to VERBOSE"));
      break;
      
    case 'd':
    case 'D':
      if (!detectingCard) {
        detectingCard = true;
        Serial.println(F("\nCard detection started. Place a card on the reader..."));
        Serial.println(F("Press 's' to stop detection"));
      }
      break;
      
    case 's':
    case 'S':
      if (detectingCard) {
        detectingCard = false;
        Serial.println(F("\nCard detection stopped"));
      }
      break;
      
    case 'v':
    case 'V':
      // Attempt to get the card version without continuous detection mode
      if (desfire.detectCard()) {
        DESFireCardVersion version;
        Serial.println(F("\nReading card version..."));
        DesfireStatus status = desfire.getVersion(&version);
        
        if (status == DesfireStatus::DFST_SUCCESS) {
          // Print detailed version info
          printVersionInfo(version);
        } else {
          Serial.println(F("Failed to read card version!"));
          DesfireErrorHandler::printErrorReport(status, "getVersion");
        }
      } else {
        Serial.println(F("\nNo card detected. Place a card on the reader."));
      }
      break;
      
    case 'h':
    case 'H':
    case '?':
      printHelp();
      break;
      
    case '\r':
    case '\n':
      // Ignore newlines and carriage returns
      break;
      
    default:
      Serial.println(F("\nUnknown command. Press 'h' for help."));
      break;
  }
}

void printHelp() {
  Serial.println(F("\n--- Command Menu ---"));
  Serial.println(F("0: Set debug level to NONE"));
  Serial.println(F("1: Set debug level to ERROR"));
  Serial.println(F("2: Set debug level to INFO"));
  Serial.println(F("3: Set debug level to VERBOSE"));
  Serial.println(F("d: Start continuous card detection"));
  Serial.println(F("s: Stop continuous card detection"));
  Serial.println(F("v: Read version information once"));
  Serial.println(F("h: Display this help message"));
  Serial.println(F("------------------"));
}

void printVersionInfo(const DESFireCardVersion& version) {
  Serial.println(F("\n=== CARD VERSION INFORMATION ==="));
  
  // Card type
  Serial.print(F("Card Type: "));
  Serial.println(version.getCardTypeName());
  
  // Hardware information
  Serial.println(F("\n-- Hardware Information --"));
  Serial.print(F("Vendor ID: 0x"));
  Serial.println(version.hardwareVendor, HEX);
  Serial.print(F("Version: "));
  Serial.print(version.hardwareVersionMajor);
  Serial.print(".");
  Serial.println(version.hardwareVersionMinor);
  
  // Software information
  Serial.println(F("\n-- Software Information --"));
  Serial.print(F("Version: "));
  Serial.print(version.softwareVersionMajor);
  Serial.print(".");
  Serial.println(version.softwareVersionMinor);
  
  // Storage
  Serial.print(F("Storage Size: "));
  Serial.print(version.getStorageSize());
  Serial.println(F(" bytes"));
  
  // UID
  Serial.println(F("\n-- Card Identifier --"));
  Serial.print(F("UID: "));
  for (int i = 0; i < 7; i++) {
    if (version.uid[i] < 0x10) Serial.print("0");
    Serial.print(version.uid[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
  
  Serial.println(F("\n=================================="));
} 