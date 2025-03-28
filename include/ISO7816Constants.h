/**
 * @file ISO7816Constants.h
 * @brief ISO 7816-4 protocol constants
 *
 * This file contains constants used in ISO 7816-4 protocol communication,
 * including APDU command and response formatting.
 */

#ifndef ISO7816_CONSTANTS_H
#define ISO7816_CONSTANTS_H

#include <Arduino.h>

/**
 * @brief ISO 7816-4 class byte (CLA) values
 */
enum ISO7816Class : uint8_t {
    ISO_CLA_STANDARD    = 0x00,  ///< Standard class
    ISO_CLA_DESFIRE     = 0x90,  ///< DESFire class
    ISO_CLA_PROPRIETARY = 0x80   ///< Proprietary class
};

/**
 * @brief ISO 7816-4 instruction byte (INS) values
 */
enum ISO7816Instruction : uint8_t {
    // Basic interindustry instructions
    ISO_INS_ERASE_BINARY   = 0x0E,  ///< Erase binary data
    ISO_INS_VERIFY         = 0x20,  ///< Verify PIN/password
    ISO_INS_MANAGE_CHANNEL = 0x70,  ///< Manage logical channel
    ISO_INS_EXTERNAL_AUTH  = 0x82,  ///< External authenticate
    ISO_INS_GET_CHALLENGE  = 0x84,  ///< Get challenge for authentication
    ISO_INS_INTERNAL_AUTH  = 0x88,  ///< Internal authenticate
    ISO_INS_SELECT_FILE    = 0xA4,  ///< Select file/application
    ISO_INS_READ_BINARY    = 0xB0,  ///< Read binary data
    ISO_INS_READ_RECORD    = 0xB2,  ///< Read record
    ISO_INS_GET_RESPONSE   = 0xC0,  ///< Get response data
    ISO_INS_ENVELOPE       = 0xC2,  ///< Envelope command
    ISO_INS_GET_DATA       = 0xCA,  ///< Get data object
    ISO_INS_WRITE_BINARY   = 0xD0,  ///< Write binary data
    ISO_INS_WRITE_RECORD   = 0xD2,  ///< Write record
    ISO_INS_UPDATE_BINARY  = 0xD6,  ///< Update binary data
    ISO_INS_PUT_DATA       = 0xDA,  ///< Put data object

    // DESFire specific instructions
    ISO_INS_DESFIRE_NATIVE = 0x00  ///< DESFire native command wrapper
};

/**
 * @brief ISO 7816-4 status word (SW1-SW2) values
 */
enum ISO7816StatusWord : uint16_t {
    // Success codes
    ISO_SW_SUCCESS         = 0x9000,  ///< Command successful
    ISO_SW_SUCCESS_DESFIRE = 0x9100,  ///< DESFire success

    // Warning codes
    ISO_SW_WARNING_NVM         = 0x6200,  ///< Warning: NVM unchanged
    ISO_SW_WARNING_MEMORY      = 0x6281,  ///< Warning: Memory unchanged
    ISO_SW_WARNING_EOF         = 0x6282,  ///< Warning: End of file reached
    ISO_SW_WARNING_INVALIDATED = 0x6283,  ///< Warning: File invalidated
    ISO_SW_WARNING_FCI         = 0x6284,  ///< Warning: FCI not formatted
    ISO_SW_BYTES_REMAINING     = 0x6100,  ///< Warning: Bytes still available

    // Error codes
    ISO_SW_WRONG_LENGTH           = 0x6700,  ///< Wrong length
    ISO_SW_WRONG_LE               = 0x6C00,  ///< Incorrect Le byte (expected length doesn't match)
    ISO_SW_SECURITY_NOT_SATISFIED = 0x6982,  ///< Security status not satisfied
    ISO_SW_AUTH_METHOD_BLOCKED    = 0x6983,  ///< Authentication method blocked
    ISO_SW_DATA_INVALID           = 0x6984,  ///< Referenced data invalid
    ISO_SW_CONDITIONS_NOT_SATISFIED = 0x6985,  ///< Conditions of use not satisfied
    ISO_SW_COMMAND_NOT_ALLOWED      = 0x6986,  ///< Command not allowed
    ISO_SW_WRONG_DATA               = 0x6A80,  ///< Incorrect data
    ISO_SW_FILE_NOT_FOUND           = 0x6A82,  ///< File not found
    ISO_SW_RECORD_NOT_FOUND         = 0x6A83,  ///< Record not found
    ISO_SW_WRONG_P1P2               = 0x6A86,  ///< Incorrect P1-P2 parameters
    ISO_SW_WRONG_DATA_LENGTH        = 0x6A87,  ///< Wrong data length
    ISO_SW_INS_NOT_SUPPORTED        = 0x6D00,  ///< Instruction code not supported
    ISO_SW_CLASS_NOT_SUPPORTED      = 0x6E00,  ///< Class not supported
    ISO_SW_UNKNOWN                  = 0x6F00   ///< Unknown error
};

/**
 * @brief ISO 7816-4 APDU constants
 */
enum ISO7816Constants : uint16_t {
    ISO_MAX_APDU_SIZE = 261,  ///< Maximum size of APDU (CLA+INS+P1+P2+Lc+Data+Le)
    ISO_MAX_DATA_SIZE = 255,  ///< Maximum size of data field
    ISO_HEADER_LENGTH = 4,    ///< Length of APDU header (CLA+INS+P1+P2)
    ISO_MAX_LE_SIZE   = 1,    ///< Maximum size of Le field (1 byte in short APDU)
    ISO_MAX_LC_SIZE   = 1,    ///< Maximum size of Lc field (1 byte in short APDU)
    ISO_STATUS_LENGTH = 2     ///< Length of status word (SW1+SW2)
};

#endif  // ISO7816_CONSTANTS_H