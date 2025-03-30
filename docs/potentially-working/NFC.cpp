#include "NFC.h"
#include <SPI.h>
#include <PubSubClient.h>
#include "helpers.h"
#include <ArduinoLog.h>
#include <Adafruit_PN532.h>

NFC::NFC(int pin_sda, int pin_scl)
{
    Log.trace(F("NFC: Start"));

    // Set I2C clock to 25kHz to improve reliability with problematic connections
    Wire.setClock(25000);
    Log.trace(F("NFC: I2C bus speed set to 25kHz for improved reliability"));

    // Create a new PN532 instance with the defined IRQ and RESET pins.
    rfid = new Adafruit_PN532(PN532_IRQ, PN532_RESET);
    rfid->begin(); // Initialize the PN532 chip

    // Retrieve the firmware version from the PN532.
    uint32_t versiondata = rfid->getFirmwareVersion();
    if (!versiondata)
    {
        // If no version data is returned, the PN532 was not detected.
        Log.error(F("Didn't find PN53x board"));
        delay(5000);
        ESP.restart();
    }

    // Print chip and firmware details using formatted output.
    Log.trace(F("NFC: Found chip PN5%02X"), (versiondata >> 24) & 0xFF);
    Log.trace(F("NFC: Firmware ver. %d.%d"), (versiondata >> 16) & 0xFF, (versiondata >> 8) & 0xFF);
}

// Convert the card's UID (stored in a byte array) into a hexadecimal String.
String NFC::getUID() const
{
    String uidStr;
    uidStr.reserve(uidLength * 2); // Reserve memory to optimize String allocation.

    for (uint8_t i = 0; i < uidLength; i++)
    {
        // Ensure each byte is represented by two hex digits.
        if (uid[i] < 0x10)
        {
            uidStr += "0";
        }
        uidStr += String(uid[i], HEX); // Convert byte to hex string.
    }
    return uidStr;
}

// Returns true if a card has been successfully selected/connected.
bool NFC::hasCardSelected() const
{
    return cardSelected;
}

// This function is a placeholder for a possible PN532 self-test.
// Uncomment and implement if your PN532 supports self-testing.
bool NFC::testNFC()
{
    // return rfid->PCD_PerformSelfTest();
    return true;
}

// Check for the presence of an NFC card within a 1000ms timeout.
bool NFC::checkforCard()
{
    // Reset error state
    lastErrorCode = CARD_ERROR_NONE;
    lastErrorMessage = "";

    // Log that we're starting card detection
    Log.verbose(F("Starting card detection with timeout of 1000ms"));

    // Attempt to read the card's UID (ISO14443A standard) within 1000 ms.
    bool success = rfid->readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 1000);
    if (!success)
    {
        Log.warning(F("No card found within the timeout period"));
        lastErrorCode = CARD_ERROR_NO_CARD;
        lastErrorMessage = "No card found";
        return false;
    }

    // Validate the card's UID length: 7 for DESFire, 4 for MIFARE Classic/ISO14443A.
    if (uidLength != 7 && uidLength != 4)
    {
        Log.warning(F("Detected card with unsupported UID length: %d"), uidLength);
        lastErrorCode = CARD_ERROR_UNSUPPORTED_TYPE;
        lastErrorMessage = "Unsupported card type";
        return false;
    }

    // Report the type of card detected.
    if (uidLength == 7)
    {
        Log.info(F("DESFire card detected"));
    }
    else
    {
        Log.info(F("MIFARE Classic or another ISO14443A card detected"));
    }

    // Print the card's UID in hexadecimal format.
    Log.trace(F("UID: %s"), getUID().c_str());

    // Additional detailed debug information about the card
    Log.verbose(F("Card details - UID Length: %d, Card Type: %s"),
                uidLength, getCardType().c_str());

    return true;
}

// Connect to the detected card using RATS (Request for ATS) and PPS (Protocol and Parameter Selection).
bool NFC::connecttoCard()
{
    // Store card type information for external access
    String cardTypeInfo = (uidLength == 7) ? "DESFire" : (uidLength == 4 ? "MIFARE Classic" : "Unknown");

    // Log detailed card info before attempting connection
    Log.trace(F("Attempting to connect to %s card with UID: %s"), cardTypeInfo.c_str(), getUID().c_str());

    // Check if card is supported
    if (uidLength != 7 && uidLength != 4)
    {
        Log.error(F("Unsupported card type: UID length %d is not recognized"), uidLength);
        lastErrorCode = CARD_ERROR_UNSUPPORTED_TYPE;
        lastErrorMessage = "Unsupported card type";
        return false;
    }

    // --- RATS (Request for ATS) ---
    // RATS command structure: [0xE0, parameter]
    // The parameter byte encodes the Frame Size and other options.
    // 0xE0 identifies the command; 0x80 is an example parameter for frame size.
    uint8_t ratsCmd[] = {0xE0, 0x80};
    uint8_t atsResponse[16] = {0}; // Buffer to store ATS response.
    uint8_t atsResponseLength = sizeof(atsResponse);

    // Log RATS command being sent
    Log.verbose(F("Sending RATS command: 0x%02X 0x%02X"), ratsCmd[0], ratsCmd[1]);

    // Send the RATS command and check for errors.
    int ratsStatus = rfid->inDataExchange(ratsCmd, sizeof(ratsCmd), atsResponse, &atsResponseLength);
    if (ratsStatus < 0)
    {
        Log.error(F("Failed ATS command with status: %d"), ratsStatus);
        // More detailed error description based on error code
        if (ratsStatus == -1)
        {
            Log.error(F("I2C communication error during RATS - possibly incompatible ISO14443-4 implementation"));

            // Specific handling for the Wire.cpp error we're seeing
            Log.warning(F("Attempting to recover from I2C error..."));

            // Increase the delay to give more time for I2C bus to recover
            delay(100);
            ratsStatus = rfid->inDataExchange(ratsCmd, sizeof(ratsCmd), atsResponse, &atsResponseLength);

            if (ratsStatus < 0)
            {
                // Still failed after retry
                Log.error(F("Recovery attempt failed for RATS command with status: %d"), ratsStatus);
                lastErrorCode = CARD_ERROR_I2C_ERROR;
                lastErrorMessage = "I2C communication error";

                // Despite the I2C error, we might still be able to use the card
                // For some cards, RATS might fail but basic card operations still work
                Log.warning(F("Continuing with card despite RATS failure - limited functionality may be available"));
                cardSelected = true;
                pcb = 0x0A;
                return true;
            }
            else
            {
                // Recovery succeeded
                Log.info(F("Successfully recovered from I2C error"));
            }
        }
        else if (ratsStatus == -2)
        {
            Log.error(F("Timeout error during RATS command"));
        }

        if (ratsStatus < 0)
        {
            lastErrorCode = CARD_ERROR_RATS_FAILED;
            lastErrorMessage = "RATS command failed";

            // If it's a DESFire card and RATS failed, it might not support ISO14443-4 protocol
            if (uidLength == 7)
            {
                Log.warning(F("This DESFire card may not support ISO14443-4 protocol or may require different parameters"));
            }

            return false;
        }
    }

    // Log successful RATS response
    Log.verbose(F("RATS command successful, received %d bytes response"), atsResponseLength);
    if (atsResponseLength > 0)
    {
        String atsHex = "";
        for (int i = 0; i < atsResponseLength; i++)
        {
            if (atsResponse[i] < 0x10)
                atsHex += "0";
            atsHex += String(atsResponse[i], HEX) + " ";
        }
        Log.verbose(F("ATS Response: %s"), atsHex.c_str());
    }

    // --- PPS (Protocol and Parameter Selection) ---
    // PPS command example: [0xD0, PPS0, PPS1]
    // 0xD0 indicates the PPS command; 0x11 and 0x00 are example parameters.
    uint8_t ppsCmd[] = {0xD0, 0x11, 0x00};
    uint8_t ppsResponse[16] = {0}; // Buffer to store PPS response.
    uint8_t ppsResponseLength = sizeof(ppsResponse);

    // Log PPS command being sent
    Log.verbose(F("Sending PPS command: 0x%02X 0x%02X 0x%02X"), ppsCmd[0], ppsCmd[1], ppsCmd[2]);

    // Send the PPS command and check for errors.
    int ppsStatus = rfid->inDataExchange(ppsCmd, sizeof(ppsCmd), ppsResponse, &ppsResponseLength);
    if (ppsStatus < 0)
    {
        Log.error(F("Failed PPS command with status: %d"), ppsStatus);
        // More detailed error description based on error code
        if (ppsStatus == -1)
        {
            Log.error(F("I2C communication error during PPS - possibly incompatible ISO14443-4 implementation"));

            // Specific handling for the Wire.cpp error we're seeing
            Log.warning(F("Attempting to recover from I2C error..."));

            // Increase the delay to give more time for I2C bus to recover
            delay(100);
            ppsStatus = rfid->inDataExchange(ppsCmd, sizeof(ppsCmd), ppsResponse, &ppsResponseLength);

            if (ppsStatus < 0)
            {
                // Still failed after retry
                Log.error(F("Recovery attempt failed for PPS command with status: %d"), ppsStatus);
                lastErrorCode = CARD_ERROR_I2C_ERROR;
                lastErrorMessage = "I2C communication error";
            }
            else
            {
                // Recovery succeeded
                Log.info(F("Successfully recovered from I2C error"));
                // Continue with normal processing
                goto pps_success;
            }
        }
        else if (ppsStatus == -2)
        {
            Log.error(F("Timeout error during PPS command"));
        }

        lastErrorCode = CARD_ERROR_PPS_FAILED;
        lastErrorMessage = "PPS command failed";

        // Even if PPS fails, we might still be able to communicate with the card
        // For backwards compatibility, we'll continue but warn about it
        Log.warning(F("Continuing with card communication despite PPS failure"));
        cardSelected = true;
        pcb = 0x0A;
        return true;
    }

pps_success:
    // Log successful PPS response
    Log.verbose(F("PPS command successful, received %d bytes response"), ppsResponseLength);
    if (ppsResponseLength > 0)
    {
        String ppsHex = "";
        for (int i = 0; i < ppsResponseLength; i++)
        {
            if (ppsResponse[i] < 0x10)
                ppsHex += "0";
            ppsHex += String(ppsResponse[i], HEX) + " ";
        }
        Log.verbose(F("PPS Response: %s"), ppsHex.c_str());
    }

    // If both RATS and PPS commands succeed, mark the card as selected.
    cardSelected = true;
    lastErrorCode = CARD_ERROR_NONE;
    lastErrorMessage = "";

    // Save any card-specific data if needed here.
    // Note: Unlike MFRC522, Adafruit_PN532 does not store the UID in a public member.
    // We save the UID when we detect the card.
    pcb = 0x0A; // Example PCB (Protocol Control Byte) value.

    Log.info(F("Successfully connected to %s card"), cardTypeInfo.c_str());
    return true;
}

// Disconnect from the card by issuing an ISO14443A HALT command.
bool NFC::disconnectCard()
{
    // HALT command for ISO14443A cards in auto-CRC mode: [0x50, 0x00]
    // You can either compute the CRC or, if your setup automatically appends it,
    // simply send the command without CRC bytes.
    // Here, we assume you have to include the CRC.
    // Note: The correct CRC for [0x50, 0x00] is usually 0x00, 0x00 if the PN532
    // is set to auto-calculate CRC, or you can compute it if needed.

    // Option 1: If your PN532 is configured to append the CRC automatically,
    // you can send just the two-byte command.
    uint8_t haltCmd[] = {0x50, 0x00};
    uint8_t response[8] = {0}; // Buffer to hold any response.
    uint8_t responseLength = sizeof(response);

    // Send the HALT command. A negative return value indicates failure.
    int status = rfid->inDataExchange(haltCmd, sizeof(haltCmd), response, &responseLength);
    if (status < 0)
    {
        Log.error(F("Failed to send HALT command: %d"), status); // Combined Serial.println calls
        return false;
    }

    // Option 2: If you need to send a complete command with CRC, you must
    // calculate the CRC bytes for [0x50, 0x00]. For many ISO14443A cards, the CRC
    // may be 0x00, 0x00 when using a PN532 in auto-CRC mode, so this might work:
    //
    // uint8_t haltCmd[] = { 0x50, 0x00, 0x00, 0x00 };
    // int status = rfid->inDataExchange(haltCmd, sizeof(haltCmd), response, &responseLength);
    // if (status < 0) { ... }
    //
    // Uncomment the above block and comment out the Option 1 block if needed.

    return true;
}

// A placeholder function to test card functionalities; extend as needed.
bool NFC::testCard()
{
    return true;
}

uint8_t NFC::Transceive(uint8_t *command, uint8_t command_len, uint8_t *response, uint8_t *response_len)
{
    // Reset error state
    lastErrorCode = CARD_ERROR_NONE;
    lastErrorMessage = "";

    // Validate input parameters
    if (command == nullptr || response == nullptr || response_len == nullptr)
    {
        Log.error(F("Transceive: Null pointer provided"));
        lastErrorCode = CARD_ERROR_TRANSCEIVE_FAILED;
        lastErrorMessage = "Null pointer provided";
        return 0x01; // General error
    }

    // Check if command length exceeds the maximum allowed (command_len + 2 <= 255)
    if (command_len > 253)
    {
        Log.error(F("Transceive: Command too long (%d bytes)"), command_len);
        lastErrorCode = CARD_ERROR_TRANSCEIVE_FAILED;
        lastErrorMessage = "Command too long";
        return 0x01; // General error (no room for the command)
    }

    // Construct the request buffer with PCB and CID
    uint8_t request_buffer[command_len + 2]; // PCB + CID + command data
    request_buffer[0] = pcb;                 // Protocol Control Byte
    request_buffer[1] = cid;                 // Card Identifier
    memcpy(&request_buffer[2], command, command_len);

    // Toggle PCB as per ISO-DEP protocol
    pcb = (pcb == 0x0A) ? 0x0B : 0x0A;

    // Log the command being sent in hex format
    String cmdHex = "";
    for (int i = 0; i < command_len + 2; i++)
    {
        if (request_buffer[i] < 0x10)
            cmdHex += "0";
        cmdHex += String(request_buffer[i], HEX) + " ";
    }
    Log.verbose(F("Transceive: Sending command: %s"), cmdHex.c_str());

    // Prepare response buffer - use a larger buffer to be safe
    uint8_t response_buffer[APDU_BUFFER_SIZE] = {0}; // Use the defined buffer size and initialize to 0
    uint8_t response_length = APDU_BUFFER_SIZE;      // Max allowed response length

    // Transceive data using Adafruit's inDataExchange
    Log.trace(F("Sending %d bytes to card"), command_len + 2);
    uint8_t status = rfid->inDataExchange(request_buffer, command_len + 2, response_buffer, &response_length);

    // Check if the operation was successful
    if (status != 0x00)
    {
        // If this is an I2C error, try to recover with a retry
        if (status == -1)
        {
            Log.error(F("Transceive: I2C Error - attempting to recover"));

            // Add a delay to give I2C bus time to recover
            delay(100);

            // Try once more
            status = rfid->inDataExchange(request_buffer, command_len + 2, response_buffer, &response_length);

            if (status == 0x00)
            {
                Log.info(F("Transceive: Successfully recovered from I2C error"));
                // Continue with normal processing since we recovered
                goto transceive_success;
            }
        }

        Log.error(F("Transceive: inDataExchange failed with status 0x%02X"), status);

        // Provide more detailed error information
        if (status == 0x01)
        {
            Log.error(F("Transceive: PN532 Error - timeout or CRC error"));
            lastErrorMessage = "PN532 timeout or CRC error";
        }
        else if (status == 0x02)
        {
            Log.error(F("Transceive: PN532 Error - wrong parity"));
            lastErrorMessage = "PN532 wrong parity";
        }
        else if (status == 0x03)
        {
            Log.error(F("Transceive: PN532 Error - NAK received"));
            lastErrorMessage = "PN532 NAK received";
        }
        else if (status == 0x04)
        {
            Log.error(F("Transceive: PN532 Error - invalid card response"));
            lastErrorMessage = "PN532 invalid card response";
        }
        else if (status == -1)
        {
            Log.error(F("Transceive: I2C Error - communication failed"));
            lastErrorMessage = "I2C communication error";
            lastErrorCode = CARD_ERROR_I2C_ERROR;
            return status;
        }
        else
        {
            lastErrorMessage = "Unknown error";
        }

        lastErrorCode = CARD_ERROR_TRANSCEIVE_FAILED;
        return status; // Return the Adafruit_PN532 error code
    }

transceive_success:
    // Check if response contains at least PCB and CID
    if (response_length < 2)
    {
        Log.error(F("Transceive: Response too short (%d bytes)"), response_length);
        lastErrorCode = CARD_ERROR_TRANSCEIVE_FAILED;
        lastErrorMessage = "Response too short";
        return 0x01; // Invalid response
    }

    // Log the response received in hex format
    String respHex = "";
    for (int i = 0; i < response_length; i++)
    {
        if (response_buffer[i] < 0x10)
            respHex += "0";
        respHex += String(response_buffer[i], HEX) + " ";
    }
    Log.verbose(F("Transceive: Received response: %s"), respHex.c_str());

    // Calculate safe response length
    uint8_t data_length = response_length - 2;

    // Make sure we don't overflow the provided response buffer
    if (data_length > *response_len)
    {
        Log.warning(F("Transceive: Response truncated from %d to %d bytes"), data_length, *response_len);
        data_length = *response_len;
    }

    // Copy the actual data (without PCB and CID) to the response buffer
    memcpy(response, &response_buffer[2], data_length);
    *response_len = data_length;

    Log.trace(F("Transceive: Successfully exchanged data with card"));
    return 0x00; // Success
}