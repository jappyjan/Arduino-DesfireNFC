#ifndef NFC_H
#define NFC_H

// NOTE: The warning about "PN532_I2C_ADDRESS redefined" comes from multiple definitions
// (e.g. one from your build configuration or command-line and one in the library).
// You can safely ignore this warning, or if needed, undefine the macro before including
// the Adafruit PN532 library, for example:
//
// #ifdef PN532_I2C_ADDRESS
//   #undef PN532_I2C_ADDRESS
// #endif

#include <PubSubClient.h>   // For MQTT functionality
#include <Pins.h>           // Board-specific pin definitions
#include <Wire.h>           // For I2C communication
#include <Adafruit_PN532.h> // NFC library
#include <Arduino.h>        // For String and other Arduino types

#define APDU_BUFFER_SIZE 255 // Buffer size for APDU communication (max 255 for uint8_t)

// Error codes for card operations
enum CardErrorCode
{
    CARD_ERROR_NONE = 0,              // No error
    CARD_ERROR_NO_CARD = 1,           // No card detected
    CARD_ERROR_UNSUPPORTED_TYPE = 2,  // Unsupported card type
    CARD_ERROR_RATS_FAILED = 3,       // RATS command failed
    CARD_ERROR_PPS_FAILED = 4,        // PPS command failed
    CARD_ERROR_TRANSCEIVE_FAILED = 5, // General transceive error
    CARD_ERROR_I2C_ERROR = 6          // I2C communication error
};

// The NFC class encapsulates the functionality to interact with a PN532-based NFC reader.
class NFC
{
private:
    uint8_t uid[7];                                // Buffer to hold the card's UID (supports up to 7 bytes for ISO14443A)
    uint8_t uidLength;                             // Actual length of the UID (set by readPassiveTargetID)
    uint8_t pcb = 0x0A;                            // Protocol Control Byte (example value)
    uint8_t cid = 0x00;                            // Card Identifier (example value)
    bool cardSelected = false;                     // Flag indicating if a card is currently selected/connected
    CardErrorCode lastErrorCode = CARD_ERROR_NONE; // Last error code
    String lastErrorMessage = "";                  // Last error message description

public:
    // Pointer to the PN532 instance for interfacing with the NFC chip.
    Adafruit_PN532 *rfid;

    // Constructor: Initializes the NFC reader using the provided SDA and SCL pins.
    explicit NFC(int pin_sda, int pin_scl);

    // Test the NFC hardware (e.g., via a self-test, if supported).
    bool testNFC();

    // Attempt to detect an NFC card within a specified timeout period.
    bool checkforCard();

    // Connect to a detected card by performing RATS and PPS exchanges.
    bool connecttoCard();

    // Disconnect from the currently connected card using the HALT command.
    bool disconnectCard();

    // Placeholder for card-specific tests; extend as needed.
    bool testCard();

    // Returns true if a card has been successfully selected/connected.
    bool hasCardSelected() const;

    // Returns the UID of the detected card as a hexadecimal String.
    String getUID() const;

    // Get the last error code
    CardErrorCode getLastErrorCode() const { return lastErrorCode; }

    // Get the last error message
    String getLastErrorMessage() const { return lastErrorMessage; }

    // Get card type information
    String getCardType() const
    {
        if (uidLength == 7)
            return "DESFire";
        else if (uidLength == 4)
            return "MIFARE Classic";
        else
            return "Unknown";
    }

    uint8_t Transceive(uint8_t *command, uint8_t command_len, uint8_t *response, uint8_t *response_len);
};

#endif // NFC_H
