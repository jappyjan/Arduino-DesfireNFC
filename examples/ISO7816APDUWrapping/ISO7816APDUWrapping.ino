/**
 * @file ISO7816APDUWrapping.ino
 * @brief Example sketch demonstrating ISO 7816-4 APDU wrapping for DESFire cards
 *
 * This example demonstrates how to use the ISO7816Wrapper class to communicate
 * with DESFire cards using ISO 7816-4 APDU wrapping. It shows how to:
 * 1. Detect a DESFire card
 * 2. Get card version information
 * 3. Select an application
 * 4. Handle multi-frame responses
 */

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_PN532.h>
#include "DesfireNFC.h"
#include "ISO7816Wrapper.h"
#include "PN532Reader.h"

// PN532 connection pins (adjust for your hardware setup)
#if defined(ESP32)
  #define PN532_SCK  (18)
  #define PN532_MOSI (23)
  #define PN532_SS   (5)
  #define PN532_MISO (19)
#elif defined(ESP8266)
  #define PN532_SCK  (14)
  #define PN532_MOSI (13)
  #define PN532_SS   (15)
  #define PN532_MISO (12)
#elif defined(ARDUINO_AVR_UNO)
  #define PN532_SCK  (13)
  #define PN532_MOSI (11)
  #define PN532_SS   (10)
  #define PN532_MISO (12)
#else
  #error "Please define your board's pin connections for PN532"
#endif

// Create PN532 instance
Adafruit_PN532 pn532(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);

// Create PN532Reader instance (adapter for NFCReaderInterface)
PN532Reader reader(pn532);

// Create ISO7816Wrapper instance
ISO7816Wrapper iso7816Wrapper(reader);

// Buffer for card responses
uint8_t responseBuffer[256];

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  while (!Serial) delay(10);

  Serial.println("ISO 7816-4 APDU Wrapping Example");
  Serial.println("--------------------------------");

  // Initialize PN532 NFC reader
  if (!reader.begin()) {
    Serial.println("Failed to initialize PN532 reader!");
    while (1) delay(10);
  }
  
  // Check firmware version
  uint32_t firmwareVersion = reader.getFirmwareVersion();
  if (!firmwareVersion) {
    Serial.println("PN532 not found!");
    while (1) delay(10);
  }
  
  // Display firmware information
  Serial.print("Found PN532 with firmware version: ");
  Serial.print((firmwareVersion >> 24) & 0xFF, HEX); Serial.print(".");
  Serial.println((firmwareVersion >> 16) & 0xFF, HEX);
  
  // Configure reader
  reader.configure();
  
  Serial.println("Waiting for a DESFire card...");
}

void loop() {
  // Check if a card is present
  uint8_t uid[7];
  uint8_t uidLength;
  
  if (reader.detectCard(uid, &uidLength)) {
    Serial.println("Card detected!");
    
    // Print card UID
    Serial.print("UID: ");
    for (uint8_t i = 0; i < uidLength; i++) {
      Serial.print(uid[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
    
    // Demonstrate ISO 7816-4 APDU wrapping
    demoAPDUWrapping();
    
    // Wait for card removal
    Serial.println("Remove card and wait...");
    while (reader.detectCard(uid, &uidLength)) {
      delay(100);
    }
    
    Serial.println("Card removed. Ready for next card.");
    delay(1000);
  }
  
  delay(100);
}

/**
 * @brief Demonstrate ISO 7816-4 APDU wrapping functionality
 */
void demoAPDUWrapping() {
  Serial.println("Demonstrating ISO 7816-4 APDU wrapping");
  Serial.println("-------------------------------------");
  
  // Example 1: Get card version
  Serial.println("Example 1: Get card version");
  getCardVersion();
  
  // Example 2: Select application
  Serial.println("\nExample 2: Select application (AID: 000000 - PICC level)");
  uint8_t aid[3] = {0x00, 0x00, 0x00};  // Select PICC level (root)
  selectApplication(aid);
  
  Serial.println("\nAPDU wrapping demo complete.");
}

/**
 * @brief Get and display card version information
 */
void getCardVersion() {
  uint16_t responseLen = sizeof(responseBuffer);
  
  // Transmit GET_VERSION command wrapped in ISO 7816-4 APDU
  DesfireStatus status = iso7816Wrapper.transmitCommand(
      DesfireCommand::DF_CMD_GET_VERSION, nullptr, 0, responseBuffer, responseLen);
  
  if (status == DesfireStatus::DFST_SUCCESS) {
    Serial.println("Card version information:");
    
    // Parse and display version information
    if (responseLen >= 7) {
      Serial.print("Hardware: Vendor=0x");
      Serial.print(responseBuffer[0], HEX);
      Serial.print(", Type=0x");
      Serial.print(responseBuffer[1], HEX);
      Serial.print(", Subtype=0x");
      Serial.print(responseBuffer[2], HEX);
      Serial.print(", Version=");
      Serial.print(responseBuffer[3], DEC);
      Serial.print(".");
      Serial.print(responseBuffer[4], DEC);
      Serial.print(", Storage=0x");
      Serial.print(responseBuffer[5], HEX);
      Serial.print(", Protocol=0x");
      Serial.println(responseBuffer[6], HEX);
      
      // Map hardware type to card name
      Serial.print("Card type: ");
      switch (responseBuffer[1]) {
        case 0x00:
          Serial.println("DESFire");
          break;
        case 0x01:
          Serial.println("DESFire EV1");
          break;
        case 0x02:
          Serial.println("DESFire EV2");
          break;
        case 0x03:
          Serial.println("DESFire EV3");
          break;
        case 0x41:
          Serial.println("DESFire Light");
          break;
        default:
          Serial.println("Unknown");
          break;
      }
    } else {
      Serial.println("Incomplete version information received");
    }
  } else {
    Serial.print("Failed to get card version. Status: ");
    Serial.println(static_cast<int>(status), HEX);
  }
}

/**
 * @brief Select an application by AID
 * 
 * @param aid Application ID (3 bytes)
 */
void selectApplication(uint8_t* aid) {
  // Buffer for APDU
  uint8_t apdu[32];
  uint16_t apduLen = 0;
  
  // Build select application APDU
  DesfireStatus status = iso7816Wrapper.buildSelectApplicationAPDU(aid, apdu, apduLen);
  
  if (status != DesfireStatus::DFST_SUCCESS) {
    Serial.print("Failed to build select application APDU. Status: ");
    Serial.println(static_cast<int>(status), HEX);
    return;
  }
  
  // Print APDU
  Serial.print("Select Application APDU: ");
  for (uint16_t i = 0; i < apduLen; i++) {
    Serial.print(apdu[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
  
  // Transmit APDU
  uint16_t responseLen = sizeof(responseBuffer);
  status = iso7816Wrapper.transmitAPDU(apdu, apduLen, responseBuffer, responseLen);
  
  if (status == DesfireStatus::DFST_SUCCESS) {
    Serial.println("Application selected successfully");
    
    // Print response data (if any)
    if (responseLen > 0) {
      Serial.print("Response data: ");
      for (uint16_t i = 0; i < responseLen; i++) {
        Serial.print(responseBuffer[i], HEX);
        Serial.print(" ");
      }
      Serial.println();
    }
  } else {
    Serial.print("Failed to select application. Status: ");
    Serial.println(static_cast<int>(status), HEX);
  }
  
  // Alternatively, demonstrate using the native DESFire command wrapped in APDU
  Serial.println("\nDemonstrating native DESFire SELECT_APPLICATION command wrapped in APDU:");
  responseLen = sizeof(responseBuffer);
  status = iso7816Wrapper.transmitCommand(
      DesfireCommand::DF_CMD_SELECT_APPLICATION, aid, 3, responseBuffer, responseLen);
  
  if (status == DesfireStatus::DFST_SUCCESS) {
    Serial.println("Application selected successfully using native command");
  } else {
    Serial.print("Failed to select application using native command. Status: ");
    Serial.println(static_cast<int>(status), HEX);
  }
} 