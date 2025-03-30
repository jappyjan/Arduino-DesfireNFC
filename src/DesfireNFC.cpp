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
 * @param debugLevel Initial debug level (default: no debug output)
 */
DesfireNFC::DesfireNFC(NFCReaderInterface& reader, DebugLevel debugLevel) : _reader(reader) {
    _authenticated = false;
    _cardDetected  = false;
    _cryptoMode    = DesfreCryptoMode::DF_CRYPTO_DES;  // Using proper enum reference
    _debugLevel    = debugLevel;
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
 * @return DesfireStatus Status code of the operation
 */
DesfireStatus DesfireNFC::getVersion() {
    uint8_t       response[32];
    uint16_t      responseLen = 0;  // Changed to uint16_t for consistency
    DesfireStatus status;

    debugPrintInfo("Sending GET_VERSION command");

    // Send the GET_VERSION command
    status = transmit(DesfireCommand::DF_CMD_GET_VERSION, nullptr, 0, response, responseLen);

    if (status != DesfireStatus::DFST_SUCCESS && status != DesfireStatus::DFST_MORE_FRAMES) {
        debugPrintError("GET_VERSION command failed", status);
        return status;
    }

    debugPrintInfo("GET_VERSION command returned success or more frames");
    debugPrintHex("Response data: ", response, responseLen);

    return status;
}

/**
 * @brief Get version information from the card
 *
 * @param version Pointer to a DESFireCardVersion struct to store version info
 * @return DesfireStatus Status code of the operation
 */
DesfireStatus DesfireNFC::getVersion(DESFireCardVersion* version) {
    if (!version) {
        debugPrintError("Version parameter is NULL", DesfireStatus::DFST_PARAMETER_NULL);
        return DesfireStatus::DFST_PARAMETER_NULL;
    }

    // Initialize version struct with zeros
    memset(version, 0, sizeof(DESFireCardVersion));

    uint8_t       response[64];  // Increased buffer size to handle all responses
    uint16_t      responseLen = 0;
    DesfireStatus status;

    debugPrintInfo("Sending GET_VERSION command (Frame 1)");

    // Frame 1: Hardware version info
    status = transmit(DesfireCommand::DF_CMD_GET_VERSION, nullptr, 0, response, responseLen);
    if (status != DesfireStatus::DFST_SUCCESS && status != DesfireStatus::DFST_MORE_FRAMES) {
        // Return specific error status for debugging
        debugPrintError("GET_VERSION Frame 1 failed", status);
        return status;
    }

    debugPrintHex("Frame 1 response: ", response, responseLen);

    // Parse hardware version info
    if (responseLen >= 7) {
        version->hardwareVendor       = response[0];
        version->hardwareType         = response[1];
        version->hardwareSubtype      = response[2];
        version->hardwareVersionMajor = response[3];
        version->hardwareVersionMinor = response[4];
        version->hardwareStorageSize  = response[5];
        version->hardwareProtocol     = response[6];

        debugPrintInfo("Hardware info parsed successfully");
    } else {
        // Received a partial hardware version info
        debugPrintError("Hardware info response length error", DesfireStatus::DFST_LENGTH_ERROR);
        return DesfireStatus::DFST_LENGTH_ERROR;
    }

    // Frame 2: Software version info
    if (status == DesfireStatus::DFST_MORE_FRAMES) {
        uint8_t  response2[64];
        uint16_t responseLen2 = 0;

        debugPrintInfo("Sending GET_ADDITIONAL_FRAME command (Frame 2)");
        status = transmit(
            DesfireCommand::DF_CMD_GET_ADDITIONAL_FRAME, nullptr, 0, response2, responseLen2);
        if (status != DesfireStatus::DFST_SUCCESS && status != DesfireStatus::DFST_MORE_FRAMES) {
            // We have hardware info but failed to get software info
            debugPrintError("GET_VERSION Frame 2 failed", status);
            return status;
        }

        debugPrintHex("Frame 2 response: ", response2, responseLen2);

        // Parse software version info
        if (responseLen2 >= 7) {
            version->softwareVendor       = response2[0];
            version->softwareType         = response2[1];
            version->softwareSubtype      = response2[2];
            version->softwareVersionMajor = response2[3];
            version->softwareVersionMinor = response2[4];
            version->softwareStorageSize  = response2[5];
            version->softwareProtocol     = response2[6];

            debugPrintInfo("Software info parsed successfully");
        } else {
            // Received a partial software version info
            debugPrintError("Software info response length error",
                            DesfireStatus::DFST_LENGTH_ERROR);
            return DesfireStatus::DFST_LENGTH_ERROR;
        }

        // Frame 3: UID and production info
        if (status == DesfireStatus::DFST_MORE_FRAMES) {
            uint8_t  response3[64];
            uint16_t responseLen3 = 0;

            debugPrintInfo("Sending GET_ADDITIONAL_FRAME command (Frame 3)");
            status = transmit(
                DesfireCommand::DF_CMD_GET_ADDITIONAL_FRAME, nullptr, 0, response3, responseLen3);
            if (status != DesfireStatus::DFST_SUCCESS) {
                // We have hardware and software info but failed to get production info
                debugPrintError("GET_VERSION Frame 3 failed", status);
                return status;
            }

            debugPrintHex("Frame 3 response: ", response3, responseLen3);

            // Parse UID and production info - minimum we need is the 7-byte UID
            if (responseLen3 >= 7) {
                memcpy(version->uid, response3, 7);
                debugPrintInfo("UID info parsed successfully");

                // Parse production information if available
                if (responseLen3 >= 14) {  // Complete 7 (UID) + 5 (batch) + 2 (week/year)
                    memcpy(version->batchNumber, &response3[7], 5);
                    version->productionWeek = response3[12];
                    version->productionYear = response3[13];
                    debugPrintInfo("Production info parsed successfully");
                } else if (responseLen3 > 7) {
                    // Partial production info available
                    int batchBytes = responseLen3 - 9;
                    if (batchBytes > 0) {
                        // Don't overflow if we have fewer bytes
                        batchBytes = min(batchBytes, 5);
                        memcpy(version->batchNumber, &response3[7], batchBytes);
                    }

                    // Get week/year if available
                    if (responseLen3 >= 9) {
                        version->productionWeek = response3[responseLen3 - 2];
                        version->productionYear = response3[responseLen3 - 1];
                        debugPrintInfo("Partial production info parsed");
                    }
                }
            } else {
                // UID information is incomplete
                debugPrintError("UID info response length error", DesfireStatus::DFST_LENGTH_ERROR);
                return DesfireStatus::DFST_LENGTH_ERROR;
            }
        }
    }

    debugPrintInfo("getVersion completed successfully");
    return status;
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
        debugPrintError("Null response buffer provided", DesfireStatus::DFST_PARAMETER_NULL);
        return DesfireStatus::DFST_PARAMETER_NULL;
    }

    // Debug output of command and parameters
    if (_debugLevel >= DEBUG_VERBOSE) {
        char cmdBuffer[100];
        snprintf(cmdBuffer,
                 sizeof(cmdBuffer),
                 "Transmitting command: 0x%02X, Data length: %u",
                 static_cast<uint8_t>(command),
                 dataLen);
        debugPrintVerbose(cmdBuffer);

        if (data && dataLen > 0) {
            debugPrintHex("Command data: ", data, dataLen);
        }
    }

    // Fast path optimization - reuse the buffer we already have as a member
    // This avoids stack allocations for repeatedly used buffers
    uint8_t& commandLen = _apduBuffer[0];  // Temporarily use first byte to store length
    commandLen          = 1;               // Start with command code

    // Add command code
    _apduBuffer[1] = static_cast<uint8_t>(command);

    // Add command data (if any)
    if (data && dataLen > 0) {
        if (dataLen > ISO7816Constants::ISO_MAX_DATA_SIZE - 1) {
            debugPrintError("Command data too large", DesfireStatus::DFST_BUFFER_OVERFLOW);
            return DesfireStatus::DFST_BUFFER_OVERFLOW;
        }
        memcpy(&_apduBuffer[2], data, dataLen);
        commandLen += dataLen;
    }

    // Build APDU directly into the response buffer area
    uint16_t apduLen =
        buildAPDU(ISO7816Class::ISO_CLA_DESFIRE,
                  ISO7816Instruction::ISO_INS_DESFIRE_NATIVE,
                  0,
                  0,
                  &_apduBuffer[1],  // Skip the length byte we used
                  commandLen,
                  0,
                  _apduBuffer);  // Reuse the buffer since we no longer need the command data

    if (apduLen == 0) {
        debugPrintError("APDU construction failed", DesfireStatus::DFST_BUFFER_OVERFLOW);
        return DesfireStatus::DFST_BUFFER_OVERFLOW;
    }

    // Debug output of full APDU before sending
    debugPrintHex("APDU sent: ", _apduBuffer, apduLen);

    // Use response buffer for receiving the data
    uint16_t responseBufferLen = ISO7816Constants::ISO_MAX_APDU_SIZE;

    // Attempt transceive multiple times with detailed error reporting
    bool      transceiveSuccess = false;
    const int maxRetries        = 3;

    for (int retry = 0; retry < maxRetries; retry++) {
        if (retry > 0) {
            char retryMsg[50];
            snprintf(retryMsg,
                     sizeof(retryMsg),
                     "Retrying transceive attempt %d of %d",
                     retry + 1,
                     maxRetries);
            debugPrintInfo(retryMsg);
            delay(50 * retry);  // Increasing delay between retries
        }

        transceiveSuccess =
            _reader.transceive(_apduBuffer, apduLen, _responseBuffer, &responseBufferLen);

        if (transceiveSuccess) {
            break;
        }

        debugPrintError("Transceive attempt failed", DesfireStatus::DFST_COMMUNICATION_ERROR);
    }

    if (!transceiveSuccess) {
        debugPrintError("All transceive attempts failed", DesfireStatus::DFST_COMMUNICATION_ERROR);
        return DesfireStatus::DFST_COMMUNICATION_ERROR;
    }

    // Debug output of raw response
    debugPrintHex("Raw response: ", _responseBuffer, responseBufferLen);

    // Parse response
    if (responseBufferLen < ISO7816Constants::ISO_STATUS_LENGTH) {
        debugPrintError("Response too short to parse status word",
                        DesfireStatus::DFST_LENGTH_ERROR);
        return DesfireStatus::DFST_LENGTH_ERROR;
    }

    // Extract status word
    uint16_t statusWord =
        (_responseBuffer[responseBufferLen - 2] << 8) | _responseBuffer[responseBufferLen - 1];

    // Debug output of status word
    if (_debugLevel >= DEBUG_INFO) {
        char statusMsg[50];
        snprintf(statusMsg, sizeof(statusMsg), "Status word: 0x%04X", statusWord);
        debugPrintInfo(statusMsg);
    }

    // Optimize status code conversion with fast-path for common statuses
    DesfireStatus status;

    // Check most common status codes first for fast-path
    if (statusWord == static_cast<uint16_t>(ISO7816StatusWord::ISO_SW_SUCCESS) ||
        statusWord == static_cast<uint16_t>(ISO7816StatusWord::ISO_SW_SUCCESS_DESFIRE)) {
        status = DesfireStatus::DFST_SUCCESS;
    }
    // Check if more frames are available (second most common case)
    else if (statusWord == 0x91AF) {
        status = DesfireStatus::DFST_MORE_FRAMES;
    }
    // Handle all other status codes
    else {
        switch (statusWord) {
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
                if (_debugLevel >= DEBUG_INFO) {
                    char unknownStatus[50];
                    snprintf(unknownStatus,
                             sizeof(unknownStatus),
                             "Unknown status word: 0x%04X",
                             statusWord);
                    debugPrintInfo(unknownStatus);
                }
                status = DesfireStatus::DFST_LIBRARY_ERROR;
                break;
        }
    }

    // If successful or need more frames, copy response data to output buffer
    if (status == DesfireStatus::DFST_SUCCESS || status == DesfireStatus::DFST_MORE_FRAMES) {
        // Calculate response data length (excluding status word)
        uint16_t dataLen = (responseBufferLen > ISO7816Constants::ISO_STATUS_LENGTH)
                               ? responseBufferLen - ISO7816Constants::ISO_STATUS_LENGTH
                               : 0;

        // Copy response data to output buffer
        if (dataLen > 0) {
            memcpy(response, _responseBuffer, dataLen);
            responseLen = dataLen;

            if (_debugLevel >= DEBUG_INFO) {
                char dataLenMsg[50];
                snprintf(
                    dataLenMsg, sizeof(dataLenMsg), "Copied %u bytes to response buffer", dataLen);
                debugPrintInfo(dataLenMsg);
            }
        } else {
            responseLen = 0;
            debugPrintInfo("No data bytes in response (only status word)");
        }
    } else {
        responseLen = 0;
        debugPrintError("Command failed, no data copied to response buffer", status);
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

/**
 * @brief Handle multi-frame data transfer for large data payloads
 *
 * @param command Initial command to send
 * @param data Command data
 * @param dataLen Length of command data
 * @param response Buffer to store the response
 * @param responseLen Reference to variable that will hold response length
 * @param maxResponseLen Maximum size of the response buffer
 * @return DesfireStatus Status code of the operation
 */
DesfireStatus DesfireNFC::handleMultiFrameTransfer(DesfireCommand command,
                                                   const uint8_t* data,
                                                   uint8_t        dataLen,
                                                   uint8_t*       response,
                                                   uint16_t&      responseLen,
                                                   uint16_t       maxResponseLen) {
    if (!response) {
        return DesfireStatus::DFST_PARAMETER_NULL;
    }

    // Send initial command
    DesfireStatus status = transmit(command, data, dataLen, response, responseLen);

    // If initial response is not successful and doesn't need more frames, return immediately
    if (status != DesfireStatus::DFST_SUCCESS && status != DesfireStatus::DFST_MORE_FRAMES) {
        return status;
    }

    // Current position in the output buffer
    uint16_t currentPos = responseLen;

    // Handle multi-frame response using a more efficient approach
    while (status == DesfireStatus::DFST_MORE_FRAMES) {
        // Check if we have enough space in the output buffer
        if (currentPos >= maxResponseLen) {
            return DesfireStatus::DFST_BUFFER_TOO_SMALL;
        }

        // Use remaining space in the output buffer directly
        uint16_t tempResponseLen = 0;

        // Get additional frame directly into the output buffer at the current position
        status = transmit(DesfireCommand::DF_CMD_GET_ADDITIONAL_FRAME,
                          nullptr,
                          0,
                          response + currentPos,
                          tempResponseLen);

        // Update current position
        currentPos += tempResponseLen;
    }

    // Update total response length
    responseLen = currentPos;

    return status;
}

/**
 * @brief Format the PICC (card)
 *
 * @return DesfireStatus Status code of the operation
 */
DesfireStatus DesfireNFC::formatPICC() {
    uint8_t  response[32];
    uint16_t responseLen = 0;

    // Send FORMAT_PICC command
    return transmit(DesfireCommand::DF_CMD_FORMAT_PICC, nullptr, 0, response, responseLen);
}

/**
 * @brief Get free memory available on the card
 *
 * @param freeMemory Pointer to store the free memory in bytes
 * @return DesfireStatus Status code of the operation
 */
DesfireStatus DesfireNFC::getFreeMem(uint32_t* freeMemory) {
    if (!freeMemory) {
        return DesfireStatus::DFST_PARAMETER_NULL;
    }

    uint8_t  response[16];
    uint16_t responseLen = 0;

    DesfireStatus status =
        transmit(DesfireCommand::DF_CMD_GET_FREE_MEMORY, nullptr, 0, response, responseLen);
    if (status != DesfireStatus::DFST_SUCCESS) {
        return status;
    }

    // Check if we have enough data in the response (at least 3 bytes for memory size)
    if (responseLen >= 3) {
        *freeMemory = response[0] | (response[1] << 8) | (response[2] << 16);
        return DesfireStatus::DFST_SUCCESS;
    } else {
        return DesfireStatus::DFST_LENGTH_ERROR;
    }
}

/**
 * @brief Get application IDs present on card
 *
 * @param appIds Buffer to store the application IDs (3 bytes each)
 * @param maxCount Maximum number of application IDs to retrieve
 * @param count Reference to variable that will store the actual count
 * @return DesfireStatus Status code of the operation
 */
DesfireStatus DesfireNFC::getApplicationIDs(uint8_t* appIds, uint8_t maxCount, uint8_t& count) {
    if (!appIds || maxCount == 0) {
        return DesfireStatus::DFST_PARAMETER_NULL;
    }

    uint8_t  response[96];  // Allow space for multiple application IDs
    uint16_t responseLen = 0;

    DesfireStatus status =
        transmit(DesfireCommand::DF_CMD_GET_APPLICATION_IDS, nullptr, 0, response, responseLen);
    if (status != DesfireStatus::DFST_SUCCESS) {
        count = 0;
        return status;
    }

    // Each application ID is 3 bytes
    count = responseLen / 3;
    if (count > maxCount) {
        count = maxCount;
    }

    // Copy application IDs to the output buffer
    for (uint8_t i = 0; i < count; i++) {
        memcpy(&appIds[i * 3], &response[i * 3], 3);
    }

    return DesfireStatus::DFST_SUCCESS;
}

/**
 * @brief Create a new application on the card
 *
 * @param aid Application ID (3 bytes)
 * @param keySettings Key settings for the application
 * @param numKeys Number of keys in the application (1-14)
 * @return DesfireStatus Status code of the operation
 */
DesfireStatus DesfireNFC::createApplication(const uint8_t* aid,
                                            uint8_t        keySettings,
                                            uint8_t        numKeys) {
    if (!aid) {
        return DesfireStatus::DFST_PARAMETER_NULL;
    }

    if (numKeys < 1 || numKeys > 14) {
        return DesfireStatus::DFST_PARAMETER_ERROR;
    }

    uint8_t cmdData[5];
    memcpy(cmdData, aid, 3);
    cmdData[3] = keySettings;
    cmdData[4] = numKeys;

    uint8_t  response[32];
    uint16_t responseLen = 0;

    return transmit(DesfireCommand::DF_CMD_CREATE_APPLICATION, cmdData, 5, response, responseLen);
}

/**
 * @brief Delete an application from the card
 *
 * @param aid Application ID (3 bytes)
 * @return DesfireStatus Status code of the operation
 */
DesfireStatus DesfireNFC::deleteApplication(const uint8_t* aid) {
    if (!aid) {
        return DesfireStatus::DFST_PARAMETER_NULL;
    }

    uint8_t  response[32];
    uint16_t responseLen = 0;

    return transmit(DesfireCommand::DF_CMD_DELETE_APPLICATION, aid, 3, response, responseLen);
}

/**
 * @brief Change key settings for the current application
 *
 * @param keySettings New key settings
 * @return DesfireStatus Status code of the operation
 */
DesfireStatus DesfireNFC::changeKeySettings(uint8_t keySettings) {
    uint8_t  response[32];
    uint16_t responseLen = 0;

    return transmit(
        DesfireCommand::DF_CMD_CHANGE_KEY_SETTINGS, &keySettings, 1, response, responseLen);
}

/**
 * @brief Get key settings for the current application
 *
 * @param keySettings Reference to variable that will store the key settings
 * @param maxKeyNo Reference to variable that will store the maximum key number
 * @return DesfireStatus Status code of the operation
 */
DesfireStatus DesfireNFC::getKeySettings(uint8_t& keySettings, uint8_t& maxKeyNo) {
    uint8_t  response[32];
    uint16_t responseLen = 0;

    DesfireStatus status =
        transmit(DesfireCommand::DF_CMD_CHANGE_KEY_SETTINGS, nullptr, 0, response, responseLen);
    if (status != DesfireStatus::DFST_SUCCESS) {
        return status;
    }

    // Response contains key settings and max key number
    if (responseLen >= 2) {
        keySettings = response[0];
        maxKeyNo    = response[1];
        return DesfireStatus::DFST_SUCCESS;
    } else {
        return DesfireStatus::DFST_LENGTH_ERROR;
    }
}

/**
 * @brief Get file IDs in the current application
 *
 * @param fileIds Buffer to store the file IDs
 * @param maxCount Maximum number of file IDs to retrieve
 * @param count Reference to variable that will store the actual count
 * @return DesfireStatus Status code of the operation
 */
DesfireStatus DesfireNFC::getFileIDs(uint8_t* fileIds, uint8_t maxCount, uint8_t& count) {
    if (!fileIds || maxCount == 0) {
        return DesfireStatus::DFST_PARAMETER_NULL;
    }

    uint8_t  response[32];  // Allow space for multiple file IDs
    uint16_t responseLen = 0;

    DesfireStatus status =
        transmit(DesfireCommand::DF_CMD_GET_FILE_IDS, nullptr, 0, response, responseLen);
    if (status != DesfireStatus::DFST_SUCCESS) {
        count = 0;
        return status;
    }

    // Each file ID is 1 byte
    count = responseLen;
    if (count > maxCount) {
        count = maxCount;
    }

    // Copy file IDs to the output buffer
    memcpy(fileIds, response, count);

    return DesfireStatus::DFST_SUCCESS;
}

/**
 * @brief Create a standard data file
 *
 * @param fileNo File number
 * @param commMode Communication mode
 * @param accessRights Access rights
 * @param fileSize File size in bytes
 * @return DesfireStatus Status code of the operation
 */
DesfireStatus DesfireNFC::createStdDataFile(uint8_t                 fileNo,
                                            DesfreCommunicationMode commMode,
                                            uint16_t                accessRights,
                                            uint32_t                fileSize) {
    uint8_t cmdData[7];
    cmdData[0] = fileNo;
    cmdData[1] = static_cast<uint8_t>(commMode);
    // Access rights (2 bytes, little-endian)
    cmdData[2] = accessRights & 0xFF;
    cmdData[3] = (accessRights >> 8) & 0xFF;
    // File size (3 bytes, little-endian)
    cmdData[4] = fileSize & 0xFF;
    cmdData[5] = (fileSize >> 8) & 0xFF;
    cmdData[6] = (fileSize >> 16) & 0xFF;

    uint8_t  response[32];
    uint16_t responseLen = 0;

    return transmit(DesfireCommand::DF_CMD_CREATE_STANDARD_FILE, cmdData, 7, response, responseLen);
}

/**
 * @brief Read data from a standard or backup file
 *
 * @param fileNo File number
 * @param offset Offset in bytes
 * @param length Length of data to read
 * @param data Buffer to store the read data
 * @param bytesRead Reference to variable that will store the actual bytes read
 * @return DesfireStatus Status code of the operation
 */
DesfireStatus DesfireNFC::readData(uint8_t   fileNo,
                                   uint32_t  offset,
                                   uint32_t  length,
                                   uint8_t*  data,
                                   uint32_t& bytesRead) {
    if (!data) {
        return DesfireStatus::DFST_PARAMETER_NULL;
    }

    uint8_t cmdData[7];
    cmdData[0] = fileNo;
    // Offset (3 bytes, little-endian)
    cmdData[1] = offset & 0xFF;
    cmdData[2] = (offset >> 8) & 0xFF;
    cmdData[3] = (offset >> 16) & 0xFF;
    // Length (3 bytes, little-endian)
    cmdData[4] = length & 0xFF;
    cmdData[5] = (length >> 8) & 0xFF;
    cmdData[6] = (length >> 16) & 0xFF;

    uint8_t  response[ISO7816Constants::ISO_MAX_DATA_SIZE * 4];  // Increased buffer for multi-frame
    uint16_t responseLen = 0;

    // Use multi-frame transfer for potentially large data
    DesfireStatus status = handleMultiFrameTransfer(DesfireCommand::DF_CMD_READ_DATA,
                                                    cmdData,
                                                    7,
                                                    response,
                                                    responseLen,
                                                    ISO7816Constants::ISO_MAX_DATA_SIZE * 4);

    if (status != DesfireStatus::DFST_SUCCESS) {
        bytesRead = 0;
        return status;
    }

    // Make sure we don't exceed the provided buffer size
    bytesRead = (responseLen > length) ? length : responseLen;
    memcpy(data, response, bytesRead);

    return DesfireStatus::DFST_SUCCESS;
}

/**
 * @brief Write data to a standard or backup file
 *
 * @param fileNo File number
 * @param offset Offset in bytes
 * @param length Length of data to write
 * @param data Data to write
 * @return DesfireStatus Status code of the operation
 */
DesfireStatus DesfireNFC::writeData(uint8_t        fileNo,
                                    uint32_t       offset,
                                    uint32_t       length,
                                    const uint8_t* data) {
    if (!data) {
        return DesfireStatus::DFST_PARAMETER_NULL;
    }

    // For small data chunks that fit in a single frame
    if (length <= ISO7816Constants::ISO_MAX_DATA_SIZE - 7) {
        uint8_t cmdData[7 + ISO7816Constants::ISO_MAX_DATA_SIZE];
        cmdData[0] = fileNo;
        // Offset (3 bytes, little-endian)
        cmdData[1] = offset & 0xFF;
        cmdData[2] = (offset >> 8) & 0xFF;
        cmdData[3] = (offset >> 16) & 0xFF;
        // Length (3 bytes, little-endian)
        cmdData[4] = length & 0xFF;
        cmdData[5] = (length >> 8) & 0xFF;
        cmdData[6] = (length >> 16) & 0xFF;

        // Copy data
        memcpy(&cmdData[7], data, length);

        uint8_t  response[32];
        uint16_t responseLen = 0;

        return transmit(
            DesfireCommand::DF_CMD_WRITE_DATA, cmdData, 7 + length, response, responseLen);
    }
    // For larger data chunks that require multi-frame transfer
    else {
        // We'll split the data into multiple chunks
        const uint16_t maxChunkSize   = ISO7816Constants::ISO_MAX_DATA_SIZE - 7;
        uint32_t       remainingBytes = length;
        uint32_t       currentOffset  = offset;
        uint32_t       bytesSent      = 0;

        while (remainingBytes > 0) {
            // Calculate the size of the current chunk
            uint32_t chunkSize = (remainingBytes > maxChunkSize) ? maxChunkSize : remainingBytes;

            // Prepare command data for this chunk
            uint8_t cmdData[7 + ISO7816Constants::ISO_MAX_DATA_SIZE];
            cmdData[0] = fileNo;
            // Current offset (3 bytes, little-endian)
            cmdData[1] = currentOffset & 0xFF;
            cmdData[2] = (currentOffset >> 8) & 0xFF;
            cmdData[3] = (currentOffset >> 16) & 0xFF;
            // Chunk length (3 bytes, little-endian)
            cmdData[4] = chunkSize & 0xFF;
            cmdData[5] = (chunkSize >> 8) & 0xFF;
            cmdData[6] = (chunkSize >> 16) & 0xFF;

            // Copy chunk data
            memcpy(&cmdData[7], data + bytesSent, chunkSize);

            // Send this chunk
            uint8_t  response[32];
            uint16_t responseLen = 0;

            DesfireStatus status = transmit(
                DesfireCommand::DF_CMD_WRITE_DATA, cmdData, 7 + chunkSize, response, responseLen);

            if (status != DesfireStatus::DFST_SUCCESS) {
                return status;
            }

            // Update tracking variables
            remainingBytes -= chunkSize;
            currentOffset += chunkSize;
            bytesSent += chunkSize;
        }

        return DesfireStatus::DFST_SUCCESS;
    }
}

/**
 * @brief Commit transaction
 *
 * @return DesfireStatus Status code of the operation
 */
DesfireStatus DesfireNFC::commitTransaction() {
    uint8_t  response[32];
    uint16_t responseLen = 0;

    return transmit(DesfireCommand::DF_CMD_COMMIT_TRANSACTION, nullptr, 0, response, responseLen);
}

/**
 * @brief Abort transaction
 *
 * @return DesfireStatus Status code of the operation
 */
DesfireStatus DesfireNFC::abortTransaction() {
    uint8_t  response[32];
    uint16_t responseLen = 0;

    return transmit(DesfireCommand::DF_CMD_ABORT_TRANSACTION, nullptr, 0, response, responseLen);
}

/**
 * @brief Delete a file from the current application
 *
 * @param fileNo File number to delete
 * @return DesfireStatus Status code of the operation
 */
DesfireStatus DesfireNFC::deleteFile(uint8_t fileNo) {
    uint8_t  response[32];
    uint16_t responseLen = 0;

    return transmit(DesfireCommand::DF_CMD_DELETE_FILE, &fileNo, 1, response, responseLen);
}

/**
 * @brief Helper method to print hex data for debugging
 *
 * @param prefix Text prefix before the hex data
 * @param data Data array to print
 * @param length Length of the data array
 */
void DesfireNFC::debugPrintHex(const char* prefix, const uint8_t* data, uint16_t length) {
    if (_debugLevel < DEBUG_VERBOSE)
        return;

    Serial.print(prefix);
    for (uint16_t i = 0; i < length; i++) {
        if (data[i] < 0x10) {
            Serial.print("0");
        }
        Serial.print(data[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
}

/**
 * @brief Print an error message with status code if debug level allows
 *
 * @param message Error message to print
 * @param status Status code related to the error
 */
void DesfireNFC::debugPrintError(const char* message, DesfireStatus status) {
    if (_debugLevel < DEBUG_ERROR)
        return;

    Serial.print("[ERROR] ");
    Serial.print(message);
    Serial.print(" - Status: 0x");
    Serial.println(static_cast<uint16_t>(status), HEX);
}

/**
 * @brief Print an info message if debug level allows
 *
 * @param message Info message to print
 */
void DesfireNFC::debugPrintInfo(const char* message) {
    if (_debugLevel < DEBUG_INFO)
        return;

    Serial.print("[INFO] ");
    Serial.println(message);
}

/**
 * @brief Print a verbose debug message if debug level allows
 *
 * @param message Debug message to print
 */
void DesfireNFC::debugPrintVerbose(const char* message) {
    if (_debugLevel < DEBUG_VERBOSE)
        return;

    Serial.print("[DEBUG] ");
    Serial.println(message);
}