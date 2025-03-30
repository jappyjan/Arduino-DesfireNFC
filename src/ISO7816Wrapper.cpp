/**
 * @file ISO7816Wrapper.cpp
 * @brief Implementation of ISO 7816-4 APDU wrapping functionality for DESFire cards
 */

#include "ISO7816Wrapper.h"

ISO7816Wrapper::ISO7816Wrapper(NFCReaderInterface& reader) : _reader(reader) {
    _lastStatusWord = 0;
}

DesfireStatus ISO7816Wrapper::transmitCommand(DesfireCommand command,
                                              const uint8_t* data,
                                              uint8_t        dataLen,
                                              uint8_t*       response,
                                              uint16_t&      responseLen) {
    if (!response) {
        return DesfireStatus::DFST_PARAMETER_NULL;
    }

    // Build wrapped command directly into the APDU buffer
    uint16_t      apduLen = 0;
    DesfireStatus status  = buildWrappedCommand(command, data, dataLen, _apduBuffer, apduLen);
    if (status != DesfireStatus::DFST_SUCCESS) {
        return status;
    }

    // Transmit APDU with direct response into output buffer
    // This reduces a memory copy operation
    status = transmitAPDU(_apduBuffer, apduLen, response, responseLen);

    // Fast path for successful single-frame responses
    if (status != DesfireStatus::DFST_MORE_FRAMES) {
        return status;
    }

    // We need to handle multi-frame response
    // Use a simplified approach to avoid unnecessary buffer copies
    uint16_t firstFrameLen = responseLen;

    // Initialize complete response length with the first frame
    uint16_t completeResponseLen = firstFrameLen;

    // Save the pointer to where we need to continue appending data
    uint8_t* currentResponsePos = response + firstFrameLen;

    // Handle up to 16 additional frames (arbitrary limit to prevent infinite loops)
    // This should be more than enough for any practical DESFire command
    for (int i = 0; i < 16; i++) {
        // Create GET_ADDITIONAL_FRAME command
        uint16_t cmdApduLen = 0;
        status              = buildWrappedCommand(
            DesfireCommand::DF_CMD_GET_ADDITIONAL_FRAME, nullptr, 0, _apduBuffer, cmdApduLen);
        if (status != DesfireStatus::DFST_SUCCESS) {
            break;
        }

        // Calculate remaining space in the response buffer
        uint16_t remainingSpace = responseLen - completeResponseLen;

        // If we don't have enough space, break
        if (remainingSpace < 2) {
            status = DesfireStatus::DFST_BUFFER_OVERFLOW;
            break;
        }

        // Get additional frame directly into the next position in the response buffer
        uint16_t additionalFrameLen = remainingSpace;

        status = transmitAPDU(_apduBuffer, cmdApduLen, currentResponsePos, additionalFrameLen);

        // If we got an error or there are no more frames, break
        if (status != DesfireStatus::DFST_MORE_FRAMES && status != DesfireStatus::DFST_SUCCESS) {
            break;
        }

        // Update the response position and total length
        currentResponsePos += additionalFrameLen;
        completeResponseLen += additionalFrameLen;

        // If we got all frames, break
        if (status == DesfireStatus::DFST_SUCCESS) {
            break;
        }
    }

    // Update the response length
    responseLen = completeResponseLen;

    return status;
}

DesfireStatus ISO7816Wrapper::transmitAPDU(const uint8_t* apdu,
                                           uint16_t       apduLen,
                                           uint8_t*       response,
                                           uint16_t&      responseLen) {
    if (!apdu || !response) {
        return DesfireStatus::DFST_PARAMETER_NULL;
    }

    // Transceive directly into the response buffer
    uint16_t receivedLen = responseLen;
    if (!_reader.transceive(apdu, apduLen, response, &receivedLen)) {
        return DesfireStatus::DFST_COMMUNICATION_ERROR;
    }

    // Store the actual received length
    responseLen = receivedLen;

    // Check that we have at least a status word
    if (receivedLen < ISO7816Constants::ISO_STATUS_LENGTH) {
        return DesfireStatus::DFST_LENGTH_ERROR;
    }

    // Extract and save the status word
    _lastStatusWord = (response[receivedLen - 2] << 8) | response[receivedLen - 1];

    // Fast path for the most common status codes
    if (_lastStatusWord == 0x9000) {
        return DesfireStatus::DFST_SUCCESS;
    } else if (_lastStatusWord == 0x91AF) {
        return DesfireStatus::DFST_MORE_FRAMES;
    } else if ((_lastStatusWord >> 8) == 0x91) {
        // Other DESFire success codes start with 0x91
        return DesfireStatus::DFST_SUCCESS;
    }

    // Convert ISO7816 status to DesfireStatus for error codes
    switch (_lastStatusWord) {
        case 0x6700:
            return DesfireStatus::DFST_ISO_WRONG_LENGTH;

        case 0x6982:
            return DesfireStatus::DFST_ISO_SECURITY_STATUS_ERROR;

        case 0x6983:
            return DesfireStatus::DFST_ISO_AUTHENTICATION_BLOCKED;

        case 0x6984:
            return DesfireStatus::DFST_ISO_DATA_INVALID;

        case 0x6985:
            return DesfireStatus::DFST_ISO_CONDITION_NOT_SATISFIED;

        case 0x6A82:
            return DesfireStatus::DFST_ISO_FILE_NOT_FOUND;

        case 0x6A86:
            return DesfireStatus::DFST_ISO_WRONG_PARAMS;

        case 0x6D00:
            return DesfireStatus::DFST_ISO_UNKNOWN_INSTRUCTION;

        case 0x6E00:
            return DesfireStatus::DFST_ISO_WRONG_CLA;

        default:
            return DesfireStatus::DFST_LIBRARY_ERROR;
    }
}

DesfireStatus ISO7816Wrapper::buildWrappedCommand(DesfireCommand command,
                                                  const uint8_t* data,
                                                  uint8_t        dataLen,
                                                  uint8_t*       apdu,
                                                  uint16_t&      apduLen) {
    if (!apdu) {
        return DesfireStatus::DFST_PARAMETER_NULL;
    }

    // Fast path approach - build command directly into APDU
    uint16_t index = 0;

    // Add command code first to avoid temporary buffer
    uint8_t commandCode = static_cast<uint8_t>(command);

    // APDU header (CLA, INS, P1, P2)
    apdu[index++] = static_cast<uint8_t>(ISO7816Class::ISO_CLA_DESFIRE);               // CLA = 0x90
    apdu[index++] = static_cast<uint8_t>(ISO7816Instruction::ISO_INS_DESFIRE_NATIVE);  // INS = 0x00
    apdu[index++] = 0x00;                                                              // P1 = 0x00
    apdu[index++] = 0x00;                                                              // P2 = 0x00

    // Calculate total data length (command code + data)
    uint8_t totalDataLen = 1 + (dataLen > 0 ? dataLen : 0);

    // Add Lc field
    apdu[index++] = totalDataLen;

    // Add command code
    apdu[index++] = commandCode;

    // Add command data (if any)
    if (data && dataLen > 0) {
        memcpy(&apdu[index], data, dataLen);
        index += dataLen;
    }

    // Add Le field (request all available data)
    apdu[index++] = 0x00;  // Le = 0x00 (get all available bytes)

    // Update APDU length
    apduLen = index;

    return DesfireStatus::DFST_SUCCESS;
}

DesfireStatus ISO7816Wrapper::buildSelectApplicationAPDU(const uint8_t* aid,
                                                         uint8_t*       apdu,
                                                         uint16_t&      apduLen) {
    if (!aid || !apdu) {
        return DesfireStatus::DFST_PARAMETER_NULL;
    }

    // For DESFire, we can either use:
    // 1. Native command: DF_CMD_SELECT_APPLICATION (0x5A)
    // 2. ISO command: ISO_INS_SELECT_FILE (0xA4)

    // Here we implement the ISO command for better compatibility

    uint16_t index = 0;

    // APDU header
    apdu[index++] = static_cast<uint8_t>(ISO7816Class::ISO_CLA_STANDARD);           // CLA = 0x00
    apdu[index++] = static_cast<uint8_t>(ISO7816Instruction::ISO_INS_SELECT_FILE);  // INS = 0xA4
    apdu[index++] = 0x04;  // P1 = 0x04 (Select by DF name)
    apdu[index++] = 0x00;  // P2 = 0x00 (First occurrence)

    // Add Lc and AID data (note: AID is 3 bytes)
    apdu[index++] = 0x03;  // Lc = 3 bytes
    memcpy(&apdu[index], aid, 3);
    index += 3;

    // Add Le field (request all available data)
    apdu[index++] = 0x00;  // Le = 0x00 (get all available bytes)

    // Update APDU length
    apduLen = index;

    return DesfireStatus::DFST_SUCCESS;
}

DesfireStatus ISO7816Wrapper::handleMultiFrameResponse(const uint8_t* initialResponse,
                                                       uint16_t       initialResponseLen,
                                                       uint8_t*       completeResponse,
                                                       uint16_t&      completeResponseLen) {
    if (!initialResponse || !completeResponse) {
        return DesfireStatus::DFST_PARAMETER_NULL;
    }

    // Copy initial response data
    memcpy(completeResponse, initialResponse, initialResponseLen);
    completeResponseLen = initialResponseLen;

    // Loop until we have all frames
    while (hasMoreFrames(_lastStatusWord)) {
        uint8_t  additionalResponse[ISO7816Constants::ISO_MAX_APDU_SIZE];
        uint16_t additionalResponseLen = 0;

        // Get additional frame
        DesfireStatus status = getAdditionalFrame(additionalResponse, additionalResponseLen);
        if (status != DesfireStatus::DFST_SUCCESS && status != DesfireStatus::DFST_MORE_FRAMES) {
            return status;
        }

        // Add response data to complete response (excluding status word)
        if (additionalResponseLen > 2) {  // Make sure we have data beyond status word
            uint16_t dataLen = additionalResponseLen - 2;  // Exclude status word

            // Check if we have space in the complete response buffer
            if (completeResponseLen + dataLen > ISO7816Constants::ISO_MAX_APDU_SIZE * 4) {
                return DesfireStatus::DFST_BUFFER_OVERFLOW;
            }

            // Copy data
            memcpy(completeResponse + completeResponseLen, additionalResponse, dataLen);
            completeResponseLen += dataLen;
        }

        // Extract status word from additional response
        if (additionalResponseLen >= 2) {
            _lastStatusWord = (additionalResponse[additionalResponseLen - 2] << 8) |
                              additionalResponse[additionalResponseLen - 1];
        }
    }

    return DesfireStatus::DFST_SUCCESS;
}

DesfireStatus ISO7816Wrapper::parseResponse(const uint8_t* response,
                                            uint16_t       responseLen,
                                            uint8_t*       data,
                                            uint16_t&      dataLen,
                                            uint16_t&      statusWord) {
    if (!response) {
        return DesfireStatus::DFST_PARAMETER_NULL;
    }

    // Response must have at least a status word (2 bytes)
    if (responseLen < ISO7816Constants::ISO_STATUS_LENGTH) {
        return DesfireStatus::DFST_LENGTH_ERROR;
    }

    // Extract status word (last two bytes)
    statusWord = (response[responseLen - 2] << 8) | response[responseLen - 1];

    // Extract data if any
    if (responseLen > ISO7816Constants::ISO_STATUS_LENGTH && data != nullptr) {
        dataLen = responseLen - ISO7816Constants::ISO_STATUS_LENGTH;
        memcpy(data, response, dataLen);
    } else {
        dataLen = 0;
    }

    // Convert status word to DESFire status
    return convertStatusWord(statusWord);
}

bool ISO7816Wrapper::hasMoreFrames(uint16_t statusWord) const {
    // Check for DESFire "more frames" status: 0x91AF
    return statusWord == 0x91AF;
}

DesfireStatus ISO7816Wrapper::getAdditionalFrame(uint8_t* response, uint16_t& responseLen) {
    // Build Get Additional Frame command
    uint16_t      apduLen = 0;
    DesfireStatus status  = buildWrappedCommand(
        DesfireCommand::DF_CMD_GET_ADDITIONAL_FRAME, nullptr, 0, _apduBuffer, apduLen);
    if (status != DesfireStatus::DFST_SUCCESS) {
        return status;
    }

    // Transmit APDU
    return transmitAPDU(_apduBuffer, apduLen, response, responseLen);
}

DesfireStatus ISO7816Wrapper::convertStatusWord(uint16_t statusWord) const {
    // Common ISO 7816-4 and DESFire status words
    switch (statusWord) {
        case static_cast<uint16_t>(ISO7816StatusWord::ISO_SW_SUCCESS):
        case static_cast<uint16_t>(ISO7816StatusWord::ISO_SW_SUCCESS_DESFIRE):
            return DesfireStatus::DFST_SUCCESS;

        case 0x91AF:  // DESFire more frames
            return DesfireStatus::DFST_MORE_FRAMES;

        case 0x91AE:  // DESFire authentication error
            return DesfireStatus::DFST_AUTHENTICATION_ERROR;

        case static_cast<uint16_t>(ISO7816StatusWord::ISO_SW_WRONG_LENGTH):
            return DesfireStatus::DFST_ISO_WRONG_LENGTH;

        case static_cast<uint16_t>(ISO7816StatusWord::ISO_SW_SECURITY_NOT_SATISFIED):
            return DesfireStatus::DFST_ISO_SECURITY_STATUS_ERROR;

        case static_cast<uint16_t>(ISO7816StatusWord::ISO_SW_CONDITIONS_NOT_SATISFIED):
            return DesfireStatus::DFST_ISO_CONDITION_NOT_SATISFIED;

        case static_cast<uint16_t>(ISO7816StatusWord::ISO_SW_WRONG_P1P2):
            return DesfireStatus::DFST_ISO_WRONG_PARAMS;

        case static_cast<uint16_t>(ISO7816StatusWord::ISO_SW_INS_NOT_SUPPORTED):
            return DesfireStatus::DFST_ISO_UNKNOWN_INSTRUCTION;

        case static_cast<uint16_t>(ISO7816StatusWord::ISO_SW_CLASS_NOT_SUPPORTED):
            return DesfireStatus::DFST_ISO_WRONG_CLA;

        // Add other DESFire-specific status codes
        case 0x91F0:  // DESFire file not found
            return DesfireStatus::DFST_FILE_NOT_FOUND;

        case 0x910C:  // DESFire no changes
            return DesfireStatus::DFST_NO_CHANGES;

        case 0x910E:  // DESFire out of EEPROM
            return DesfireStatus::DFST_OUT_OF_EEPROM;

        case 0x911C:  // DESFire illegal command
            return DesfireStatus::DFST_ILLEGAL_COMMAND;

        case 0x911E:  // DESFire integrity error
            return DesfireStatus::DFST_INTEGRITY_ERROR;

        case 0x911F:  // DESFire parameter error
            return DesfireStatus::DFST_PARAMETER_ERROR;

        case 0x9140:  // DESFire no such key
            return DesfireStatus::DFST_NO_SUCH_KEY;

        default:
            return DesfireStatus::DFST_LIBRARY_ERROR;
    }
}