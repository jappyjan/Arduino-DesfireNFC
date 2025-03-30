/**
 * @file MultiFrameDataTransfer.ino
 * @brief Example sketch demonstrating multi-frame data transfer for large payloads
 *
 * This example demonstrates how to use the DesfireNFC library to perform
 * multi-frame data transfer operations with DESFire cards. It shows:
 * 1. Detecting a DESFire card
 * 2. Creating a standard data file with large size
 * 3. Writing a large amount of data that requires multi-frame transfers
 * 4. Reading back the data using multi-frame transfers
 * 5. Verifying the data integrity
 */

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_PN532.h>
#include "DesfireNFC.h"
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

// Test file parameters
#define TEST_FILE_NO 0x01
#define TEST_FILE_SIZE 1024  // 1KB file
#define TEST_WRITE_SIZE 512  // Write operation in chunks of 512 bytes

// Create PN532 instance
Adafruit_PN532 pn532(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);

// Create PN532Reader instance
PN532Reader reader(pn532);

// Create DesfireNFC instance
DesfireNFC nfc(reader);

// Buffer for card data
uint8_t dataBuffer[TEST_FILE_SIZE];

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  
  Serial.println(F("DESFire Multi-Frame Data Transfer Example"));
  
  // Initialize PN532
  Serial.println(F("Initializing PN532 reader..."));
  if (!reader.begin()) {
    Serial.println(F("Failed to initialize PN532!"));
    while (1) delay(10);
  }
  
  Serial.println(F("PN532 initialized."));
  Serial.println(F("Waiting for a DESFire card..."));
}

void loop() {
  // Check if a card is present
  if (nfc.detectCard()) {
    // Get card UID and print it
    uint8_t uid[10];
    uint8_t uidLength;
    
    if (nfc.getCardUID(uid, &uidLength)) {
      Serial.print(F("Card detected! UID: "));
      for (uint8_t i = 0; i < uidLength; i++) {
        Serial.print(uid[i] < 0x10 ? "0" : "");
        Serial.print(uid[i], HEX);
        Serial.print(" ");
      }
      Serial.println();
      
      // Select the master application (PICC level)
      uint8_t masterAid[3] = {0x00, 0x00, 0x00};
      if (nfc.selectApplication(masterAid) == DesfireStatus::DFST_SUCCESS) {
        Serial.println(F("Master application selected."));
        
        // Authenticate with default master key
        uint8_t defaultKey[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        
        if (nfc.authenticate(0, defaultKey, 16) == DesfireStatus::DFST_SUCCESS) {
          Serial.println(F("Authentication successful."));
          
          // Create a standard data file
          Serial.print(F("Creating standard data file ("));
          Serial.print(TEST_FILE_SIZE);
          Serial.println(F(" bytes)..."));
          
          // First delete the file if it exists
          nfc.deleteFile(TEST_FILE_NO);
          
          DesfireStatus status = nfc.createStdDataFile(
            TEST_FILE_NO, 
            DesfreCommunicationMode::DF_COMM_MODE_PLAIN, 
            0xEEEE,  // Access rights (all operations allowed)
            TEST_FILE_SIZE
          );
          
          if (status == DesfireStatus::DFST_SUCCESS) {
            Serial.println(F("File created successfully."));
            
            // Prepare test data (pattern with incrementing values)
            for (uint16_t i = 0; i < TEST_FILE_SIZE; i++) {
              dataBuffer[i] = i & 0xFF;
            }
            
            // Write data in multiple chunks to demonstrate multi-frame transfer
            Serial.println(F("Writing data with multi-frame transfer..."));
            
            status = nfc.writeData(TEST_FILE_NO, 0, TEST_WRITE_SIZE, dataBuffer);
            if (status == DesfireStatus::DFST_SUCCESS) {
              Serial.println(F("First chunk written successfully."));
              
              // Write the second chunk
              status = nfc.writeData(TEST_FILE_NO, TEST_WRITE_SIZE, 
                                    TEST_FILE_SIZE - TEST_WRITE_SIZE, 
                                    dataBuffer + TEST_WRITE_SIZE);
              
              if (status == DesfireStatus::DFST_SUCCESS) {
                Serial.println(F("Second chunk written successfully."));
                
                // Commit the transaction
                nfc.commitTransaction();
                
                // Clear the buffer for reading
                memset(dataBuffer, 0, TEST_FILE_SIZE);
                
                // Read back the data to verify
                Serial.println(F("Reading data with multi-frame transfer..."));
                uint32_t bytesRead = 0;
                
                status = nfc.readData(TEST_FILE_NO, 0, TEST_FILE_SIZE, dataBuffer, bytesRead);
                
                if (status == DesfireStatus::DFST_SUCCESS) {
                  Serial.print(F("Data read successfully. Bytes read: "));
                  Serial.println(bytesRead);
                  
                  // Verify the data
                  bool dataValid = true;
                  for (uint16_t i = 0; i < bytesRead; i++) {
                    if (dataBuffer[i] != (i & 0xFF)) {
                      dataValid = false;
                      Serial.print(F("Data mismatch at index "));
                      Serial.print(i);
                      Serial.print(F(": expected "));
                      Serial.print(i & 0xFF, HEX);
                      Serial.print(F(", got "));
                      Serial.println(dataBuffer[i], HEX);
                      break;
                    }
                  }
                  
                  if (dataValid) {
                    Serial.println(F("Data verification successful! Multi-frame transfer works!"));
                  } else {
                    Serial.println(F("Data verification failed!"));
                  }
                } else {
                  Serial.print(F("Failed to read data. Status: "));
                  Serial.println(static_cast<uint8_t>(status), HEX);
                }
              } else {
                Serial.print(F("Failed to write second chunk. Status: "));
                Serial.println(static_cast<uint8_t>(status), HEX);
              }
            } else {
              Serial.print(F("Failed to write first chunk. Status: "));
              Serial.println(static_cast<uint8_t>(status), HEX);
            }
          } else {
            Serial.print(F("Failed to create file. Status: "));
            Serial.println(static_cast<uint8_t>(status), HEX);
          }
        } else {
          Serial.println(F("Authentication failed."));
        }
      } else {
        Serial.println(F("Failed to select master application."));
      }
      
      // Wait before continuing
      Serial.println(F("\nRemove card to continue...\n"));
      while (nfc.detectCard()) {
        delay(100);
      }
      Serial.println(F("Card removed. Waiting for next card...\n"));
    }
  }
  
  delay(100);
} 