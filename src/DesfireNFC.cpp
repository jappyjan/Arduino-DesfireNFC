/**
 * @file DesfireNFC.cpp
 * @brief Implementation of the DesfireNFC library
 */

#include "DesfireNFC.h"
#include "DesfireTypes.h"
#include "ISO7816Constants.h"

// ISO7816 constants
#define ISO7816_CLA_DESFIRE 0x90
#define ISO7816_INS_WRAPPER 0x00
#define ISO7816_SW1_SUCCESS 0x91
#define ISO7816_SW2_SUCCESS 0x00
#define ISO7816_MF_MAX_FRAME 60  // Maximum data size in a single APDU frame

// DESFire constants
#define DF_PICC_MAX_FRAME 40  // Maximum number of frames for a complete command
#define MIFARE_DESFIRE 0x01   // Type of card

// DESFire Commands - REMOVED, using DesfireCommand enum instead
// #define DF_CMD_GET_VERSION            0x60
// #define DF_CMD_GET_ADDITIONAL_FRAME   0xAF
// #define DF_CMD_SELECT_APPLICATION     0x5A

// Crypto modes - REMOVED, using enum instead
// #define DF_CRYPTO_DES        0x00

/**
 * @brief Construct a new DesfireNFC object
 *
 * @param reader Reference to an NFC reader implementation
 */
DesfireNFC::DesfireNFC(NFCReaderInterface& reader) : _reader(reader) {
    _authenticated = false;
    _cardDetected  = false;
    _cryptoMode    = DesfreCryptoMode::DF_CRYPTO_DES;  // Using proper enum reference
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
bool DesfireNFC::getCardUID(uint8_t* uid, uint8_t* uidLength) {
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
        _uidLength    = *uidLength;
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
    uint8_t       response[32];
    uint16_t      responseLen = 0;  // Changed to uint16_t for consistency
    DesfireStatus status;

    // Send the GET_VERSION command
    status = transmit(DesfireCommand::DF_CMD_GET_VERSION, nullptr, 0, response, responseLen);

    return (status == DesfireStatus::DFST_SUCCESS || status == DesfireStatus::DFST_MORE_FRAMES);
}

/**
 * @brief Get version information from the card
 *
 * @param version Pointer to a DESFireCardVersion struct to store version info
 * @return true if the command was successful
 * @return false if the command failed
 */
bool DesfireNFC::getVersion(DESFireCardVersion* version) {
    if (!version) {
        return false;
    }

    uint8_t       response[32];
    uint16_t      responseLen = 0;  // Changed to uint16_t for consistency
    DesfireStatus status;

    // Send the GET_VERSION command
    status = transmit(DesfireCommand::DF_CMD_GET_VERSION, nullptr, 0, response, responseLen);
    if (status != DesfireStatus::DFST_SUCCESS && status != DesfireStatus::DFST_MORE_FRAMES) {
        return false;
    }

    // Parse hardware version info
    if (responseLen >= 7) {
        version->hardwareVendor       = response[0];
        version->hardwareType         = response[1];
        version->hardwareSubtype      = response[2];
        version->hardwareVersionMajor = response[3];
        version->hardwareVersionMinor = response[4];
        version->hardwareStorageSize  = response[5];
        version->hardwareProtocol     = response[6];
    } else {
        return false;
    }

    // If more data is expected, get the second frame with software version info
    if (status == DesfireStatus::DFST_MORE_FRAMES) {
        uint8_t  response2[32];
        uint16_t responseLen2 = 0;  // Changed to uint16_t for consistency

        status = transmit(
            DesfireCommand::DF_CMD_GET_ADDITIONAL_FRAME, nullptr, 0, response2, responseLen2);
        if (status != DesfireStatus::DFST_SUCCESS && status != DesfireStatus::DFST_MORE_FRAMES) {
            return false;
        }

        // Parse software version info
        if (responseLen2 >= 7) {
            version->softwareVendor       = response2[0];
            version->softwareType         = response2[1];
            version->softwareSubtype      = response2[2];
            version->softwareVersionMajor = response2[3];
            version->softwareVersionMinor = response2[4];
            version->softwareStorageSize  = response2[5];
            version->softwareProtocol     = response2[6];
        } else {
            return false;
        }

        // If more data is expected, get the third frame with UID and production info
        if (status == DesfireStatus::DFST_MORE_FRAMES) {
            uint8_t  response3[32];
            uint16_t responseLen3 = 0;  // Changed to uint16_t for consistency

            status = transmit(
                DesfireCommand::DF_CMD_GET_ADDITIONAL_FRAME, nullptr, 0, response3, responseLen3);
            if (status != DesfireStatus::DFST_SUCCESS) {
                return false;
            }

            // Parse UID and production info
            if (responseLen3 >= 10) {  // Check for UID (7 bytes) + at least 3 production bytes
                memcpy(version->uid, response3, 7);

                // Handle production information
                // Check if we have all 5 bytes for batch number
                if (responseLen3 >= 14) {  // 7 (UID) + 5 (batch) + 2 (week/year)
                    memcpy(version->batchNumber, &response3[7], 5);
                    version->productionWeek = response3[12];
                    version->productionYear = response3[13];
                } else {
                    // Handle partial information
                    // Set available batch number bytes
                    memset(version->batchNumber, 0, 5);  // Clear first
                    memcpy(version->batchNumber,
                           &response3[7],
                           responseLen3 - 9);  // Copy what we have

                    // Set production week/year
                    version->productionWeek = response3[responseLen3 - 2];
                    version->productionYear = response3[responseLen3 - 1];
                }
            } else {
                return false;
            }
        }
    }

    return true;
}

/**
 * @brief Transmit a DESFire command and receive the response
 *
 * @param command DESFire command code
 * @param data Pointer to command data
 * @param dataLen Length of command data
 * @param response Buffer to store the response
 * @param responseLen Reference to variable that will hold response length
 * @return DesfireStatus Status code of the operation
 */
DesfireStatus DesfireNFC::transmit(DesfireCommand command,
                                   const uint8_t* data,
                                   uint8_t        dataLen,
                                   uint8_t*       response,
                                   uint16_t&      responseLen) {
    if (!response) {
        return DesfireStatus::DFST_PARAMETER_NULL;
    }

    // Prepare command data: command code + data
    uint8_t commandData[ISO7816Constants::ISO_MAX_DATA_SIZE];
    uint8_t commandLen = 1;  // Start with command code

    // Add command code
    commandData[0] = static_cast<uint8_t>(command);

    // Add command data (if any)
    if (data && dataLen > 0) {
        if (dataLen > ISO7816Constants::ISO_MAX_DATA_SIZE - 1) {
            return DesfireStatus::DFST_BUFFER_OVERFLOW;
        }
        memcpy(&commandData[commandLen], data, dataLen);
        commandLen += dataLen;
    }

    // Build APDU
    uint8_t  apdu[ISO7816Constants::ISO_MAX_APDU_SIZE];
    uint16_t apduLen = buildAPDU(ISO7816Class::ISO_CLA_DESFIRE,
                                 ISO7816Instruction::ISO_INS_DESFIRE_NATIVE,
                                 0,
                                 0,
                                 commandData,
                                 commandLen,
                                 0,
                                 apdu);

    if (apduLen == 0) {
        return DesfireStatus::DFST_BUFFER_OVERFLOW;
    }

    // Transmit APDU and get response
    uint8_t  responseBuffer[ISO7816Constants::ISO_MAX_APDU_SIZE];
    uint16_t responseBufferLen = sizeof(responseBuffer);

    if (!_reader.transceive(apdu, apduLen, responseBuffer, &responseBufferLen)) {
        return DesfireStatus::DFST_COMMUNICATION_ERROR;
    }

    // Parse response
    if (responseBufferLen < ISO7816Constants::ISO_STATUS_LENGTH) {
        return DesfireStatus::DFST_LENGTH_ERROR;
    }

    // Extract status word
    uint16_t statusWord =
        (responseBuffer[responseBufferLen - 2] << 8) | responseBuffer[responseBufferLen - 1];

    // Convert ISO7816 status to DesfireStatus
    DesfireStatus status;
    switch (statusWord) {
        case static_cast<uint16_t>(ISO7816StatusWord::ISO_SW_SUCCESS):
        case static_cast<uint16_t>(ISO7816StatusWord::ISO_SW_SUCCESS_DESFIRE):
            status = DesfireStatus::DFST_SUCCESS;
            break;

        case static_cast<uint16_t>(ISO7816StatusWord::ISO_SW_WRONG_LENGTH):
            status = DesfireStatus::DFST_ISO_WRONG_LENGTH;
            break;

        case static_cast<uint16_t>(ISO7816StatusWord::ISO_SW_SECURITY_NOT_SATISFIED):
            status = DesfireStatus::DFST_ISO_SECURITY_STATUS_ERROR;
            break;

        case static_cast<uint16_t>(ISO7816StatusWord::ISO_SW_AUTH_METHOD_BLOCKED):
            status = DesfireStatus::DFST_ISO_AUTHENTICATION_BLOCKED;
            break;

        case static_cast<uint16_t>(ISO7816StatusWord::ISO_SW_DATA_INVALID):
            status = DesfireStatus::DFST_ISO_DATA_INVALID;
            break;

        case static_cast<uint16_t>(ISO7816StatusWord::ISO_SW_CONDITIONS_NOT_SATISFIED):
            status = DesfireStatus::DFST_ISO_CONDITION_NOT_SATISFIED;
            break;

        case static_cast<uint16_t>(ISO7816StatusWord::ISO_SW_FILE_NOT_FOUND):
            status = DesfireStatus::DFST_ISO_FILE_NOT_FOUND;
            break;

        case static_cast<uint16_t>(ISO7816StatusWord::ISO_SW_WRONG_P1P2):
            status = DesfireStatus::DFST_ISO_WRONG_PARAMS;
            break;

        case static_cast<uint16_t>(ISO7816StatusWord::ISO_SW_INS_NOT_SUPPORTED):
            status = DesfireStatus::DFST_ISO_UNKNOWN_INSTRUCTION;
            break;

        case static_cast<uint16_t>(ISO7816StatusWord::ISO_SW_CLASS_NOT_SUPPORTED):
            status = DesfireStatus::DFST_ISO_WRONG_CLA;
            break;

        default:
            status = DesfireStatus::DFST_LIBRARY_ERROR;
            break;
    }

    // If successful, copy response data to output buffer
    if (status == DesfireStatus::DFST_SUCCESS) {
        responseLen = responseBufferLen - ISO7816Constants::ISO_STATUS_LENGTH;
        if (responseLen > 0) {
            memcpy(response, responseBuffer, responseLen);
        }
    }

    return status;
}

/**
 * @brief Select a DESFire application by its ID
 *
 * @param aid Application ID (3 bytes)
 * @return DesfireStatus Status code of the operation
 */
DesfireStatus DesfireNFC::selectApplication(uint8_t* aid) {
    if (!aid) {
        return DesfireStatus::DFST_PARAMETER_NULL;
    }

    uint8_t  response[32];
    uint16_t responseLen = 0;  // Changed to uint16_t for consistency

    return transmit(DesfireCommand::DF_CMD_SELECT_APPLICATION, aid, 3, response, responseLen);
}

/**
 * @brief Authenticate with the specified key
 *
 * @param keyNo Key number to authenticate with
 * @param key Pointer to the key data
 * @param keySize Size of the key in bytes
 * @return DesfireStatus Status code of the operation
 */
DesfireStatus DesfireNFC::authenticate(uint8_t keyNo, const uint8_t* key, uint8_t keySize) {
    if (!key) {
        return DesfireStatus::DFST_PARAMETER_NULL;
    }

    // Check key size based on crypto mode
    switch (_cryptoMode) {
        case DesfreCryptoMode::DF_CRYPTO_DES:
            if (keySize != 8) {
                return DesfireStatus::DFST_PARAMETER_ERROR;
            }
            break;

        case DesfreCryptoMode::DF_CRYPTO_3K3DES:  // Corrected enum member name
            if (keySize != 16) {
                return DesfireStatus::DFST_PARAMETER_ERROR;
            }
            break;

        case DesfreCryptoMode::DF_CRYPTO_AES:
            if (keySize != 16) {
                return DesfireStatus::DFST_PARAMETER_ERROR;
            }
            break;

        default:
            return DesfireStatus::DFST_PARAMETER_ERROR;
    }

    // Prepare command data
    uint8_t cmdData[2];
    cmdData[0] = keyNo;
    cmdData[1] = static_cast<uint8_t>(_cryptoMode);  // Proper static_cast of enum to uint8_t

    uint8_t  response[32];
    uint16_t responseLen = 0;  // Changed to uint16_t to match method parameter

    // Send authentication command
    DesfireStatus status =
        transmit(DesfireCommand::DF_CMD_AUTHENTICATE, cmdData, 2, response, responseLen);
    if (status != DesfireStatus::DFST_SUCCESS) {
        return status;
    }

    // Store session key
    memcpy(_sessionKey, key, keySize);
    _authenticated = true;

    return DesfireStatus::DFST_SUCCESS;
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
 * @return uint16_t Length of the constructed APDU
 */
uint16_t DesfireNFC::buildAPDU(ISO7816Class       cla,
                               ISO7816Instruction ins,
                               uint8_t            p1,
                               uint8_t            p2,
                               const uint8_t*     data,
                               uint8_t            dataLen,
                               uint8_t            le,
                               uint8_t*           apdu) {
    if (!apdu) {
        return 0;
    }

    uint16_t index = 0;

    // APDU header (CLA, INS, P1, P2)
    apdu[index++] = static_cast<uint8_t>(cla);
    apdu[index++] = static_cast<uint8_t>(ins);
    apdu[index++] = p1;
    apdu[index++] = p2;

    // Command data (if any)
    if (dataLen > 0) {
        if (dataLen > ISO7816Constants::ISO_MAX_DATA_SIZE) {
            dataLen = ISO7816Constants::ISO_MAX_DATA_SIZE;  // Truncate if too large
        }
        apdu[index++] = dataLen;  // Lc field
        memcpy(&apdu[index], data, dataLen);
        index += dataLen;
    }

    // Expected response length (if any)
    if (le > 0) {
        apdu[index++] = le;  // Le field
    }

    return index;
}