/**
 * @file CardInformationRetrieval.ino
 * @brief Comprehensive DESFire Card Information Retrieval Example
 * 
 * This example demonstrates how to retrieve and display comprehensive
 * information from a MIFARE DESFire card using the DesfireNFC library.
 * 
 * Information retrieved:
 * - Card UID
 * - Card version details (hardware/software info, storage size, etc.)
 * - Free memory available
 * - Applications present on the card
 * - Key settings and security configuration
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
  
  Serial.println("DESFire Card Information Retrieval Example");
  Serial.println("------------------------------------------");
  
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
    Serial.println("\n=== DESFire Card Detected! ===");
    
    // Get the card information
    retrieveCardInfo();
    
    // Wait before trying to detect another card
    Serial.println("\nRemove card and place a new one to read information again...");
    delay(3000);  // Debounce to prevent repeated detection of the same card
  }
  
  delay(100);  // Short delay between detection attempts
}

/**
 * Retrieves and displays comprehensive card information
 */
void retrieveCardInfo() {
  // 1. Read and display the card's UID
  displayCardUID();
  
  // 2. Get and display card version information
  displayCardVersion();
  
  // 3. Get and display available memory
  displayFreeMemory();
  
  // 4. Select the PICC level and get application list
  displayApplications();
  
  // 5. Get key settings
  displayKeySettings();
}

/**
 * Display card UID
 */
void displayCardUID() {
  uint8_t uid[7];  // DESFire UIDs are 7 bytes
  uint8_t uidLength;
  
  Serial.println("\n--- Card Identifier ---");
  
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
}

/**
 * Display card version information
 */
void displayCardVersion() {
  DESFireCardVersion version;
  
  Serial.println("\n--- Card Version Information ---");
  
  if (desfire.getVersion(&version)) {
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
  } else {
    Serial.println("Failed to read card version information!");
  }
}

/**
 * Display free memory available on the card
 */
void displayFreeMemory() {
  uint32_t freeMemory = 0;
  
  Serial.println("\n--- Free Memory Information ---");
  
  DesfireStatus status = desfire.getFreeMem(&freeMemory);
  if (status == DesfireStatus::DFST_SUCCESS) {
    Serial.print("Free Memory: ");
    Serial.print(freeMemory);
    Serial.println(" bytes");
  } else {
    Serial.print("Failed to get free memory information. Status: 0x");
    Serial.println(static_cast<uint8_t>(status), HEX);
  }
}

/**
 * Display applications present on the card
 */
void displayApplications() {
  // First select PICC level (root)
  uint8_t root_aid[3] = {0x00, 0x00, 0x00};
  
  Serial.println("\n--- Application Information ---");
  
  DesfireStatus status = desfire.selectApplication(root_aid);
  if (status != DesfireStatus::DFST_SUCCESS) {
    Serial.print("Failed to select PICC level. Status: 0x");
    Serial.println(static_cast<uint8_t>(status), HEX);
    return;
  }
  
  Serial.println("Selected PICC level (root)");
  
  // Get application IDs
  uint8_t appIds[48];  // Buffer for up to 16 application IDs (3 bytes each)
  uint8_t count = 0;
  
  status = desfire.getApplicationIDs(appIds, 16, count);
  if (status == DesfireStatus::DFST_SUCCESS) {
    Serial.print("Number of applications on card: ");
    Serial.println(count);
    
    if (count == 0) {
      Serial.println("No applications found.");
    } else {
      Serial.println("Application IDs:");
      // Display each application ID
      for (uint8_t i = 0; i < count; i++) {
        Serial.print("  App ");
        Serial.print(i + 1);
        Serial.print(": 0x");
        for (uint8_t j = 0; j < 3; j++) {
          if (appIds[i * 3 + j] < 0x10) Serial.print("0");
          Serial.print(appIds[i * 3 + j], HEX);
        }
        Serial.println();
      }
    }
  } else {
    Serial.print("Failed to get application IDs. Status: 0x");
    Serial.println(static_cast<uint8_t>(status), HEX);
  }
}

/**
 * Display key settings for the current application
 */
void displayKeySettings() {
  uint8_t keySettings = 0;
  uint8_t maxKeyNo = 0;
  
  Serial.println("\n--- Key Settings Information ---");
  
  DesfireStatus status = desfire.getKeySettings(keySettings, maxKeyNo);
  if (status == DesfireStatus::DFST_SUCCESS) {
    Serial.print("Key Settings: 0x");
    Serial.println(keySettings, HEX);
    
    // Interpret key settings
    Serial.println("Security Configuration:");
    
    // Application configuration
    if (keySettings & 0x01) {
      Serial.println("- Application master key authentication needed for create/delete files");
    } else {
      Serial.println("- No authentication needed for create/delete files");
    }
    
    if (keySettings & 0x02) {
      Serial.println("- Directory listing without authentication is allowed");
    } else {
      Serial.println("- Authentication required for directory listing");
    }
    
    if (keySettings & 0x04) {
      Serial.println("- Application master key can be changed without auth with master key");
    } else {
      Serial.println("- Master key authentication required to change master key");
    }
    
    // Get key configuration
    uint8_t keyType = (keySettings >> 6) & 0x03;
    Serial.print("- Key Type: ");
    switch (keyType) {
      case 0:
        Serial.println("DES/3DES");
        break;
      case 1:
        Serial.println("3K3DES");
        break;
      case 2:
        Serial.println("AES");
        break;
      default:
        Serial.println("Unknown");
        break;
    }
    
    Serial.print("Maximum number of keys: ");
    Serial.println(maxKeyNo);
  } else {
    Serial.print("Failed to get key settings. Status: 0x");
    Serial.println(static_cast<uint8_t>(status), HEX);
    
    if (status == DesfireStatus::DFST_AUTHENTICATION_ERROR) {
      Serial.println("Authentication required to read key settings.");
      Serial.println("Note: Some cards require authentication to read key settings.");
    }
  }
} 