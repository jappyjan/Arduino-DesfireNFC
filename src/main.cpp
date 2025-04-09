#include <Wire.h>
#include <SPI.h>
// #include <Adafruit_PN532.h> // Removed
#include "Desfire.h" // Use the custom library header
#include "Utils.h" // Include Utils for PrintHexBuf maybe?

// --- Configuration ---
// Define PN532 pins depending on connection type

// SPI Connection (Using custom library's SPI)
#define PN532_SCK (4)   // Default ESP32-C3 SuperMini SPI SCK
#define PN532_MISO (5)  // Default ESP32-C3 SuperMini SPI MISO
#define PN532_MOSI (6)  // Default ESP32-C3 SuperMini SPI MOSI
#define PN532_SS (7)    // Default ESP32-C3 SuperMini SPI SS (Chip Select)
// Define Reset Pin if connected, otherwise use -1 or an unused pin
#define PN532_RESET 0xFF // Use 0xFF for potentially unused byte pin

// --- DESFire Example Parameters (Using custom library types) ---
// Default 2K3DES key (16 zero bytes)
const byte DEFAULT_16_ZERO_BYTES[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// Application ID (AID) - Use uint32_t as per Desfire.h
const uint32_t APP_AID = 0x090909; // Use 0x notation for clarity
// PICC Master Application AID
const uint32_t MASTER_AID = 0x000000;

// File ID
const byte FILE_ID = 0x01;

// Key Settings for new application (Using DESFireKeySettings enum)
// Their KS_FACTORY_DEFAULT is 0x0F, matches our previous value.
const DESFireKeySettings APP_KEY_SETTINGS = KS_FACTORY_DEFAULT;

// Number of keys and key type for CreateApplication
// Desfire::CreateApplication takes u8_KeyCount and DESFireKeyType e_KeyType
const byte APP_NUM_KEYS = 5;
// const DESFireKeyType APP_KEY_TYPE = DF_KEY_AES; // Keep App keys as AES for now
const DESFireKeyType APP_KEY_TYPE = DF_KEY_2K3DES; // Use 2K3DES for App keys

// File Settings:
const DESFireFileEncryption FILE_COMM_SETTINGS = CM_PLAIN; // Plain communication
// Access Rights (Using DESFireFilePermissions struct and DESFireAccessRights enum)
DESFireFilePermissions FILE_PERMISSIONS;
// FILE_PERMISSIONS.e_ReadAccess = AR_FREE; // E = Everyone
// FILE_PERMISSIONS.e_WriteAccess = AR_KEY3; // 3 = Key 3
// FILE_PERMISSIONS.e_ReadAndWriteAccess = AR_KEY3; // 3 = Key 3
// FILE_PERMISSIONS.e_ChangeAccess = AR_KEY0; // 0 = Master Key (Key 0)
// Let's initialize this in setup()

// File Size
const uint32_t FILE_SIZE = 0x40; // 64 bytes

// --- Global Objects ---
Desfire desfire; // Instantiate the custom Desfire class (which includes PN532)

// --- Key Objects ---
DES masterKey; // PICC Master Key (Key 0) - Use 2K3DES (16 zero bytes)
DES appKey0;   // App Master Key (Key 0) - Use 2K3DES
DES appKey3;   // App Key 3 (for Write/RW access) - Use 2K3DES

// --- Arduino Setup ---
void setup() {
  Serial.begin(115200);
  while (!Serial);
  delay(500); // Short delay after serial init
  Serial.println("\nESP32 DESFire EV1 Example (Using Custom Lib)");

  // Initialize File Permissions struct
  FILE_PERMISSIONS.e_ReadAccess = AR_FREE; // E = Everyone
  FILE_PERMISSIONS.e_WriteAccess = AR_KEY3; // 3 = Key 3
  FILE_PERMISSIONS.e_ReadAndWriteAccess = AR_KEY3; // 3 = Key 3
  FILE_PERMISSIONS.e_ChangeAccess = AR_KEY0; // 0 = Master Key (Key 0)

  // Initialize Key Objects
  masterKey.SetKeyData(DEFAULT_16_ZERO_BYTES, 16, 0); // Initialize as 2K3DES (16 bytes)
  appKey0.SetKeyData(DEFAULT_16_ZERO_BYTES, 16, 0); 
  appKey3.SetKeyData(DEFAULT_16_ZERO_BYTES, 16, 0); 

  // --- Explicitly Initialize SPI Pins FIRST ---
  Serial.println("Initializing SPI pins...");
  SPI.begin(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);
  // Optionally set SPI clock, although library might handle this
  // SPI.setFrequency(1000000); // e.g., 1 MHz like in PN532.h
  // SPI.setDataMode(SPI_MODE0); // PN532 uses Mode 0

  // Initialize PN532 via Desfire object (using SPI)
  Serial.println("Initializing PN532 library (Hardware SPI)...");
  desfire.InitHardwareSPI(PN532_SS, PN532_RESET);
  desfire.begin(); // Call begin after initialization

  // Set debug level (optional, check Desfire.cpp/PN532.cpp for levels)
  // desfire.SetDebugLevel(1); // Example: Set debug level to 1
  Serial.println("Enabling Desfire library debug level 2...");
  desfire.SetDebugLevel(2); // Max debug level

  Serial.println("Checking PN532 connection...");
  byte icType, verHi, verLo, flags;
  if (!desfire.GetFirmwareVersion(&icType, &verHi, &verLo, &flags)) {
      Serial.println("Didn't find PN53x board! Halting.");
      while (1); // halt
  }
  Serial.print("Found Chip PN5"); Serial.println(icType, HEX);
  Serial.print("Firmware ver. "); Serial.print(verHi, DEC);
  Serial.print('.'); Serial.println(verLo, DEC);

  // Configure board to operate in ISO14443-A mode
  Serial.println("Configuring SAM...");
  if (!desfire.SamConfig()) {
       Serial.println("SAM configuration failed! Halting.");
       while(1); // halt
  }
  Serial.println("SAM configured.");

}

// --- Arduino Loop ---
void loop() {
  Serial.println("\nWaiting for Desfire card...");

  byte uid[7]; // Buffer for card UID
  byte uidLength = 0;
  eCardType cardType;

  // Try to read a card
  // This function now returns card type and UID length
  if (desfire.ReadPassiveTargetID(uid, &uidLength, &cardType)) {
    Serial.print("Found card: ");
    // Use Utils::PrintHexBuf if available, otherwise manual print
    // Assuming Utils provides PrintHexBuf based on Desfire.cpp usage
    Utils::PrintHexBuf(uid, uidLength, LF);
    Serial.printf("  UID Length: %d\n", uidLength);
    Serial.printf("  Card Type: %d (0=Unk, 1=DESFire, 3=DESFireRandom)\n", cardType);

    // Check if it's a DESFire card
    if (cardType == CARD_Desfire || cardType == CARD_DesRandom) {
        Serial.println("DESFire card detected! Starting operations...");
        bool success = true;

        // 1. Select PICC-level (Master File Directory)
        Serial.println("\n[1] Selecting Master Application (AID 000000)...");
        // NOTE: SelectApplication now takes uint32_t
        if (success) success = desfire.SelectApplication(MASTER_AID);

        // 2. Authenticate with PICC Master Key
        Serial.println("\n[2] Authenticating with PICC Master Key (Key 0)...");
        // NOTE: Authenticate now takes DESFireKey*
        if (success) success = desfire.Authenticate(0x00, &masterKey);

        // 3. Format Card (Requires PICC Master Key Auth)
        Serial.println("\n[3] Formatting PICC (Erases everything!)...");
        // Comment out if not desired
        if (success) success = desfire.FormatCard();

        // Re-authenticate with PICC Master Key after format
        Serial.println("\n[3b] Re-Authenticating with PICC Master Key (Key 0) after format...");
        if (success) success = desfire.Authenticate(0x00, &masterKey);

        // 4. Create Application (Requires PICC Master Key Auth)
        Serial.println("\n[4] Creating Application AID 090909...");
        // NOTE: Parameters use enums and uint32_t
        if (success) success = desfire.CreateApplication(APP_AID, APP_KEY_SETTINGS, APP_NUM_KEYS, APP_KEY_TYPE);

        // 5. Select the new Application
        Serial.println("\n[5] Selecting new Application (AID 090909)...");
        if (success) success = desfire.SelectApplication(APP_AID);

        // 6. Authenticate with new App Master Key (Key 0)
        Serial.println("\n[6] Authenticating with new App Master Key (Key 0)...");
        if (success) success = desfire.Authenticate(0x00, &appKey0);

        // 7. Create Standard Data File (Requires App Master Key 0 Auth)
        Serial.println("\n[6b] Creating Standard Data File (ID 01)...");
        // NOTE: CreateStdDataFile takes DESFireFilePermissions*
        if (success) success = desfire.CreateStdDataFile(FILE_ID, &FILE_PERMISSIONS, FILE_SIZE);

        // 8. Authenticate with Key 0x03 for writing (Write access = AR_KEY3)
        Serial.println("\n[7] Authenticating with App Key 0x03 (for write access)...");
        if (success) success = desfire.Authenticate(0x03, &appKey3);

        // 9. Write Data to the file
        Serial.println("\n[8] Writing data to file...");
        byte write_data[] = {0x1a, 0x2b, 0x3c, 0x4d, 0x5e};
        // NOTE: WriteFileData takes length as int
        if (success) success = desfire.WriteFileData(FILE_ID, 0, sizeof(write_data), write_data);

        // 10. Read Data from the file (Read access = AR_FREE, but auth still needed)
        Serial.println("\n[9] Reading data from file...");
        byte read_buffer[64];
        // NOTE: ReadFileData takes length as int, returns bool
        int bytes_read = 0; // Need to track length manually if needed, func returns bool
        if (success) {
            // Although Read is FREE, write requires Key 3, so stay authenticated with Key 3.
            // If we needed a different key for reading, we'd re-authenticate here.
            // We are already authenticated with Key 3 from the write step.

            // Clear buffer before reading
            memset(read_buffer, 0, sizeof(read_buffer));

            success = desfire.ReadFileData(FILE_ID, 0, sizeof(write_data), read_buffer); // Read back same amount we wrote
            if (success) {
                bytes_read = sizeof(write_data); // Assume success means we read the requested amount
                Serial.print("Read Data (" + String(bytes_read) + " bytes): ");
                Utils::PrintHexBuf(read_buffer, bytes_read, LF);
                // Verify data
                if (memcmp(write_data, read_buffer, bytes_read) == 0) {
                    Serial.println("Data verification successful!");
                } else {
                    Serial.println("Data verification FAILED!");
                    success = false;
                }
            } else {
                 Serial.println("ReadFileData failed!");
            }
        }

        // --- End of Operations ---
        if (success) {
          Serial.println("\nDESFire operations completed successfully!");
        } else {
          Serial.println("\nDESFire operations failed at some point.");
        }

        // Release the card (optional, PN532 might do this automatically on next read attempt)
        // desfire.ReleaseCard();
        Serial.println("Card operations finished. Waiting 5 seconds...");
        delay(5000); // Wait before trying again

    } else {
      Serial.println("Card is not a DESFire card.");
      delay(1000);
    }
  } else {
    // No card found, wait a bit
    delay(500);
  }
} 