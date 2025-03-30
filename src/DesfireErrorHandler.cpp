/**
 * @file DesfireErrorHandler.cpp
 * @brief Implementation of error handling and status code interpretation for DESFire operations
 */

#include "DesfireErrorHandler.h"

const char* DesfireErrorHandler::getErrorMessage(DesfireStatus status) {
    switch (status) {
        // Success codes
        case DesfireStatus::DFST_SUCCESS:
            return "Operation successful";
        case DesfireStatus::DFST_MORE_FRAMES:
            return "Additional data frames are expected";
        case DesfireStatus::DFST_NO_CHANGES:
            return "No changes made to backup files";

        // General error codes
        case DesfireStatus::DFST_OUT_OF_EEPROM:
            return "Insufficient memory for operation";
        case DesfireStatus::DFST_ILLEGAL_COMMAND:
            return "Command code not supported";
        case DesfireStatus::DFST_INTEGRITY_ERROR:
            return "CRC or MAC does not match data";
        case DesfireStatus::DFST_PARAMETER_ERROR:
            return "Invalid command parameter";
        case DesfireStatus::DFST_NO_SUCH_KEY:
            return "Key specified in command does not exist";
        case DesfireStatus::DFST_LENGTH_ERROR:
            return "Length of command does not match with its specification";
        case DesfireStatus::DFST_PERMISSION_DENIED:
            return "Current configuration/status does not allow the operation";
        case DesfireStatus::DFST_APPLICATION_NOT_FOUND:
            return "Requested AID not present on PICC";
        case DesfireStatus::DFST_APPLICATION_INTEGRITY_ERROR:
            return "Unrecoverable error within application";
        case DesfireStatus::DFST_AUTHENTICATION_ERROR:
            return "Current authentication status does not allow the operation";
        case DesfireStatus::DFST_BOUNDARY_ERROR:
            return "Attempted to read/write beyond the file's/record's limit";
        case DesfireStatus::DFST_PICC_INTEGRITY_ERROR:
            return "Unrecoverable error within the PICC";
        case DesfireStatus::DFST_COMMAND_ABORTED:
            return "Previous command was not fully completed";
        case DesfireStatus::DFST_CARD_INTEGRITY_ERROR:
            return "Unrecoverable error within PICC";
        case DesfireStatus::DFST_DUPLICATE_ERROR:
            return "Attempted creation of file/application that already exists";
        case DesfireStatus::DFST_EEPROM_ERROR:
            return "Error related to EEPROM";
        case DesfireStatus::DFST_FILE_NOT_FOUND:
            return "Specified file does not exist";
        case DesfireStatus::DFST_FILE_INTEGRITY_ERROR:
            return "Unrecoverable error within file";

        // Library-specific error codes
        case DesfireStatus::DFST_LIBRARY_ERROR:
            return "Internal library error";
        case DesfireStatus::DFST_COMMUNICATION_ERROR:
            return "Error in communication with card";
        case DesfireStatus::DFST_PARAMETER_NULL:
            return "NULL parameter provided";
        case DesfireStatus::DFST_CRYPTO_ERROR:
            return "Error in cryptographic operation";
        case DesfireStatus::DFST_BUFFER_OVERFLOW:
            return "Buffer overflow";
        case DesfireStatus::DFST_BUFFER_TOO_SMALL:
            return "Buffer provided is too small";

        // ISO7816 status codes
        case DesfireStatus::DFST_ISO_COMMAND_COMPLETED:
            return "Command completed successfully";
        case DesfireStatus::DFST_ISO_FILE_NOT_FOUND:
            return "File not found";
        case DesfireStatus::DFST_ISO_WRONG_LENGTH:
            return "Wrong length";
        case DesfireStatus::DFST_ISO_WRONG_PARAMS:
            return "Incorrect parameters P1-P2";
        case DesfireStatus::DFST_ISO_UNKNOWN_INSTRUCTION:
            return "Instruction code not supported";
        case DesfireStatus::DFST_ISO_SECURITY_STATUS_ERROR:
            return "Security status not satisfied";
        case DesfireStatus::DFST_ISO_AUTHENTICATION_BLOCKED:
            return "Authentication method blocked";
        case DesfireStatus::DFST_ISO_DATA_INVALID:
            return "Referenced data invalidated";
        case DesfireStatus::DFST_ISO_CONDITION_NOT_SATISFIED:
            return "Conditions of use not satisfied";
        case DesfireStatus::DFST_ISO_WRONG_LE:
            return "Incorrect Le byte";
        case DesfireStatus::DFST_ISO_WRONG_CLA:
            return "Class not supported";

        default:
            return "Unknown error code";
    }
}

bool DesfireErrorHandler::isSuccess(DesfireStatus status) {
    return status == DesfireStatus::DFST_SUCCESS ||
           status == DesfireStatus::DFST_ISO_COMMAND_COMPLETED;
}

bool DesfireErrorHandler::isMoreFrames(DesfireStatus status) {
    return status == DesfireStatus::DFST_MORE_FRAMES;
}

const char* DesfireErrorHandler::getErrorCategory(DesfireStatus status) {
    if (isAuthenticationError(status)) {
        return "Authentication";
    } else if (isFileError(status)) {
        return "File";
    } else if (isApplicationError(status)) {
        return "Application";
    } else if (isCommunicationError(status)) {
        return "Communication";
    } else {
        return "General";
    }
}

bool DesfireErrorHandler::isAuthenticationError(DesfireStatus status) {
    return status == DesfireStatus::DFST_AUTHENTICATION_ERROR ||
           status == DesfireStatus::DFST_ISO_SECURITY_STATUS_ERROR ||
           status == DesfireStatus::DFST_ISO_AUTHENTICATION_BLOCKED ||
           status == DesfireStatus::DFST_NO_SUCH_KEY;
}

bool DesfireErrorHandler::isFileError(DesfireStatus status) {
    return status == DesfireStatus::DFST_FILE_NOT_FOUND ||
           status == DesfireStatus::DFST_ISO_FILE_NOT_FOUND ||
           status == DesfireStatus::DFST_FILE_INTEGRITY_ERROR ||
           status == DesfireStatus::DFST_BOUNDARY_ERROR;
}

bool DesfireErrorHandler::isApplicationError(DesfireStatus status) {
    return status == DesfireStatus::DFST_APPLICATION_NOT_FOUND ||
           status == DesfireStatus::DFST_APPLICATION_INTEGRITY_ERROR ||
           status == DesfireStatus::DFST_DUPLICATE_ERROR;
}

bool DesfireErrorHandler::isCommunicationError(DesfireStatus status) {
    return status == DesfireStatus::DFST_COMMUNICATION_ERROR ||
           status == DesfireStatus::DFST_INTEGRITY_ERROR ||
           status == DesfireStatus::DFST_LENGTH_ERROR;
}

void DesfireErrorHandler::printErrorReport(DesfireStatus status, const char* operation) {
    if (isSuccess(status)) {
        Serial.println(F("Operation successful"));
        return;
    }

    Serial.println(F("DESFire Error:"));

    if (operation) {
        Serial.print(F("  Operation: "));
        Serial.println(operation);
    }

    Serial.print(F("  Status code: 0x"));
    Serial.println(static_cast<uint16_t>(status), HEX);

    Serial.print(F("  Category: "));
    Serial.println(getErrorCategory(status));

    Serial.print(F("  Message: "));
    Serial.println(getErrorMessage(status));

    // Add specific troubleshooting tips for common errors
    if (isAuthenticationError(status)) {
        Serial.println(F("  Troubleshooting: Please check authentication key and try again."));
        Serial.println(F("                   Make sure you've selected the correct application."));
    } else if (isFileError(status)) {
        Serial.println(F("  Troubleshooting: Verify file existence and correct file ID."));
        Serial.println(F("                   Make sure you have the proper file access rights."));
    } else if (isCommunicationError(status)) {
        Serial.println(F("  Troubleshooting: Check card position and connection to reader."));
        Serial.println(F("                   Try slower communication speed or verify wiring."));
    }
}