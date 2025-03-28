/**
 * @file ISO7816APDU.cpp
 * @brief Implementation of ISO 7816-4 APDU operations
 */

#include "ISO7816APDU.h"

DesfireStatus ISO7816APDU::buildCommand(const Command& cmd, uint8_t* apdu, uint8_t* apduLength) {
    if (!apdu || !apduLength) {
        return DesfireStatus::DFST_PARAMETER_NULL;
    }

    if (!validateCommand(cmd)) {
        return DesfireStatus::DFST_PARAMETER_ERROR;
    }

    uint8_t index = 0;

    // APDU header (CLA, INS, P1, P2)
    apdu[index++] = cmd.cla;
    apdu[index++] = cmd.ins;
    apdu[index++] = cmd.p1;
    apdu[index++] = cmd.p2;

    // Command data (if any)
    if (cmd.dataLength > 0) {
        apdu[index++] = cmd.dataLength;  // Lc field
        memcpy(&apdu[index], cmd.data, cmd.dataLength);
        index += cmd.dataLength;
    }

    // Expected response length (if any)
    if (cmd.le > 0) {
        apdu[index++] = cmd.le;  // Le field
    }

    *apduLength = index;
    return DesfireStatus::DFST_SUCCESS;
}

DesfireStatus ISO7816APDU::parseResponse(const uint8_t* response,
                                         uint8_t        responseLength,
                                         Response&      result) {
    if (!response || responseLength < ISO7816Constants::ISO_STATUS_LENGTH) {
        return DesfireStatus::DFST_PARAMETER_ERROR;
    }

    // Extract status word (last two bytes)
    result.status = (response[responseLength - 2] << 8) | response[responseLength - 1];

    // Extract response data (if any)
    if (responseLength > ISO7816Constants::ISO_STATUS_LENGTH) {
        result.dataLength = responseLength - ISO7816Constants::ISO_STATUS_LENGTH;
        memcpy(result.data, response, result.dataLength);
    } else {
        result.dataLength = 0;
    }

    return DesfireStatus::DFST_SUCCESS;
}

bool ISO7816APDU::isSuccess(uint16_t status) {
    // Common success codes
    return (status ==
            static_cast<uint16_t>(ISO7816StatusWord::ISO_SW_SUCCESS)) ||  // Normal processing
           (status ==
            static_cast<uint16_t>(ISO7816StatusWord::ISO_SW_SUCCESS_DESFIRE)) ||  // DESFire success
           ((status & 0xFF00) ==
            static_cast<uint16_t>(
                ISO7816StatusWord::ISO_SW_BYTES_REMAINING));  // Response bytes available
}

DesfireStatus ISO7816APDU::convertStatus(uint16_t status) {
    // Convert common ISO 7816-4 status words to DESFire status codes
    switch (status) {
        case static_cast<uint16_t>(ISO7816StatusWord::ISO_SW_SUCCESS):
            return DesfireStatus::DFST_SUCCESS;
        case static_cast<uint16_t>(ISO7816StatusWord::ISO_SW_SUCCESS_DESFIRE):
            return DesfireStatus::DFST_SUCCESS;
        case 0x91AF:
            return DesfireStatus::DFST_MORE_FRAMES;
        case 0x91AE:
            return DesfireStatus::DFST_AUTHENTICATION_ERROR;
        case 0x91F0:
            return DesfireStatus::DFST_FILE_NOT_FOUND;
        case 0x910C:
            return DesfireStatus::DFST_NO_CHANGES;
        case 0x910E:
            return DesfireStatus::DFST_OUT_OF_EEPROM;
        case 0x911C:
            return DesfireStatus::DFST_ILLEGAL_COMMAND;
        case 0x911E:
            return DesfireStatus::DFST_INTEGRITY_ERROR;
        case 0x911F:
            return DesfireStatus::DFST_PARAMETER_ERROR;
        case 0x9140:
            return DesfireStatus::DFST_NO_SUCH_KEY;
        case 0x917E:
            return DesfireStatus::DFST_LENGTH_ERROR;
        case 0x919D:
            return DesfireStatus::DFST_PERMISSION_DENIED;
        case 0x91A0:
            return DesfireStatus::DFST_APPLICATION_NOT_FOUND;
        case 0x91A1:
            return DesfireStatus::DFST_APPLICATION_INTEGRITY_ERROR;
        case 0x91BE:
            return DesfireStatus::DFST_BOUNDARY_ERROR;
        case 0x91C1:
            return DesfireStatus::DFST_PICC_INTEGRITY_ERROR;
        case 0x91CA:
            return DesfireStatus::DFST_COMMAND_ABORTED;
        case 0x91CD:
            return DesfireStatus::DFST_CARD_INTEGRITY_ERROR;
        case 0x91DE:
            return DesfireStatus::DFST_DUPLICATE_ERROR;
        case 0x91EE:
            return DesfireStatus::DFST_EEPROM_ERROR;
        case 0x91F1:
            return DesfireStatus::DFST_FILE_INTEGRITY_ERROR;

        // Additional ISO 7816-4 specific status codes
        case static_cast<uint16_t>(ISO7816StatusWord::ISO_SW_FILE_NOT_FOUND):
            return DesfireStatus::DFST_ISO_FILE_NOT_FOUND;
        case static_cast<uint16_t>(ISO7816StatusWord::ISO_SW_WRONG_LENGTH):
            return DesfireStatus::DFST_ISO_WRONG_LENGTH;  // 0x6700 - Wrong length
        case static_cast<uint16_t>(ISO7816StatusWord::ISO_SW_WRONG_P1P2):
            return DesfireStatus::DFST_ISO_WRONG_PARAMS;
        case static_cast<uint16_t>(ISO7816StatusWord::ISO_SW_INS_NOT_SUPPORTED):
            return DesfireStatus::DFST_ISO_UNKNOWN_INSTRUCTION;
        case static_cast<uint16_t>(ISO7816StatusWord::ISO_SW_SECURITY_NOT_SATISFIED):
            return DesfireStatus::DFST_ISO_SECURITY_STATUS_ERROR;
        case static_cast<uint16_t>(ISO7816StatusWord::ISO_SW_AUTH_METHOD_BLOCKED):
            return DesfireStatus::DFST_ISO_AUTHENTICATION_BLOCKED;
        case static_cast<uint16_t>(ISO7816StatusWord::ISO_SW_DATA_INVALID):
            return DesfireStatus::DFST_ISO_DATA_INVALID;
        case static_cast<uint16_t>(ISO7816StatusWord::ISO_SW_CONDITIONS_NOT_SATISFIED):
            return DesfireStatus::DFST_ISO_CONDITION_NOT_SATISFIED;
        case static_cast<uint16_t>(ISO7816StatusWord::ISO_SW_WRONG_LE):
            return DesfireStatus::DFST_ISO_WRONG_LE;  // 0x6C00 - Incorrect Le byte
        case static_cast<uint16_t>(ISO7816StatusWord::ISO_SW_CLASS_NOT_SUPPORTED):
            return DesfireStatus::DFST_ISO_WRONG_CLA;

        default:
            return DesfireStatus::DFST_LIBRARY_ERROR;
    }
}

bool ISO7816APDU::validateCommand(const Command& cmd) {
    // Check data length
    if (cmd.dataLength > ISO7816Constants::ISO_MAX_DATA_SIZE) {
        return false;
    }

    // Check expected response length
    if (cmd.le > ISO7816Constants::ISO_MAX_DATA_SIZE) {
        return false;
    }

    // Check data pointer if data length is non-zero
    if (cmd.dataLength > 0 && !cmd.data) {
        return false;
    }

    return true;
}