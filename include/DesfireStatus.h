/**
 * @file DesfireStatus.h
 * @brief Status codes for DESFire operations
 *
 * This file contains definitions for status codes returned by DESFire operations.
 * These are based on the DESFire EV1/EV2 documentation.
 */

#ifndef DESFIRE_STATUS_H
#define DESFIRE_STATUS_H

/**
 * @brief Status codes for DESFire operations
 */
enum class DesfireStatus : uint16_t {
    // Success codes
    DFST_SUCCESS     = 0x00,  ///< Successful operation
    DFST_MORE_FRAMES = 0xAF,  ///< Additional data frames are expected

    // General error codes
    DFST_OPERATION_OK      = 0x00,  ///< Operation successful
    DFST_NO_CHANGES        = 0x0C,  ///< No changes made to backup files
    DFST_OUT_OF_EEPROM     = 0x0E,  ///< Insufficient memory for operation
    DFST_ILLEGAL_COMMAND   = 0x1C,  ///< Command code not supported
    DFST_INTEGRITY_ERROR   = 0x1E,  ///< CRC or MAC does not match data
    DFST_PARAMETER_ERROR   = 0x1F,  ///< Invalid command parameter
    DFST_NO_SUCH_KEY       = 0x40,  ///< Key specified in command does not exist
    DFST_LENGTH_ERROR      = 0x7E,  ///< Length of command does not match with its specification
    DFST_PERMISSION_DENIED = 0x9D,  ///< Current configuration/status does not allow the operation
    DFST_APPLICATION_NOT_FOUND       = 0xA0,  ///< Requested AID not present on PICC
    DFST_APPLICATION_INTEGRITY_ERROR = 0xA1,  ///< Unrecoverable error within application
    DFST_AUTHENTICATION_ERROR =
        0xAE,  ///< Current authentication status does not allow the operation
    DFST_BOUNDARY_ERROR       = 0xBE,  ///< Attempted to read/write beyond the file's/record's limit
    DFST_PICC_INTEGRITY_ERROR = 0xC1,  ///< Unrecoverable error within the PICC
    DFST_COMMAND_ABORTED      = 0xCA,  ///< Previous command was not fully completed
    DFST_CARD_INTEGRITY_ERROR = 0xCD,  ///< Unrecoverable error within PICC
    DFST_DUPLICATE_ERROR = 0xDE,  ///< Attempted creation of file/application that already exists
    DFST_EEPROM_ERROR    = 0xEE,  ///< Error related to EEPROM
    DFST_FILE_NOT_FOUND  = 0xF0,  ///< Specified file does not exist
    DFST_FILE_INTEGRITY_ERROR = 0xF1,  ///< Unrecoverable error within file

    // Library-specific error codes
    DFST_LIBRARY_ERROR       = 0xFF,  ///< Internal library error
    DFST_COMMUNICATION_ERROR = 0xFE,  ///< Error in communication with card
    DFST_PARAMETER_NULL      = 0xFD,  ///< NULL parameter provided
    DFST_CRYPTO_ERROR        = 0xFC,  ///< Error in cryptographic operation
    DFST_BUFFER_OVERFLOW     = 0xFB,  ///< Buffer overflow
    DFST_BUFFER_TOO_SMALL    = 0xFA,  ///< Buffer provided is too small

    // ISO7816 status codes
    DFST_ISO_COMMAND_COMPLETED       = 0x9000,  ///< Command completed
    DFST_ISO_FILE_NOT_FOUND          = 0x6A82,  ///< File not found
    DFST_ISO_WRONG_LENGTH            = 0x6700,  ///< Wrong length
    DFST_ISO_WRONG_PARAMS            = 0x6A86,  ///< Incorrect parameters P1-P2
    DFST_ISO_UNKNOWN_INSTRUCTION     = 0x6D00,  ///< Instruction code not supported
    DFST_ISO_SECURITY_STATUS_ERROR   = 0x6982,  ///< Security status not satisfied
    DFST_ISO_AUTHENTICATION_BLOCKED  = 0x6983,  ///< Authentication method blocked
    DFST_ISO_DATA_INVALID            = 0x6984,  ///< Referenced data invalidated
    DFST_ISO_CONDITION_NOT_SATISFIED = 0x6985,  ///< Conditions of use not satisfied
    DFST_ISO_WRONG_LE                = 0x6C00,  ///< Incorrect Le byte
    DFST_ISO_WRONG_CLA               = 0x6E00   ///< Class not supported
};

#endif  // DESFIRE_STATUS_H