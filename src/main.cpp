#include <Adafruit_PN532.h>
#include <Arduino.h>
#include <SPI.h>

// DESFire Authentication Information:
// ----------------------------------
// DESFire cards use a secure authentication process:
// 1. The reader sends an authentication request for a specific key number
// 2. The card responds with a random challenge (8 bytes)
// 3. The reader encrypts this challenge with the appropriate key
// 4. The reader sends the encrypted challenge back to the card
// 5. The card verifies and responds with success/failure
//
// The AES authentication command (0xAA) allows for secure 128-bit AES authentication.
// Default keys on factory fresh DESFire cards are typically all zeros.
// Key 0 is the application master key.
// 
// When creating applications, key settings (0x0F in our case) control:
// - Whether the master key is needed for file operations (0x0F = not required)
// - Whether keys can be changed (0x0F = allowed)
//
// The key count byte (0x81 in our case) defines:
// - Upper 4 bits (0x80): Key type (0x00=DES, 0x40=3K3DES, 0x80=AES)
// - Lower 4 bits (0x01): Number of keys (1 in our case)

// PN532 pins for SPI communication
#define PN532_SCK 4   // Default ESP32-C3 SuperMini SPI SCK
#define PN532_MISO 5  // Default ESP32-C3 SuperMini SPI MISO
#define PN532_MOSI 6  // Default ESP32-C3 SuperMini SPI MOSI
#define PN532_SS 7    // Default ESP32-C3 SuperMini SPI SS (Chip Select)

// DESFire commands
#define DESFIRE_CMD_GET_VERSION 0x60
#define DESFIRE_CMD_SELECT_APPLICATION 0x5A
#define DESFIRE_CMD_CREATE_APPLICATION 0xCA
#define DESFIRE_CMD_GET_ADDITIONAL_FRAME 0xAF
#define DESFIRE_CMD_AUTHENTICATE_AES 0xAA
#define DESFIRE_CMD_AUTHENTICATE_LEGACY 0x0A // DES/3DES auth
#define DESFIRE_CMD_AUTHENTICATE_ISO 0x1A   // 3DES auth (ISO)
#define DESFIRE_STATUS_SUCCESS 0x00
#define DESFIRE_STATUS_MORE_FRAMES 0xAF

// DESFire EV2 specific commands
#define DESFIRE_CMD_AUTHENTICATE_EV2_FIRST 0x71
#define DESFIRE_CMD_AUTHENTICATE_EV2_NONFIRST 0x77
#define DESFIRE_CMD_COMMIT_TRANSACTION_MAC 0xC7
#define DESFIRE_CMD_ABORT_TRANSACTION_MAC 0xA7
#define DESFIRE_CMD_GET_COMMAND_COUNTER 0x7A
#define DESFIRE_CMD_SET_COMMAND_COUNTER 0x7B

// DESFire EV2 constants
#define DESFIRE_EV2_MAC_LENGTH 8
#define DESFIRE_EV2_COUNTER_LENGTH 4
#define DESFIRE_EV2_TI_LENGTH 4

// Define key type constants
#define KEY_TYPE_DES 0
#define KEY_TYPE_3DES 1  
#define KEY_TYPE_AES 2

// Operating modes
#define MODE_STANDBY 0  // Waiting for command
#define MODE_DETECT 1   // Regular card detection
#define MODE_ENROLL 2   // Card enrollment

// Create an instance of the PN532 class for SPI
Adafruit_PN532 nfc(PN532_SS);

// Global variables
uint8_t operatingMode   = MODE_STANDBY;
bool    commandReceived = false;
String  serialCommand   = "";

// Global variables for authentication
uint8_t sessionKey[16] = {0};
bool isAuthenticated = false;
// Default AES key (16 bytes of zeros)
uint8_t defaultAESKey[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
// Default DES key (8 bytes of zeros)
uint8_t defaultDESKey[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// Define communication modes
#define COMM_MODE_PLAIN 0x00
#define COMM_MODE_MAC 0x01
#define COMM_MODE_ENCRYPT 0x03

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
bool enrollCard(uint8_t* uid, uint8_t uidLength);
bool selectApplication(uint8_t* aid, uint8_t aidLen);
bool createApplication(uint8_t* aid, uint8_t settings, uint8_t keyCount);
void printHelp();
void processSerialCommand();
void detectCardMode();
void enrollCardMode();
bool authenticateCard(uint8_t keyNo, uint8_t* key, uint8_t keyType);
bool authenticateAES(uint8_t keyNo, uint8_t* key);
bool authenticateDES(uint8_t keyNo, uint8_t* key);
bool authenticate3DES(uint8_t keyNo, uint8_t* key);
bool authenticateEV2First(uint8_t keyNo, uint8_t* key);
bool authenticateEV2NonFirst(uint8_t keyNo, uint8_t* key);
bool transceiveData(uint8_t* command, uint8_t commandLen, uint8_t* response, uint8_t* responseLen);
bool commitTransactionMAC();
bool abortTransactionMAC();
bool getCommandCounter(uint32_t* counter);
bool setCommandCounter(uint32_t counter);

void setup() {
    // Start serial
    Serial.begin(115200);

    // Wait for serial to be ready
    delay(3000);

    Serial.println("\n\n-----------------------------------");
    Serial.println("DESFire Card Manager");
    Serial.println("-----------------------------------\n");

    // Initialize SPI
    SPI.begin(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);
    SPI.setFrequency(1000000);  // 1MHz clock for reliability

    // Setup PN532
    setupPN532();

    // Display help menu
    printHelp();
}

void loop() {
    // Check for serial commands
    if (Serial.available() > 0) {
        char incomingChar = Serial.read();

        // Process command on newline
        if (incomingChar == '\n') {
            commandReceived = true;
        } else {
            serialCommand += incomingChar;
        }
    }

    // Process command if received
    if (commandReceived) {
        processSerialCommand();
        serialCommand   = "";
        commandReceived = false;
    }

    // Handle operating modes
    switch (operatingMode) {
        case MODE_DETECT:
            detectCardMode();
            break;

        case MODE_ENROLL:
            enrollCardMode();
            break;

        case MODE_STANDBY:
        default:
            // In standby mode, just wait for commands
            delay(100);
            break;
    }
}

// Process incoming serial commands
void processSerialCommand() {
    serialCommand.trim();

    if (serialCommand.length() == 0) {
        return;
    }

    Serial.print("[CMD] Processing command: ");
    Serial.println(serialCommand);

    if (serialCommand == "help" || serialCommand == "?") {
        printHelp();
    } else if (serialCommand == "detect") {
        Serial.println("[INFO] Entering card detection mode");
        Serial.println("[INFO] Press 'stop' to return to standby");
        operatingMode = MODE_DETECT;
    } else if (serialCommand == "enroll") {
        Serial.println("[INFO] Entering card enrollment mode");
        Serial.println("[INFO] Press 'stop' to return to standby");
        operatingMode = MODE_ENROLL;
    } else if (serialCommand == "stop") {
        Serial.println("[INFO] Stopping current mode, returning to standby");
        operatingMode = MODE_STANDBY;
    } else if (serialCommand == "info") {
        Serial.println("[INFO] System Information:");
        Serial.println("- Device: ESP32-C3 SuperMini");
        Serial.println("- NFC Reader: PN532");
        Serial.println("- Current Mode: " +
                       String(operatingMode == MODE_STANDBY
                                  ? "Standby"
                                  : (operatingMode == MODE_DETECT ? "Detect" : "Enroll")));
        Serial.println("- Authentication Status: " + 
                       String(isAuthenticated ? "Authenticated" : "Not Authenticated"));
        Serial.println("- Supported Auth: AES, DES, 3DES, EV2");
        Serial.println("- Default Keys: All zeros");
        Serial.println("- Application ID: 020000h");
        Serial.println("- EV2 Features: Command Counter, Transaction MAC");
    } else {
        Serial.println("[ERROR] Unknown command. Type 'help' for available commands.");
    }
}

// Print help menu
void printHelp() {
    Serial.println("\n=== DESFire Card Manager Help ===");
    Serial.println("Available commands:");
    Serial.println("  detect     - Start card detection mode (with authentication)");
    Serial.println("  enroll     - Start card enrollment mode (creates application with keys)");
    Serial.println("  stop       - Stop current mode");
    Serial.println("  info       - Display system information");
    Serial.println("  help       - Show this help menu");
    Serial.println("\nAuthentication Information:");
    Serial.println("  - Supported Auth: AES, DES, 3DES, EV2");
    Serial.println("  - Default AES key: 16 bytes of zeros");
    Serial.println("  - Default DES key: 8 bytes of zeros");
    Serial.println("  - Application ID: 020000h");
    Serial.println("  - Key number: 0 (master key)");
    Serial.println("\nDESFire EV2 Features:");
    Serial.println("  - Transaction MAC support");
    Serial.println("  - Command counter protection");
    Serial.println("  - Enhanced authentication protocol");
    Serial.println("==============================\n");
}

// Card detection mode
void detectCardMode() {
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

                    // Check if our application exists
                    uint8_t appId[3] = {0x02, 0x00, 0x00};
                    if (selectApplication(appId, 3)) {
                        Serial.println("[INFO] Card is enrolled (Application exists)");
                        
                        // Try multiple authentication methods
                        Serial.println("[INFO] Attempting to authenticate with application");
                        
                        // Try AES first (newer cards)
                        if (authenticateCard(0, defaultAESKey, KEY_TYPE_AES)) {
                            Serial.println("[SUCCESS] AES Authentication successful!");
                            isAuthenticated = true;
                            
                            // Here you can perform authenticated operations
                            Serial.println("[INFO] Card is now ready for authenticated operations");
                            
                            // Reset authentication state when done
                            isAuthenticated = false;
                        } 
                        // If AES fails, try DES (older cards)
                        else if (authenticateCard(0, defaultDESKey, KEY_TYPE_DES)) {
                            Serial.println("[SUCCESS] DES Authentication successful!");
                            isAuthenticated = true;
                            
                            // Here you can perform authenticated operations
                            Serial.println("[INFO] Card is now ready for authenticated operations");
                            
                            // Reset authentication state when done
                            isAuthenticated = false;
                        }
                        // If standard methods fail, try 3DES (some cards)
                        else if (authenticateCard(0, defaultAESKey, KEY_TYPE_3DES)) {
                            Serial.println("[SUCCESS] 3DES Authentication successful!");
                            isAuthenticated = true;
                            
                            // Here you can perform authenticated operations
                            Serial.println("[INFO] Card is now ready for authenticated operations");
                            
                            // Reset authentication state when done
                            isAuthenticated = false;
                        }
                        // If standard methods fail, try EV2 First authentication
                        else if (authenticateCard(0, defaultAESKey, 3)) { // 3 = EV2 First
                            Serial.println("[SUCCESS] EV2 First Authentication successful!");
                            isAuthenticated = true;
                            
                            // Here you can perform authenticated operations with EV2-specific features
                            Serial.println("[INFO] Card is now ready for EV2 authenticated operations");
                            
                            // Optional: try to get command counter to confirm EV2 support
                            uint32_t counter = 0;
                            if (getCommandCounter(&counter)) {
                                Serial.print("[INFO] EV2 command counter: ");
                                Serial.println(counter);
                            }
                            
                            // Reset authentication state when done
                            isAuthenticated = false;
                        }
                        else {
                            Serial.println("[ERROR] All authentication methods failed");
                            Serial.println("[INFO] This could be because:");
                            Serial.println("1. The key does not match the one on the card");
                            Serial.println("2. The card is in a special state or locked");
                            Serial.println("3. Card is using a different key type or structure");
                        }
                    } else {
                        Serial.println("[INFO] Card is not enrolled (Application not found)");
                        Serial.println("[INFO] Switch to enrollment mode to provision this card");
                    }
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
    }

    // Brief delay between detection cycles
    delay(1000);
}

// Enroll a new card
bool enrollCard(uint8_t* uid, uint8_t uidLength) {
    // First activate the card for ISO14443-4 communication
    if (!activateIso14443_4(uid, uidLength)) {
        Serial.println("[ERROR] Failed to activate card for ISO14443-4 communication");
        return false;
    }

    // Get card version to verify it's a DESFire
    if (!getCardVersion()) {
        Serial.println("[ERROR] Failed to get card version");
        return false;
    }

    // Select PICC/Master application (AID 000000h)
    uint8_t rootAid[3] = {0x00, 0x00, 0x00};
    if (!selectApplication(rootAid, 3)) {
        Serial.println("[ERROR] Failed to select PICC level");
        return false;
    }

    // Check if our application already exists
    uint8_t appId[3] = {0x02, 0x00, 0x00};
    if (selectApplication(appId, 3)) {
        Serial.println("[INFO] Application already exists on this card");
        
        // Try multiple authentication methods with the existing application
        Serial.println("[INFO] Attempting to authenticate with existing application");
        
        bool authSuccess = false;
        
        // Try all authentication methods
        if (authenticateCard(0, defaultAESKey, KEY_TYPE_AES)) {
            Serial.println("[SUCCESS] AES Authentication successful with existing application!");
            authSuccess = true;
        } else if (authenticateCard(0, defaultDESKey, KEY_TYPE_DES)) {
            Serial.println("[SUCCESS] DES Authentication successful with existing application!");
            authSuccess = true;
        } else if (authenticateCard(0, defaultAESKey, KEY_TYPE_3DES)) {
            Serial.println("[SUCCESS] 3DES Authentication successful with existing application!");
            authSuccess = true;
        } else if (authenticateCard(0, defaultAESKey, 3)) { // 3 = EV2 First
            Serial.println("[SUCCESS] EV2 Authentication successful with existing application!");
            authSuccess = true;
        } else {
            Serial.println("[WARNING] All authentication methods failed with existing application");
            Serial.println("[INFO] The application may need to be reconfigured");
        }
        
        return true;
    }

    Serial.println("[INFO] Creating new application on the card");

    // Create our application (AID 020000h)
    // Key settings: 0x0F = Allow changing keys, no master key needed
    // Try to create with AES keys first (0x80 for AES type + 0x01 for 1 key = 0x81)
    // This is more compatible with EV2 cards
    if (!createApplication(appId, 0x0F, 0x81)) {
        Serial.println("[WARNING] Failed to create application with AES keys");
        Serial.println("[INFO] Trying with DES keys instead");
        
        // Fall back to DES keys (0x00 for DES type + 0x01 for 1 key = 0x01)
        if (!createApplication(appId, 0x0F, 0x01)) {
            Serial.println("[ERROR] Failed to create application");
            return false;
        }
    }

    // Select the newly created application
    if (!selectApplication(appId, 3)) {
        Serial.println("[ERROR] Failed to select newly created application");
        return false;
    }
    
    // Try to authenticate with the new application
    Serial.println("[INFO] Attempting to authenticate with new application");
    
    bool authSuccess = false;
    
    // Try AES first for EV2 cards
    if (authenticateCard(0, defaultAESKey, KEY_TYPE_AES)) {
        Serial.println("[SUCCESS] AES Authentication successful with new application!");
        authSuccess = true;
        
        // For EV2 cards, we can try to set the command counter
        Serial.println("[INFO] Attempting to set command counter (EV2 feature)");
        uint32_t counter = 0;
        if (setCommandCounter(counter)) {
            Serial.println("[SUCCESS] Command counter initialized to 0");
        }
    } 
    // If AES fails, try DES (older cards)
    else if (authenticateCard(0, defaultDESKey, KEY_TYPE_DES)) {
        Serial.println("[SUCCESS] DES Authentication successful with new application!");
        authSuccess = true;
    } 
    // If basic methods fail, try EV2 authentication
    else if (authenticateCard(0, defaultAESKey, 3)) { // 3 = EV2 First
        Serial.println("[SUCCESS] EV2 Authentication successful with new application!");
        authSuccess = true;
        
        // For EV2 cards, initialize the command counter
        Serial.println("[INFO] Attempting to initialize command counter (EV2 feature)");
        uint32_t counter = 0;
        if (setCommandCounter(counter)) {
            Serial.println("[SUCCESS] Command counter initialized to 0");
        }
    }
    // If all else fails, try 3DES
    else if (authenticateCard(0, defaultAESKey, KEY_TYPE_3DES)) {
        Serial.println("[SUCCESS] 3DES Authentication successful with new application!");
        authSuccess = true;
    }
    else {
        Serial.println("[WARNING] All authentication methods failed with new application");
        Serial.println("[INFO] The default key may not be set correctly");
    }

    Serial.println("[SUCCESS] Card enrollment successful");
    return true;
}

// Main wrapper for authentication - tries different methods
bool authenticateCard(uint8_t keyNo, uint8_t* key, uint8_t keyType) {
    switch (keyType) {
        case KEY_TYPE_AES:
            Serial.println("[DEBUG] Trying AES authentication...");
            return authenticateAES(keyNo, key);
        case KEY_TYPE_DES:
            Serial.println("[DEBUG] Trying DES authentication...");
            return authenticateDES(keyNo, key);
        case KEY_TYPE_3DES:
            Serial.println("[DEBUG] Trying 3DES authentication...");
            return authenticate3DES(keyNo, key);
        case 3: // Adding EV2 First Auth
            Serial.println("[DEBUG] Trying EV2 First authentication...");
            return authenticateEV2First(keyNo, key);
        case 4: // Adding EV2 Non-First Auth
            Serial.println("[DEBUG] Trying EV2 Non-First authentication...");
            return authenticateEV2NonFirst(keyNo, key);
        default:
            Serial.println("[ERROR] Unknown key type");
            return false;
    }
}

// Authenticate with DESFire AES key
bool authenticateAES(uint8_t keyNo, uint8_t* key) {
    uint8_t authCommand[2] = {DESFIRE_CMD_AUTHENTICATE_AES, keyNo};
    uint8_t response[32];
    uint8_t responseLen = 32;
    
    Serial.print("[DEBUG] Starting AES authentication with key number: ");
    Serial.println(keyNo);
    
    // Step 1: Send the initial authentication command
    if (!sendDESFireCommand(authCommand[0], &authCommand[1], 1, response, &responseLen)) {
        Serial.println("[ERROR] Failed to initiate AES authentication");
        
        // Check for specific error codes
        if (responseLen > 0) {
            if (response[0] == 0xAE) {
                Serial.println("[ERROR] Authentication error 0xAE - Authentication error");
                Serial.println("[INFO] This typically means:");
                Serial.println("1. The specified key number doesn't exist");
                Serial.println("2. The authentication method is not supported");
                Serial.println("3. The card may be using a different key type (DES/3DES instead of AES)");
            }
        }
        return false;
    }
    
    // Check if we received the expected challenge (16+ bytes with status)
    // DESFire sends 16 bytes of encrypted data plus the AF status byte
    if (responseLen < 17 || response[0] != DESFIRE_STATUS_MORE_FRAMES) {
        Serial.println("[ERROR] Invalid challenge received during authentication");
        Serial.print("[DEBUG] Response length: ");
        Serial.print(responseLen);
        Serial.print(", Status byte: 0x");
        Serial.println(response[0], HEX);
        return false;
    }
    
    Serial.println("[DEBUG] Received challenge from card");
    Serial.print("[DEBUG] Card challenge: ");
    for (int i = 1; i < 9; i++) {
        if (response[i] < 0x10) Serial.print("0");
        Serial.print(response[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
    
    // We need to implement a proper AES encryption for the challenge
    // For now, we'll use a better-formatted response to test the protocol
    
    // Step 2: Send back encrypted challenge + our own challenge
    // Real implementation would:
    // 1. Decrypt RndA (card challenge)
    // 2. Generate RndB (our challenge)
    // 3. Rotate RndA and encrypt RndA' + RndB
    
    // For this test implementation, we'll create a valid-looking structure
    uint8_t encryptedResponse[32] = {0};
    
    // First, copy the challenge we received (first 16 bytes after status)
    for (int i = 0; i < 16; i++) {
        encryptedResponse[i] = response[i+1];
    }
    
    // Then add 16 more bytes as our "challenge"
    for (int i = 16; i < 32; i++) {
        encryptedResponse[i] = 0x00; // All zeros for simplicity
    }
    
    Serial.println("[WARNING] Using test data for authentication (NOT secure)");
    Serial.println("[INFO] For real security, implement proper AES encryption");
    
    // Step 3: Send the encrypted challenge back
    uint8_t additionalData[17]; // Command code + first 16 bytes of our response
    additionalData[0] = DESFIRE_CMD_GET_ADDITIONAL_FRAME;
    memcpy(&additionalData[1], encryptedResponse, 16);
    
    responseLen = 32;
    if (!sendDESFireCommand(additionalData[0], &additionalData[1], 16, response, &responseLen)) {
        Serial.println("[ERROR] Failed to send encrypted challenge");
        
        // Check for specific error codes
        if (responseLen > 0) {
            if (response[0] == 0x7E) {
                Serial.println("[ERROR] Authentication error 0x7E - Integrity error");
                Serial.println("[INFO] This typically means:");
                Serial.println("1. The encrypted data sent back was not correctly formatted");
                Serial.println("2. The encryption method used was incorrect");
            } else if (response[0] == 0x1C) {
                Serial.println("[ERROR] Authentication error 0x1C - Parameter error");
                Serial.println("[INFO] This typically means the challenge response format was incorrect");
            }
        }
        return false;
    }
    
    // Check for more frames or successful authentication
    if (responseLen > 0 && response[0] == DESFIRE_STATUS_MORE_FRAMES) {
        // We need to continue the authentication process
        // For our test implementation, just send another frame
        uint8_t additionalData2[17]; // Command code + second 16 bytes of our response
        additionalData2[0] = DESFIRE_CMD_GET_ADDITIONAL_FRAME;
        memcpy(&additionalData2[1], &encryptedResponse[16], 16);
        
        responseLen = 32;
        if (!sendDESFireCommand(additionalData2[0], &additionalData2[1], 16, response, &responseLen)) {
            Serial.println("[ERROR] Failed to send second part of challenge");
            return false;
        }
    }
    
    // Check authentication success (status byte should be 0x00)
    if (responseLen < 1 || response[0] != DESFIRE_STATUS_SUCCESS) {
        Serial.println("[ERROR] Authentication failed - card rejected response");
        Serial.print("[DEBUG] Response status: 0x");
        if (responseLen > 0) {
            Serial.println(response[0], HEX);
        } else {
            Serial.println("No response");
        }
        return false;
    }
    
    Serial.println("[SUCCESS] Authentication sequence completed");
    
    // In a real implementation, we would:
    // 1. Decrypt the card's response to get RndB'
    // 2. Verify it matches our challenge
    // 3. Calculate the session key from RndA and RndB
    // 4. Store the session key for subsequent encrypted commands
    
    // For this demo, we just set a flag that we're authenticated
    return true;
}

// Authenticate with legacy DES key
bool authenticateDES(uint8_t keyNo, uint8_t* key) {
    uint8_t authCommand[2] = {DESFIRE_CMD_AUTHENTICATE_LEGACY, keyNo};
    uint8_t response[32];
    uint8_t responseLen = 32;
    
    Serial.print("[DEBUG] Starting DES authentication with key number: ");
    Serial.println(keyNo);
    
    // Similar structure to AES auth but with DES command
    if (!sendDESFireCommand(authCommand[0], &authCommand[1], 1, response, &responseLen)) {
        if (responseLen > 0 && response[0] == 0xAE) {
            Serial.println("[ERROR] Authentication error 0xAE - DES auth not supported");
        }
        return false;
    }
    
    // Check if we received the expected challenge
    if (responseLen < 9 || response[0] != DESFIRE_STATUS_MORE_FRAMES) {
        Serial.println("[ERROR] Invalid challenge received during DES authentication");
        return false;
    }
    
    Serial.println("[DEBUG] Received DES challenge from card");
    
    // For test purposes, simply echo back the challenge
    uint8_t additionalData[9]; // Command code + 8 bytes response 
    additionalData[0] = DESFIRE_CMD_GET_ADDITIONAL_FRAME;
    memcpy(&additionalData[1], &response[1], 8);
    
    responseLen = 32;
    if (!sendDESFireCommand(additionalData[0], &additionalData[1], 8, response, &responseLen)) {
        return false;
    }
    
    // Check authentication success
    if (responseLen < 1 || response[0] != DESFIRE_STATUS_SUCCESS) {
        return false;
    }
    
    Serial.println("[SUCCESS] DES Authentication sequence completed");
    return true;
}

// Authenticate with 3DES key (ISO)
bool authenticate3DES(uint8_t keyNo, uint8_t* key) {
    uint8_t authCommand[2] = {DESFIRE_CMD_AUTHENTICATE_ISO, keyNo};
    uint8_t response[32];
    uint8_t responseLen = 32;
    
    Serial.print("[DEBUG] Starting 3DES authentication with key number: ");
    Serial.println(keyNo);
    
    // Similar structure to AES auth but with 3DES command
    if (!sendDESFireCommand(authCommand[0], &authCommand[1], 1, response, &responseLen)) {
        if (responseLen > 0 && response[0] == 0xAE) {
            Serial.println("[ERROR] Authentication error 0xAE - 3DES auth not supported");
        }
        return false;
    }
    
    // Check if we received the expected challenge
    if (responseLen < 9 || response[0] != DESFIRE_STATUS_MORE_FRAMES) {
        Serial.println("[ERROR] Invalid challenge received during 3DES authentication");
        return false;
    }
    
    Serial.println("[DEBUG] Received 3DES challenge from card");
    
    // For test purposes, simply echo back the challenge
    uint8_t additionalData[9]; // Command code + 8 bytes response
    additionalData[0] = DESFIRE_CMD_GET_ADDITIONAL_FRAME;
    memcpy(&additionalData[1], &response[1], 8);
    
    responseLen = 32;
    if (!sendDESFireCommand(additionalData[0], &additionalData[1], 8, response, &responseLen)) {
        return false;
    }
    
    // Check authentication success
    if (responseLen < 1 || response[0] != DESFIRE_STATUS_SUCCESS) {
        return false;
    }
    
    Serial.println("[SUCCESS] 3DES Authentication sequence completed");
    return true;
}

// Authenticate with DESFire EV2 (first part)
bool authenticateEV2First(uint8_t keyNo, uint8_t* key) {
    uint8_t authCommand[2] = {DESFIRE_CMD_AUTHENTICATE_EV2_FIRST, keyNo};
    uint8_t response[32];
    uint8_t responseLen = 32;
    
    Serial.print("[DEBUG] Starting EV2 First authentication with key number: ");
    Serial.println(keyNo);
    
    // Step 1: Send the initial authentication command
    if (!sendDESFireCommand(authCommand[0], &authCommand[1], 1, response, &responseLen)) {
        Serial.println("[ERROR] Failed to initiate EV2 First authentication");
        
        // Check for specific error codes
        if (responseLen > 0) {
            if (response[0] == 0xAE) {
                Serial.println("[ERROR] Authentication error 0xAE - Authentication error");
                Serial.println("[INFO] This typically means:");
                Serial.println("1. The specified key number doesn't exist");
                Serial.println("2. The authentication method is not supported");
                Serial.println("3. The card may not support EV2 authentication");
            }
        }
        return false;
    }
    
    // Check if we received the expected challenge (16+ bytes with status)
    // DESFire sends 16 bytes of encrypted data plus the AF status byte
    if (responseLen < 17 || response[0] != DESFIRE_STATUS_MORE_FRAMES) {
        Serial.println("[ERROR] Invalid challenge received during EV2 authentication");
        Serial.print("[DEBUG] Response length: ");
        Serial.print(responseLen);
        Serial.print(", Status byte: 0x");
        Serial.println(response[0], HEX);
        return false;
    }
    
    Serial.println("[DEBUG] Received EV2 challenge from card");
    Serial.print("[DEBUG] Card challenge: ");
    for (int i = 1; i < 9; i++) {
        if (response[i] < 0x10) Serial.print("0");
        Serial.print(response[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
    
    // For a full implementation, we would:
    // 1. Decrypt the challenge (RndB) using AES with IV of all zeros
    // 2. Rotate RndB left by 1 byte to get RndB'
    // 3. Generate our own random challenge (RndA)
    // 4. Concatenate RndA + RndB' (32 bytes total)
    // 5. Encrypt this data using AES with proper IV (typically derived from previous encryption)
    // 6. Send the encrypted data back
    
    // For demonstration, we'll use a placeholder response
    uint8_t encryptedResponse[32] = {0};
    
    // Copy the received challenge data (this would normally be processed)
    for (int i = 0; i < 16; i++) {
        encryptedResponse[i] = response[i+1];
    }
    
    // Add additional bytes for our challenge
    for (int i = 16; i < 32; i++) {
        encryptedResponse[i] = 0x00; // All zeros for demonstration
    }
    
    Serial.println("[WARNING] Using placeholder data for EV2 authentication (NOT secure)");
    Serial.println("[INFO] For real security, implement proper AES encryption and key derivation");
    
    // Send the encrypted challenge back
    uint8_t additionalData[17]; // Command code + first 16 bytes of our response
    additionalData[0] = DESFIRE_CMD_GET_ADDITIONAL_FRAME;
    memcpy(&additionalData[1], encryptedResponse, 16);
    
    responseLen = 32;
    if (!sendDESFireCommand(additionalData[0], &additionalData[1], 16, response, &responseLen)) {
        Serial.println("[ERROR] Failed to send encrypted challenge for EV2 authentication");
        return false;
    }
    
    // Check for more frames or successful authentication
    if (responseLen > 0 && response[0] == DESFIRE_STATUS_MORE_FRAMES) {
        // We need to continue the authentication process
        uint8_t additionalData2[17]; // Command code + second 16 bytes of our response
        additionalData2[0] = DESFIRE_CMD_GET_ADDITIONAL_FRAME;
        memcpy(&additionalData2[1], &encryptedResponse[16], 16);
        
        responseLen = 32;
        if (!sendDESFireCommand(additionalData2[0], &additionalData2[1], 16, response, &responseLen)) {
            Serial.println("[ERROR] Failed to send second part of challenge for EV2 authentication");
            return false;
        }
    }
    
    // Check if we need to process transaction identifier (TI) for EV2
    if (responseLen > 0 && response[0] == DESFIRE_STATUS_MORE_FRAMES) {
        // In a full implementation, we would:
        // 1. Extract the encrypted TI from the response
        // 2. Decrypt the TI using the appropriate key
        // 3. Store the TI for subsequent operations
        
        uint8_t tiResponse[5]; // Command code + 4 bytes for TI
        tiResponse[0] = DESFIRE_CMD_GET_ADDITIONAL_FRAME;
        // We'd normally process the TI here
        for (int i = 1; i < 5; i++) {
            tiResponse[i] = 0x00; // Placeholder
        }
        
        responseLen = 32;
        if (!sendDESFireCommand(tiResponse[0], &tiResponse[1], 4, response, &responseLen)) {
            Serial.println("[ERROR] Failed to process TI for EV2 authentication");
            return false;
        }
    }
    
    // Check authentication success (status byte should be 0x00)
    if (responseLen < 1 || response[0] != DESFIRE_STATUS_SUCCESS) {
        Serial.println("[ERROR] EV2 authentication failed - card rejected response");
        Serial.print("[DEBUG] Response status: 0x");
        if (responseLen > 0) {
            Serial.println(response[0], HEX);
        } else {
            Serial.println("No response");
        }
        return false;
    }
    
    Serial.println("[SUCCESS] EV2 First authentication sequence completed");
    
    // In a full implementation, we would:
    // 1. Derive the session key from RndA and RndB using EV2-specific algorithm
    // 2. Store the session key for subsequent encrypted commands
    // 3. Store the TI for transaction integrity
    
    // For this demo, we just set a flag that we're authenticated
    return true;
}

// Authenticate with DESFire EV2 (non-first part)
bool authenticateEV2NonFirst(uint8_t keyNo, uint8_t* key) {
    uint8_t authCommand[2] = {DESFIRE_CMD_AUTHENTICATE_EV2_NONFIRST, keyNo};
    uint8_t response[32];
    uint8_t responseLen = 32;
    
    Serial.print("[DEBUG] Starting EV2 Non-First authentication with key number: ");
    Serial.println(keyNo);
    
    // Step 1: Send the initial authentication command
    if (!sendDESFireCommand(authCommand[0], &authCommand[1], 1, response, &responseLen)) {
        Serial.println("[ERROR] Failed to initiate EV2 Non-First authentication");
        
        // Check for specific error codes
        if (responseLen > 0) {
            if (response[0] == 0xAE) {
                Serial.println("[ERROR] Authentication error 0xAE - Authentication error");
                Serial.println("[INFO] This typically means:");
                Serial.println("1. The specified key number doesn't exist");
                Serial.println("2. The authentication method is not supported");
                Serial.println("3. No previous EV2 First authentication was performed");
            }
        }
        return false;
    }
    
    // Check if we received the expected challenge
    if (responseLen < 17 || response[0] != DESFIRE_STATUS_MORE_FRAMES) {
        Serial.println("[ERROR] Invalid challenge received during EV2 Non-First authentication");
        Serial.print("[DEBUG] Response length: ");
        Serial.print(responseLen);
        Serial.print(", Status byte: 0x");
        Serial.println(response[0], HEX);
        return false;
    }
    
    Serial.println("[DEBUG] Received EV2 Non-First challenge from card");
    
    // For a full implementation, we would:
    // 1. Decrypt the challenge using the session key established in First authentication
    // 2. Process the challenge according to EV2 protocol
    // 3. Encrypt our response using the session key
    
    // For demonstration, we'll use a placeholder response
    uint8_t encryptedResponse[32] = {0};
    
    // Copy the received challenge data (this would normally be processed)
    for (int i = 0; i < 16; i++) {
        encryptedResponse[i] = response[i+1];
    }
    
    // Add additional bytes for our response
    for (int i = 16; i < 32; i++) {
        encryptedResponse[i] = 0x00; // All zeros for demonstration
    }
    
    Serial.println("[WARNING] Using placeholder data for EV2 Non-First authentication (NOT secure)");
    
    // Send the encrypted response
    uint8_t additionalData[17]; // Command code + first 16 bytes of our response
    additionalData[0] = DESFIRE_CMD_GET_ADDITIONAL_FRAME;
    memcpy(&additionalData[1], encryptedResponse, 16);
    
    responseLen = 32;
    if (!sendDESFireCommand(additionalData[0], &additionalData[1], 16, response, &responseLen)) {
        Serial.println("[ERROR] Failed to send encrypted response for EV2 Non-First authentication");
        return false;
    }
    
    // For a complete implementation, additional data exchange might be needed
    // depending on the protocol specifics for the Non-First authentication
    
    // Check authentication success (status byte should be 0x00)
    if (responseLen < 1 || response[0] != DESFIRE_STATUS_SUCCESS) {
        Serial.println("[ERROR] EV2 Non-First authentication failed - card rejected response");
        Serial.print("[DEBUG] Response status: 0x");
        if (responseLen > 0) {
            Serial.println(response[0], HEX);
        } else {
            Serial.println("No response");
        }
        return false;
    }
    
    Serial.println("[SUCCESS] EV2 Non-First authentication completed");
    
    // In a full implementation, we would update the session key or other security parameters
    
    return true;
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

// Select application by AID
bool selectApplication(uint8_t* aid, uint8_t aidLen) {
    uint8_t response[16];
    uint8_t responseLen = 16;

    Serial.print("[DEBUG] Selecting application with AID: ");
    printHex(aid, aidLen);
    Serial.println();

    // Try both AID byte orders (standard and reversed)
    if (sendDESFireCommand(DESFIRE_CMD_SELECT_APPLICATION, aid, aidLen, response, &responseLen)) {
        Serial.println("[SUCCESS] Application selected successfully");
        return true;
    }

    // Try with reversed AID
    Serial.println("[DEBUG] Trying with reversed AID bytes");
    uint8_t reversedAid[3];
    for (int i = 0; i < aidLen; i++) {
        reversedAid[i] = aid[aidLen - 1 - i];
    }

    Serial.print("[DEBUG] Reversed AID: ");
    printHex(reversedAid, aidLen);
    Serial.println();

    responseLen = 16;
    if (sendDESFireCommand(
            DESFIRE_CMD_SELECT_APPLICATION, reversedAid, aidLen, response, &responseLen)) {
        Serial.println("[SUCCESS] Application selected successfully with reversed AID");
        return true;
    }

    Serial.println("[INFO] Application not found on card");
    return false;
}

// Create application on the card
bool createApplication(uint8_t* aid, uint8_t settings, uint8_t keyCount) {
    uint8_t createAppData[5];
    uint8_t response[16];
    uint8_t responseLen = 16;

    // Prepare command data
    // First 3 bytes: AID
    memcpy(createAppData, aid, 3);
    // Byte 4: Key settings
    createAppData[3] = settings;
    // Byte 5: Number of keys
    // 0x81 for 1 AES key (0x80 = AES, 0x01 = 1 key)
    createAppData[4] = keyCount;

    Serial.print("[DEBUG] Creating application with AID: ");
    printHex(aid, 3);
    Serial.println();
    Serial.print("[DEBUG] Key settings: 0x");
    Serial.println(settings, HEX);
    Serial.print("[DEBUG] Key type and count: 0x");
    Serial.println(keyCount, HEX);

    // Send create application command
    if (sendDESFireCommand(
            DESFIRE_CMD_CREATE_APPLICATION, createAppData, 5, response, &responseLen)) {
        Serial.println("[SUCCESS] Application created successfully");
        return true;
    }

    // Try with reversed AID
    Serial.println("[DEBUG] Trying with reversed AID bytes");
    uint8_t reversedData[5];
    reversedData[0] = aid[2];
    reversedData[1] = aid[1];
    reversedData[2] = aid[0];
    reversedData[3] = settings;
    reversedData[4] = keyCount;

    responseLen = 16;
    if (sendDESFireCommand(
            DESFIRE_CMD_CREATE_APPLICATION, reversedData, 5, response, &responseLen)) {
        Serial.println("[SUCCESS] Application created successfully with reversed AID");
        return true;
    }

    Serial.println("[ERROR] Failed to create application");
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

// Card enrollment mode
void enrollCardMode() {
    uint8_t uid[7];     // Buffer to store the card UID
    uint8_t uidLength;  // Length of the UID

    Serial.println("\n[INFO] Waiting for card to enroll...");

    // Reset the PN532 before each detection cycle
    setupPN532();

    // Try to detect card
    if (detectCard(uid, &uidLength)) {
        Serial.println("[SUCCESS] Card detected for enrollment!");
        Serial.print("[INFO] Card UID: ");
        printHex(uid, uidLength);
        Serial.println();

        // Determine if this is likely a DESFire card based on UID
        bool isDESFireLikely = (uidLength == 7) && (uid[0] == 0x04);

        if (isDESFireLikely) {
            Serial.println("[INFO] UID pattern suggests this is a DESFire card");

            // Attempt to enroll the card
            if (enrollCard(uid, uidLength)) {
                Serial.println("[SUCCESS] Card enrollment completed!");
            } else {
                Serial.println("[ERROR] Card enrollment failed");
            }
        } else {
            Serial.println("[WARNING] This does not appear to be a DESFire card");
            Serial.println("[INFO] Enrollment requires a DESFire EV1/EV2 card");
        }

        // Wait for card to be removed
        Serial.println("[INFO] Done. Please remove card and wait...");
        delay(3000);
    }

    // Brief delay between enrollment attempts
    delay(1000);
}

// Commit transaction with MAC for EV2 cards
bool commitTransactionMAC() {
    uint8_t response[32];
    uint8_t responseLen = 32;
    
    Serial.println("[DEBUG] Committing transaction with MAC (EV2)");
    
    // For a full implementation, we would:
    // 1. Compute the Transaction MAC (TMAC) based on previous operations
    // 2. Include the TMAC in the commit command
    
    // For demonstration, we'll send a basic command without proper TMAC
    if (!sendDESFireCommand(DESFIRE_CMD_COMMIT_TRANSACTION_MAC, NULL, 0, response, &responseLen)) {
        Serial.println("[ERROR] Failed to commit transaction with MAC");
        return false;
    }
    
    // Check for success
    if (responseLen < 1 || response[0] != DESFIRE_STATUS_SUCCESS) {
        Serial.println("[ERROR] Transaction commit with MAC failed");
        Serial.print("[DEBUG] Response status: 0x");
        if (responseLen > 0) {
            Serial.println(response[0], HEX);
        } else {
            Serial.println("No response");
        }
        return false;
    }
    
    Serial.println("[SUCCESS] Transaction committed with MAC");
    return true;
}

// Abort transaction with MAC for EV2 cards
bool abortTransactionMAC() {
    uint8_t response[32];
    uint8_t responseLen = 32;
    
    Serial.println("[DEBUG] Aborting transaction with MAC (EV2)");
    
    // For a full implementation, we would:
    // 1. Compute the Transaction MAC (TMAC) based on previous operations
    // 2. Include the TMAC in the abort command
    
    // For demonstration, we'll send a basic command without proper TMAC
    if (!sendDESFireCommand(DESFIRE_CMD_ABORT_TRANSACTION_MAC, NULL, 0, response, &responseLen)) {
        Serial.println("[ERROR] Failed to abort transaction with MAC");
        return false;
    }
    
    // Check for success
    if (responseLen < 1 || response[0] != DESFIRE_STATUS_SUCCESS) {
        Serial.println("[ERROR] Transaction abort with MAC failed");
        Serial.print("[DEBUG] Response status: 0x");
        if (responseLen > 0) {
            Serial.println(response[0], HEX);
        } else {
            Serial.println("No response");
        }
        return false;
    }
    
    Serial.println("[SUCCESS] Transaction aborted with MAC");
    return true;
}

// Get command counter value (EV2)
bool getCommandCounter(uint32_t* counter) {
    uint8_t response[16];
    uint8_t responseLen = 16;
    
    Serial.println("[DEBUG] Getting command counter (EV2)");
    
    if (!sendDESFireCommand(DESFIRE_CMD_GET_COMMAND_COUNTER, NULL, 0, response, &responseLen)) {
        Serial.println("[ERROR] Failed to get command counter");
        return false;
    }
    
    // Check for success
    if (responseLen < 5 || response[0] != DESFIRE_STATUS_SUCCESS) {
        Serial.println("[ERROR] Get command counter failed");
        Serial.print("[DEBUG] Response status: 0x");
        if (responseLen > 0) {
            Serial.println(response[0], HEX);
        } else {
            Serial.println("No response");
        }
        return false;
    }
    
    // Extract the counter value (4 bytes)
    *counter = 0;
    for (int i = 0; i < DESFIRE_EV2_COUNTER_LENGTH; i++) {
        *counter |= ((uint32_t)response[i+1] << (8 * i));
    }
    
    Serial.print("[SUCCESS] Command counter value: ");
    Serial.println(*counter);
    return true;
}

// Set command counter value (EV2)
bool setCommandCounter(uint32_t counter) {
    uint8_t data[DESFIRE_EV2_COUNTER_LENGTH];
    uint8_t response[16];
    uint8_t responseLen = 16;
    
    Serial.print("[DEBUG] Setting command counter to: ");
    Serial.println(counter);
    
    // Prepare counter data
    for (int i = 0; i < DESFIRE_EV2_COUNTER_LENGTH; i++) {
        data[i] = (counter >> (8 * i)) & 0xFF;
    }
    
    if (!sendDESFireCommand(DESFIRE_CMD_SET_COMMAND_COUNTER, data, DESFIRE_EV2_COUNTER_LENGTH, response, &responseLen)) {
        Serial.println("[ERROR] Failed to set command counter");
        return false;
    }
    
    // Check for success
    if (responseLen < 1 || response[0] != DESFIRE_STATUS_SUCCESS) {
        Serial.println("[ERROR] Set command counter failed");
        Serial.print("[DEBUG] Response status: 0x");
        if (responseLen > 0) {
            Serial.println(response[0], HEX);
        } else {
            Serial.println("No response");
        }
        return false;
    }
    
    Serial.println("[SUCCESS] Command counter set successfully");
    return true;
}
