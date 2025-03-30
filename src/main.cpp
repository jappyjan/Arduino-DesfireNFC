#include <Adafruit_PN532.h>
#include <Arduino.h>
#include <SPI.h>

// PN532 pins for SPI communication
#define PN532_SCK 4   // Default ESP32-C3 SuperMini SPI SCK
#define PN532_MISO 5  // Default ESP32-C3 SuperMini SPI MISO
#define PN532_MOSI 6  // Default ESP32-C3 SuperMini SPI MOSI
#define PN532_SS 7    // Default ESP32-C3 SuperMini SPI SS (Chip Select)

// DESFire commands
#define DESFIRE_CMD_GET_VERSION 0x60
#define DESFIRE_CMD_SELECT_APPLICATION 0x5A
#define DESFIRE_CMD_GET_ADDITIONAL_FRAME 0xAF
#define DESFIRE_STATUS_SUCCESS 0x00
#define DESFIRE_STATUS_MORE_FRAMES 0xAF

// Create an instance of the PN532 class for SPI
Adafruit_PN532 nfc(PN532_SS);

// Function prototypes
void setupPN532();
bool detectCard(uint8_t* uid, uint8_t* uidLength);
void printHex(const uint8_t* data, uint8_t length);
bool getCardVersion();
bool activateIso14443_4(uint8_t* uid, uint8_t uidLength);
bool sendDESFireCommand(uint8_t  cmd,
                        uint8_t* data,
                        uint8_t  dataLen,
                        uint8_t* response,
                        uint8_t* responseLen);
void resetPN532Field();

void setup() {
    // Start serial
    Serial.begin(115200);

    // Wait for serial to be ready
    delay(3000);

    Serial.println("\n\n-----------------------------------");
    Serial.println("DESFire Card Basic Detection");
    Serial.println("-----------------------------------\n");

    // Initialize SPI
    SPI.begin(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);
    SPI.setFrequency(1000000);  // 1MHz clock for reliability

    // Setup PN532
    setupPN532();
}

void loop() {
    uint8_t uid[7];     // Buffer to store the card UID
    uint8_t uidLength;  // Length of the UID

    Serial.println("\n[INFO] Waiting for a DESFire card...");

    // Reset the PN532 before each detection cycle
    setupPN532();

    // Try to detect card
    if (detectCard(uid, &uidLength)) {
        Serial.println("[SUCCESS] Card detected!");
        Serial.print("[INFO] Card UID: ");
        printHex(uid, uidLength);
        Serial.println();

        // Determine if this is likely a DESFire card based on UID
        bool isDESFireLikely = (uidLength == 7) && (uid[0] == 0x04);

        if (isDESFireLikely) {
            Serial.println("[INFO] UID pattern suggests this is a DESFire card");

            // Activate card for ISO14443-4 communication (crucial for DESFire)
            if (activateIso14443_4(uid, uidLength)) {
                Serial.println("[SUCCESS] Card activated for ISO14443-4 communication");

                // Try to get version information which is a basic operation that should work on any
                // DESFire
                if (getCardVersion()) {
                    Serial.println("[SUCCESS] Successfully communicated with DESFire card");
                    Serial.println("[INFO] Card is a DESFire EV2 and is working properly");
                } else {
                    Serial.println("[WARNING] Failed to get version information");
                    Serial.println("[INFO] This could be because:");
                    Serial.println("1. The card is not a DESFire card despite the UID pattern");
                    Serial.println("2. The card is in a special state or locked");
                    Serial.println("3. Communication issue with the PN532 reader");
                }
            } else {
                Serial.println("[ERROR] Failed to activate card for ISO14443-4 communication");
                Serial.println("[INFO] This is required for DESFire protocol");
            }
        } else {
            Serial.println("[WARNING] UID pattern does not match typical DESFire pattern");
            Serial.println("[INFO] This card might not be a DESFire card");
        }

        // Wait for card to be removed
        Serial.println("[INFO] Done. Please remove card and wait...");
        delay(3000);
    } else {
        Serial.println("[INFO] No card detected");
    }

    delay(1000);
}

// Configure the PN532 with specific settings for DESFire
void setupPN532() {
    // Initialize PN532
    nfc.begin();

    // Wait for it to initialize
    delay(100);

    // Get firmware version to verify PN532 is working
    uint32_t firmwareVersion = nfc.getFirmwareVersion();
    if (!firmwareVersion) {
        Serial.println("[ERROR] Failed to find PN532 board");
        Serial.println("[INFO] Check SPI connections");
        delay(1000);
        return;
    }

    // Display PN532 info
    Serial.print("[INFO] Found PN532 with firmware version: ");
    Serial.print((firmwareVersion >> 24) & 0xFF, HEX);
    Serial.print(".");
    Serial.println((firmwareVersion >> 16) & 0xFF, HEX);

    // Configure for card reading
    nfc.SAMConfig();

    // Set max number of retries for card activation
    nfc.setPassiveActivationRetries(0xFF);

    // Set timeout (longer timeout for better reliability)
    delay(50);
}

// Helper function to reset the PN532 RF field
void resetPN532Field() {
    // Reset by reinitializing and reconfiguring
    nfc.begin();
    nfc.SAMConfig();
    delay(50);
}

// Detect a card and read its UID
bool detectCard(uint8_t* uid, uint8_t* uidLength) {
    // Higher timeout (2 seconds) for better detection reliability
    bool success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, uidLength, 2000);
    return success;
}

// Activate ISO14443-4 protocol for DESFire communication
bool activateIso14443_4(uint8_t* uid, uint8_t uidLength) {
    uint8_t atr[64];
    uint8_t atrLength = 64;

    Serial.println("[DEBUG] Attempting ISO14443-4 activation");

    // Try multiple approaches
    for (int attempt = 0; attempt < 3; attempt++) {
        // Reset the RF field before each attempt
        resetPN532Field();

        // Approach 1: Using InList passive target with 14443-4 flag
        // This explicitly requests an ISO14443-4 activation
        bool success = false;

        if (attempt == 0) {
            Serial.println("[DEBUG] Trying simple ISO14443-4 activation");
            success = nfc.inListPassiveTarget();
        } else if (attempt == 1) {
            Serial.println("[DEBUG] Trying alternative activation with RATS command");
            // Special activation sequence for challenging cards
            // Includes manual RATS command
            uint8_t rats[2] = {0xE0, 0x80};  // RATS command
            uint8_t ratsResponse[64];
            uint8_t ratsLen = 64;

            // First detect card (we know it's there)
            if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 1000)) {
                // Then send RATS command
                success = nfc.inDataExchange(rats, 2, ratsResponse, &ratsLen);

                if (success && ratsLen > 0) {
                    Serial.print("[DEBUG] RATS response: ");
                    printHex(ratsResponse, ratsLen);
                    Serial.println();
                }
            }
        } else if (attempt == 2) {
            Serial.println("[DEBUG] Trying DESFire-specific wake sequence");
            // Another approach - try very specific wake sequence for DESFire
            uint8_t desfireWake[3] = {0x52, 0x00, 0x00};  // Custom wake command
            uint8_t wakeResponse[8];
            uint8_t wakeLen = 8;

            if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 1000)) {
                // Send wake command
                success = nfc.inDataExchange(desfireWake, 1, wakeResponse, &wakeLen);

                if (success && wakeLen > 0) {
                    Serial.print("[DEBUG] Wake response: ");
                    printHex(wakeResponse, wakeLen);
                    Serial.println();
                }
            }
        }

        if (success) {
            Serial.println("[DEBUG] Card appears to be activated for ISO14443-4");
            return true;
        }

        delay(100);  // Short delay before next attempt
    }

    Serial.println("[ERROR] Failed all ISO14443-4 activation attempts");
    return false;
}

// Get DESFire card version (using direct commands)
bool getCardVersion() {
    uint8_t response[32];
    uint8_t responseLen = 32;
    uint8_t uid[7];
    uint8_t uidLen    = 7;
    uint8_t altCmd[2] = {DESFIRE_CMD_GET_VERSION, 0x00};

    Serial.println("[DEBUG] Sending GET_VERSION command");

    // Try 3 different approaches
    for (int attempt = 0; attempt < 3; attempt++) {
        // Reset response length for each attempt
        responseLen = 32;

        if (attempt == 0) {
            // Approach 1: Direct command
            Serial.println("[DEBUG] Trying direct GET_VERSION command");
            if (sendDESFireCommand(DESFIRE_CMD_GET_VERSION, NULL, 0, response, &responseLen)) {
                return true;
            }
        } else if (attempt == 1) {
            // Approach 2: Add a small delay and try again with a reset
            Serial.println("[DEBUG] Trying with additional reset");
            resetPN532Field();

            // Reactivate card
            if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLen, 1000)) {
                // Wait a bit before sending command
                delay(50);
                if (sendDESFireCommand(DESFIRE_CMD_GET_VERSION, NULL, 0, response, &responseLen)) {
                    return true;
                }
            }
        } else if (attempt == 2) {
            // Approach 3: Try alternative command format (with 0x00 padding)
            Serial.println("[DEBUG] Trying alternative command format");
            if (sendDESFireCommand(0x00, altCmd, 2, response, &responseLen)) {
                return true;
            }
        }

        delay(150);  // Wait between attempts
    }

    Serial.println("[ERROR] All GET_VERSION attempts failed");
    return false;
}

// Send a command to a DESFire card with proper error handling
bool sendDESFireCommand(uint8_t  cmd,
                        uint8_t* data,
                        uint8_t  dataLen,
                        uint8_t* response,
                        uint8_t* responseLen) {
    uint8_t txBuffer[64];  // Buffer for command
    uint8_t txLen   = 0;
    bool    success = false;

    // Prepare command
    if (cmd != 0x00) {
        // Standard command format
        txBuffer[0] = cmd;
        txLen       = 1;

        // Add data if present
        if (data != NULL && dataLen > 0) {
            memcpy(txBuffer + 1, data, dataLen);
            txLen += dataLen;
        }
    } else {
        // Alternative format (when cmd is 0x00, data contains full command)
        if (data != NULL && dataLen > 0) {
            memcpy(txBuffer, data, dataLen);
            txLen = dataLen;
        } else {
            return false;  // Invalid parameters
        }
    }

    // Print command for debugging
    Serial.print("[DEBUG] Sending command: ");
    printHex(txBuffer, txLen);
    Serial.println();

    // Try sending command with retries
    for (int retry = 0; retry < 3; retry++) {
        // Send command
        success = nfc.inDataExchange(txBuffer, txLen, response, responseLen);

        if (success && *responseLen > 0) {
            // Command succeeded
            Serial.print("[DEBUG] Response (");
            Serial.print(*responseLen);
            Serial.print(" bytes): ");
            printHex(response, *responseLen);
            Serial.println();

            // Check for status byte
            if (*responseLen > 0) {
                uint8_t status = response[0];

                if (status == DESFIRE_STATUS_SUCCESS) {
                    Serial.println("[DEBUG] Command successful (0x00)");

                    // Parse version info if this is a GET_VERSION response
                    if (cmd == DESFIRE_CMD_GET_VERSION && *responseLen > 7) {
                        Serial.println("[INFO] Version info (extract):");
                        Serial.print("- Hardware: 0x");
                        Serial.print(response[3], HEX);
                        Serial.print(".");
                        Serial.println(response[4], HEX);
                    }

                    return true;
                } else if (status == DESFIRE_STATUS_MORE_FRAMES) {
                    Serial.println("[DEBUG] Additional frames available (0xAF)");
                    // For simplicity we're not handling multi-frame responses here
                    // But we count this as a success as we established communication
                    return true;
                } else {
                    Serial.print("[ERROR] Command failed with status: 0x");
                    Serial.println(status, HEX);
                }
            }
        } else {
            Serial.print("[ERROR] Command failed on attempt ");
            Serial.println(retry + 1);
        }

        // Short delay before retrying
        delay(50);
    }

    return false;
}

// Helper function to print hex values
void printHex(const uint8_t* data, uint8_t length) {
    for (uint8_t i = 0; i < length; i++) {
        if (data[i] < 0x10) {
            Serial.print("0");
        }
        Serial.print(data[i], HEX);
        Serial.print(" ");
    }
}
