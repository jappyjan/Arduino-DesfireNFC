/**
 * @file ErrorHandlingExample.ino
 * @brief Example demonstrating error handling in the DesfireNFC library
 * 
 * This example shows how to handle and interpret DESFire error codes
 * using the DesfireErrorHandler class.
 */

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include "DesfireNFC.h"
#include "DesfireErrorHandler.h"

// PN532 connections
#define PN532_SCK  (18)
#define PN532_MOSI (23)
#define PN532_SS   (5)
#define PN532_MISO (19)

// Create an instance of the PN532 reader (implementation would be in your actual code)
// This is a placeholder - you would need to modify this based on your actual reader implementation
PN532Reader reader(PN532_SS);

// Create an instance of the DesfireNFC class
DesfireNFC nfc(reader);

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  
  Serial.println(F("DESFire NFC Error Handling Example"));
  Serial.println(F("----------------------------------"));
  
  // Initialize SPI
  SPI.begin();
  
  // Initialize the PN532 reader
  if (!nfc.initialize()) {
    Serial.println(F("Failed to initialize NFC reader!"));
    while (1) delay(10);
  }
  
  Serial.println(F("NFC reader initialized"));
  Serial.println(F("Please place a DESFire card on the reader..."));
}

void loop() {
  // Reset variables
  static unsigned long lastDetectionTime = 0;
  static bool cardWasDetected = false;
  
  // Try to detect a card
  bool cardDetected = nfc.detectCard();
  
  // Handle card detection
  if (cardDetected && !cardWasDetected) {
    // Card was just detected
    lastDetectionTime = millis();
    cardWasDetected = true;
    Serial.println(F("\nCard detected!"));
    
    // Example 1: Get the card's UID
    uint8_t uid[10];
    uint8_t uidLength = 0;
    
    if (nfc.getCardUID(uid, &uidLength)) {
      Serial.print(F("Card UID: "));
      for (uint8_t i = 0; i < uidLength; i++) {
        Serial.print(uid[i] < 0x10 ? " 0" : " ");
        Serial.print(uid[i], HEX);
      }
      Serial.println();
    } else {
      Serial.println(F("Failed to read card UID"));
    }
    
    // Example 2: Try to select a non-existent application (should generate an error)
    uint8_t nonExistentAID[3] = {0xFF, 0xFF, 0xFF};
    DesfireStatus status = nfc.selectApplication(nonExistentAID);
    
    // Use error handler to print a detailed report
    DesfireErrorHandler::printErrorReport(status, "Select non-existent application");
    
    // Example 3: Try to authenticate with the wrong key (should generate an error)
    uint8_t wrongKey[16] = {0}; // All zeros key that probably won't work
    status = nfc.authenticate(0, wrongKey, sizeof(wrongKey));
    
    // Use error handler to print a detailed report
    DesfireErrorHandler::printErrorReport(status, "Authenticate with wrong key");
    
    // Example 4: Demonstrate error categories
    Serial.println(F("\nDemonstrating error categories:"));
    
    // Define some example error codes
    DesfireStatus authError = DesfireStatus::DFST_AUTHENTICATION_ERROR;
    DesfireStatus fileError = DesfireStatus::DFST_FILE_NOT_FOUND;
    DesfireStatus appError = DesfireStatus::DFST_APPLICATION_NOT_FOUND;
    DesfireStatus commError = DesfireStatus::DFST_COMMUNICATION_ERROR;
    
    // Print information about each error
    Serial.print(F("Auth error (0x"));
    Serial.print(static_cast<uint16_t>(authError), HEX);
    Serial.print(F("): Category = "));
    Serial.print(DesfireErrorHandler::getErrorCategory(authError));
    Serial.print(F(", Message = "));
    Serial.println(DesfireErrorHandler::getErrorMessage(authError));
    
    Serial.print(F("File error (0x"));
    Serial.print(static_cast<uint16_t>(fileError), HEX);
    Serial.print(F("): Category = "));
    Serial.print(DesfireErrorHandler::getErrorCategory(fileError));
    Serial.print(F(", Message = "));
    Serial.println(DesfireErrorHandler::getErrorMessage(fileError));
    
    Serial.print(F("App error (0x"));
    Serial.print(static_cast<uint16_t>(appError), HEX);
    Serial.print(F("): Category = "));
    Serial.print(DesfireErrorHandler::getErrorCategory(appError));
    Serial.print(F(", Message = "));
    Serial.println(DesfireErrorHandler::getErrorMessage(appError));
    
    Serial.print(F("Comm error (0x"));
    Serial.print(static_cast<uint16_t>(commError), HEX);
    Serial.print(F("): Category = "));
    Serial.print(DesfireErrorHandler::getErrorCategory(commError));
    Serial.print(F(", Message = "));
    Serial.println(DesfireErrorHandler::getErrorMessage(commError));
    
    Serial.println(F("\nRemove the card and place it again to restart the demo"));
  }
  else if (!cardDetected && cardWasDetected) {
    // Card was removed
    cardWasDetected = false;
    Serial.println(F("Card removed"));
  }
  
  // Small delay to prevent tight loop
  delay(100);
} 