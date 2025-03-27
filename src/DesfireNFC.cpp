/**
 * @file DesfireNFC.cpp
 * @brief Implementation of the DesfireNFC library
 */

#include "DesfireNFC.h"

// ISO7816 constants
#define ISO7816_CLA_DESFIRE  0x90
#define ISO7816_INS_WRAPPER  0x00
#define ISO7816_SW1_SUCCESS  0x91
#define ISO7816_SW2_SUCCESS  0x00
#define ISO7816_MF_MAX_FRAME 60  // Maximum data size in a single APDU frame

// DESFire constants
#define DF_PICC_MAX_FRAMES   40  // Maximum number of frames for a complete command
#define MIFARE_DESFIRE       0x01  // Type of card

// DESFire Commands
#define DF_CMD_GET_VERSION            0x60
#define DF_CMD_GET_ADDITIONAL_FRAME   0xAF
#define DF_CMD_SELECT_APPLICATION     0x5A

// Crypto modes
#define DF_CRYPTO_DES        0x00

/**
 * @brief Construct a new DesfireNFC object
 * 
 * @param reader Reference to an NFC reader implementation
 */
DesfireNFC::DesfireNFC(NFCReaderInterface &reader) : _reader(reader) {
    _authenticated = false;
    _cardDetected = false;
    _cryptoMode = DF_CRYPTO_DES; // Default to DES
    memset(_sessionKey, 0, sizeof(_sessionKey));
    memset(_uid, 0, sizeof(_uid));
    _uidLength = 0;
}

/**
 * @brief Initialize the NFC hardware
 * 
 * @return true if initialization was successful
 * @return false if initialization failed
 */
bool DesfireNFC::initialize() {
    // Initialize the NFC reader
    if (!_reader.begin()) {
        return false;
    }
    
    // Get firmware version to check if reader is responding
    if (!_reader.getFirmwareVersion()) {
        return false;
    }
    
    // Configure the reader for card communication
    return _reader.configure();
}

/**
 * @brief Detect if a DESFire card is present in the field
 * 
 * @return true if a card was detected
 * @return false if no card was detected
 */
bool DesfireNFC::detectCard() {
    // Store UID in member variables for later use
    _cardDetected = _reader.detectCard(_uid, &_uidLength);
    
    // DESFire cards have 7-byte UIDs
    if (_cardDetected && _uidLength != 7) {
        _cardDetected = false;
        return false;
    }
    
    return _cardDetected;
}

/**
 * @brief Get the UID of the currently selected card
 * 
 * @param uid Buffer to store the UID (should be at least 7 bytes)
 * @param uidLength Pointer to variable that will store the UID length
 * @return bool true if successful, false if unsuccessful
 */
bool DesfireNFC::getCardUID(uint8_t *uid, uint8_t *uidLength) {
    if (uid == nullptr || uidLength == nullptr) {
        return false;
    }
    
    // If we have a card detected, use the stored UID
    if (_cardDetected) {
        memcpy(uid, _uid, _uidLength);
        *uidLength = _uidLength;
        return true;
    }
    
    // Otherwise try to detect the card again
    if (_reader.detectCard(uid, uidLength)) {
        // Store UID for later use
        memcpy(_uid, uid, *uidLength);
        _uidLength = *uidLength;
        _cardDetected = true;
        
        return true;
    }
    
    return false;
}

/**
 * @brief Get version information from the card
 * 
 * @return true if the command was successful
 * @return false if the command failed
 */
bool DesfireNFC::getVersion() {
    // Simplified version just to check if command works
    uint8_t response[32];
    uint8_t responseLen = 0;
    DesfireStatus status;
    
    // Send the GET_VERSION command
    status = transmit(DF_CMD_GET_VERSION, nullptr, 0, response, responseLen);
    
    return (status == DFST_SUCCESS || status == DFST_MORE_FRAMES);
}

/**
 * @brief Get version information from the card
 * 
 * @param version Pointer to a DESFireCardVersion struct to store version info
 * @return true if the command was successful
 * @return false if the command failed
 */
bool DesfireNFC::getVersion(DESFireCardVersion *version) {
    if (!version) {
        return false;
    }
    
    uint8_t response[32];
    uint8_t responseLen = 0;
    DesfireStatus status;
    
    // Send the GET_VERSION command
    status = transmit(DF_CMD_GET_VERSION, nullptr, 0, response, responseLen);
    if (status != DFST_SUCCESS && status != DFST_MORE_FRAMES) {
        return false;
    }
    
    // Parse hardware version info
    if (responseLen >= 7) {
        version->hardwareVendor = response[0];
        version->hardwareType = response[1];
        version->hardwareSubtype = response[2];
        version->hardwareVersionMajor = response[3];
        version->hardwareVersionMinor = response[4];
        version->hardwareStorageSize = response[5];
        version->hardwareProtocol = response[6];
    } else {
        return false;
    }
    
    // If more data is expected, get the second frame with software version info
    if (status == DFST_MORE_FRAMES) {
        uint8_t response2[32];
        uint8_t responseLen2 = 0;
        
        status = transmit(DF_CMD_GET_ADDITIONAL_FRAME, nullptr, 0, response2, responseLen2);
        if (status != DFST_SUCCESS && status != DFST_MORE_FRAMES) {
            return false;
        }
        
        // Parse software version info
        if (responseLen2 >= 7) {
            version->softwareVendor = response2[0];
            version->softwareType = response2[1];
            version->softwareSubtype = response2[2];
            version->softwareVersionMajor = response2[3];
            version->softwareVersionMinor = response2[4];
            version->softwareStorageSize = response2[5];
            version->softwareProtocol = response2[6];
        } else {
            return false;
        }
        
        // If more data is expected, get the third frame with UID and production info
        if (status == DFST_MORE_FRAMES) {
            uint8_t response3[32];
            uint8_t responseLen3 = 0;
            
            status = transmit(DF_CMD_GET_ADDITIONAL_FRAME, nullptr, 0, response3, responseLen3);
            if (status != DFST_SUCCESS) {
                return false;
            }
            
            // Parse UID and production info
            if (responseLen3 >= 14) {
                memcpy(version->uid, response3, 7);
                memcpy(version->batchNumber, response3 + 7, 5);
                version->productionWeek = response3[12];
                version->productionYear = response3[13];
            }
        }
    }
    
    return true;
}

/**
 * @brief Select a DESFire application by its ID
 * 
 * @param aid Application ID (3 bytes)
 * @return DesfireStatus Status code of the operation
 */
DesfireStatus DesfireNFC::selectApplication(uint8_t *aid) {
    if (!aid) {
        return DFST_PARAMETER_NULL;
    }
    
    uint8_t response[2];
    uint8_t responseLen = 0;
    
    return transmit(DF_CMD_SELECT_APPLICATION, aid, 3, response, responseLen);
}

/**
 * @brief Authenticate with the specified key
 * 
 * @param keyNo Key number to authenticate with
 * @param key Pointer to the key data
 * @param keySize Size of the key in bytes
 * @return DesfireStatus Status code of the operation
 */
DesfireStatus DesfireNFC::authenticate(uint8_t keyNo, const uint8_t *key, uint8_t keySize) {
    if (!key) {
        return DFST_PARAMETER_NULL;
    }
    
    // TODO: Implement full authentication with challenge-response
    // This is a placeholder, authentication will be implemented in depth
    // in the Authentication & Cryptography section
    
    _authenticated = false;
    return DFST_AUTHENTICATION_ERROR;
}

/**
 * @brief Transmit a DESFire command using ISO7816-4 APDU wrapping
 * 
 * @param command DESFire command code
 * @param data Pointer to command data
 * @param dataLen Length of command data
 * @param response Buffer to store the response
 * @param responseLen Reference to variable that will hold response length
 * @return DesfireStatus Status code of the operation
 */
DesfireStatus DesfireNFC::transmit(uint8_t command, const uint8_t *data, uint8_t dataLen, 
                                  uint8_t *response, uint8_t &responseLen) {
    if (dataLen > 0 && !data) {
        return DFST_PARAMETER_NULL;
    }
    
    if (!response) {
        return DFST_PARAMETER_NULL;
    }
    
    // Prepare the command data with the command code at the beginning
    uint8_t cmdData[ISO7816_MF_MAX_FRAME];
    uint8_t cmdDataLen = 0;
    
    cmdData[cmdDataLen++] = command;
    
    // Copy the command data if any
    if (dataLen > 0) {
        if (dataLen > ISO7816_MF_MAX_FRAME - 1) {
            dataLen = ISO7816_MF_MAX_FRAME - 1;  // Truncate if too large
        }
        memcpy(&cmdData[cmdDataLen], data, dataLen);
        cmdDataLen += dataLen;
    }
    
    // Build the ISO7816-4 APDU
    uint8_t apdu[ISO7816_MF_MAX_FRAME + 5];  // Add 5 bytes for APDU header
    uint8_t apduLen = buildAPDU(ISO7816_CLA_DESFIRE, ISO7816_INS_WRAPPER,
                                0x00, 0x00, cmdData, cmdDataLen, 0x00, apdu);
    
    // Send the command to the card and get the response
    uint8_t rxBuffer[ISO7816_MF_MAX_FRAME];
    uint8_t rxLen = sizeof(rxBuffer);
    
    if (!_reader.transceive(apdu, apduLen, rxBuffer, &rxLen)) {
        return DFST_PARAMETER_NULL; // Error in communication
    }
    
    // Check the response length
    if (rxLen < 2) {
        return DFST_PARAMETER_NULL;
    }
    
    // Check the status code in the response (last 2 bytes)
    uint8_t sw1 = rxBuffer[rxLen - 2];
    uint8_t sw2 = rxBuffer[rxLen - 1];
    
    // Copy the actual data response (excluding the status bytes)
    responseLen = rxLen - 2;
    memcpy(response, rxBuffer, responseLen);
    
    // Check if it's a DESFire status code
    if (sw1 == ISO7816_SW1_SUCCESS) {
        // Map sw2 to one of our enum values that we know about
        switch(sw2) {
            case 0x00: return DFST_SUCCESS;
            case 0xAF: return DFST_MORE_FRAMES;
            case 0xAE: return DFST_AUTHENTICATION_ERROR;
            case 0xF0: return DFST_FILE_NOT_FOUND;
            case 0x0C: return DFST_NO_CHANGES;
            case 0x0E: return DFST_OUT_OF_EEPROM;
            case 0x1C: return DFST_ILLEGAL_COMMAND;
            case 0x1E: return DFST_INTEGRITY_ERROR;
            case 0x1F: return DFST_PARAMETER_ERROR;
            case 0x40: return DFST_NO_SUCH_KEY;
            case 0x7E: return DFST_LENGTH_ERROR;
            case 0x9D: return DFST_PERMISSION_DENIED;
            case 0xA0: return DFST_APPLICATION_NOT_FOUND;
            case 0xA1: return DFST_APPLICATION_INTEGRITY_ERROR;
            case 0xBE: return DFST_BOUNDARY_ERROR;
            case 0xC1: return DFST_PICC_INTEGRITY_ERROR;
            case 0xCA: return DFST_COMMAND_ABORTED;
            case 0xCD: return DFST_CARD_INTEGRITY_ERROR;
            case 0xDE: return DFST_DUPLICATE_ERROR;
            case 0xEE: return DFST_EEPROM_ERROR;
            case 0xF1: return DFST_FILE_INTEGRITY_ERROR;
            default:
                // Return a default error for unknown status codes
                return DFST_LIBRARY_ERROR;
        }
    }
    
    // Return generic error if not a recognized status
    return DFST_LIBRARY_ERROR;
}

/**
 * @brief Build an ISO7816-4 APDU
 * 
 * @param cla Class byte
 * @param ins Instruction byte
 * @param p1 Parameter 1
 * @param p2 Parameter 2
 * @param data Command data
 * @param dataLen Length of command data
 * @param le Expected response length (0 for none)
 * @param apdu Buffer to store the APDU
 * @return uint8_t Length of the constructed APDU
 */
uint8_t DesfireNFC::buildAPDU(uint8_t cla, uint8_t ins, uint8_t p1, uint8_t p2,
                              const uint8_t *data, uint8_t dataLen, uint8_t le,
                              uint8_t *apdu) {
    uint8_t index = 0;
    
    // APDU header
    apdu[index++] = cla;
    apdu[index++] = ins;
    apdu[index++] = p1;
    apdu[index++] = p2;
    
    // Command data
    if (dataLen > 0) {
        apdu[index++] = dataLen;  // Lc field
        memcpy(&apdu[index], data, dataLen);
        index += dataLen;
    }
    
    // Expected response length
    if (le > 0) {
        apdu[index++] = le;  // Le field
    }
    
    return index;
} 