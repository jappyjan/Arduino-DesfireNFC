/**
 * @file DesfireTypes.h
 * @brief Type definitions for DESFire operations
 *
 * This file contains structure and enum definitions used by the DESFire library.
 */

#ifndef DESFIRE_TYPES_H
#define DESFIRE_TYPES_H

#include <Arduino.h>

/**
 * @brief DESFire command codes
 */
enum DesfireCommand : uint8_t {
    // Card Level Commands
    DF_CMD_GET_VERSION               = 0x60,  ///< Get card manufacturing related data
    DF_CMD_GET_CARD_UID              = 0x51,  ///< Get card UID (requires authentication)
    DF_CMD_GET_APPLICATION_DIRECTORY = 0x6A,  ///< Get application identifiers present on card
    DF_CMD_GET_ADDITIONAL_FRAME =
        0xAF,  ///< Get additional data if command response doesn't fit into buffer
    DF_CMD_SELECT_APPLICATION = 0x5A,  ///< Select one application for further access
    DF_CMD_FORMAT_PICC        = 0xFC,  ///< Erase all applications and files on card
    DF_CMD_GET_FREE_MEMORY    = 0x6E,  ///< Get available memory on card

    // Authentication Commands
    DF_CMD_AUTHENTICATE        = 0x0A,  ///< Authenticate with DES/3DES keys
    DF_CMD_AUTHENTICATE_ISO    = 0x1A,  ///< Authenticate with 3DES keys (ISO mode)
    DF_CMD_AUTHENTICATE_AES    = 0xAA,  ///< Authenticate with AES keys
    DF_CMD_CHANGE_KEY_SETTINGS = 0x54,  ///< Change key settings for application
    DF_CMD_SET_CONFIGURATION   = 0x5C,  ///< Change card configuration settings
    DF_CMD_CHANGE_KEY          = 0xC4,  ///< Change encryption key
    DF_CMD_GET_KEY_VERSION     = 0x64,  ///< Get encryption key version

    // Application Management Commands
    DF_CMD_CREATE_APPLICATION  = 0xCA,  ///< Create a new application on card
    DF_CMD_DELETE_APPLICATION  = 0xDA,  ///< Delete application and all related files
    DF_CMD_GET_APPLICATION_IDS = 0x6A,  ///< Get identifier of applications on card

    // File Management Commands
    DF_CMD_CREATE_STANDARD_FILE      = 0xCD,  ///< Create a standard data file
    DF_CMD_CREATE_BACKUP_FILE        = 0xCB,  ///< Create a backup data file
    DF_CMD_CREATE_VALUE_FILE         = 0xCC,  ///< Create a value file
    DF_CMD_CREATE_LINEAR_RECORD_FILE = 0xC1,  ///< Create a linear record file
    DF_CMD_CREATE_CYCLIC_RECORD_FILE = 0xC0,  ///< Create a cyclic record file
    DF_CMD_DELETE_FILE               = 0xDF,  ///< Delete a file
    DF_CMD_GET_FILE_IDS              = 0x6F,  ///< Get file identifiers
    DF_CMD_GET_FILE_SETTINGS         = 0xF5,  ///< Get file settings
    DF_CMD_CHANGE_FILE_SETTINGS      = 0x5F,  ///< Change file settings

    // Data Manipulation Commands
    DF_CMD_READ_DATA         = 0xBD,  ///< Read data from standard/backup files
    DF_CMD_WRITE_DATA        = 0x3D,  ///< Write data to standard/backup files
    DF_CMD_GET_VALUE         = 0x6C,  ///< Get value from value file
    DF_CMD_CREDIT            = 0x0C,  ///< Increase value in value file
    DF_CMD_DEBIT             = 0xDC,  ///< Decrease value in value file
    DF_CMD_LIMITED_CREDIT    = 0x1C,  ///< Limited increase of value in value file
    DF_CMD_WRITE_RECORD      = 0x3B,  ///< Write record to record file
    DF_CMD_READ_RECORDS      = 0xBB,  ///< Read record(s) from record file
    DF_CMD_CLEAR_RECORD_FILE = 0xEB,  ///< Clear a record file

    // Transaction Commands
    DF_CMD_COMMIT_TRANSACTION = 0xC7,  ///< Commit previous write access
    DF_CMD_ABORT_TRANSACTION  = 0xA7   ///< Abort previous write access
};

/**
 * @brief DESFire cryptographic mode
 */
enum DesfreCryptoMode : uint8_t {
    DF_CRYPTO_DES    = 0x00,  ///< DES mode (56-bit key)
    DF_CRYPTO_3K3DES = 0x01,  ///< 3-key Triple DES (168-bit key)
    DF_CRYPTO_AES    = 0x02   ///< AES (128-bit key)
};

/**
 * @brief Communication mode for commands
 */
enum DesfreCommunicationMode : uint8_t {
    DF_COMM_PLAIN   = 0x00,  ///< Plain communication, no encryption/MAC
    DF_COMM_MAC     = 0x01,  ///< Plain data with MAC
    DF_COMM_ENCRYPT = 0x03   ///< Encrypted data
};

/**
 * @brief DESFire card version information
 */
struct DESFireCardVersion {
    uint8_t hardwareVendor;        ///< Hardware vendor ID
    uint8_t hardwareType;          ///< Hardware type
    uint8_t hardwareSubtype;       ///< Hardware subtype
    uint8_t hardwareVersionMajor;  ///< Hardware major version
    uint8_t hardwareVersionMinor;  ///< Hardware minor version
    uint8_t hardwareStorageSize;   ///< Hardware storage size
    uint8_t hardwareProtocol;      ///< Hardware protocol info

    uint8_t softwareVendor;        ///< Software vendor ID
    uint8_t softwareType;          ///< Software type
    uint8_t softwareSubtype;       ///< Software subtype
    uint8_t softwareVersionMajor;  ///< Software major version
    uint8_t softwareVersionMinor;  ///< Software minor version
    uint8_t softwareStorageSize;   ///< Software storage size
    uint8_t softwareProtocol;      ///< Software protocol info

    uint8_t uid[7];          ///< 7-byte UID
    uint8_t batchNumber[5];  ///< 5-byte production batch number
    uint8_t productionWeek;  ///< Production week (BCD)
    uint8_t productionYear;  ///< Production year (BCD)

    /**
     * @brief Get card type name as string
     *
     * @return const char* Name of the card type
     */
    const char* getCardTypeName() const {
        switch (hardwareType) {
            case 0x00:
                return "DESFire";
            case 0x01:
                return "DESFire EV1";
            case 0x02:
                return "DESFire EV2";
            case 0x03:
                return "DESFire EV3";
            case 0x41:
                return "DESFire Light";
            default:
                return "Unknown";
        }
    }

    /**
     * @brief Get storage size in bytes
     *
     * @return uint32_t Storage size in bytes
     */
    uint32_t getStorageSize() const {
        switch (softwareStorageSize) {
            case 0x00:
                return 2048;  // 2K
            case 0x01:
                return 4096;  // 4K
            case 0x02:
                return 8192;  // 8K
            case 0x03:
                return 16384;  // 16K
            case 0x04:
                return 32768;  // 32K
            case 0x0D:
                return 131072;  // 128K (Light)
            default:
                return 0;  // Unknown
        }
    }
};

/**
 * @brief File access modes
 */
enum DesfreFileAccess : uint8_t {
    DF_ACCESS_NONE  = 0x00,  ///< Access denied
    DF_ACCESS_KEY0  = 0x01,  ///< Access granted with Key 0
    DF_ACCESS_KEY1  = 0x02,  ///< Access granted with Key 1
    DF_ACCESS_KEY2  = 0x03,  ///< Access granted with Key 2
    DF_ACCESS_KEY3  = 0x04,  ///< Access granted with Key 3
    DF_ACCESS_KEY4  = 0x05,  ///< Access granted with Key 4
    DF_ACCESS_KEY5  = 0x06,  ///< Access granted with Key 5
    DF_ACCESS_KEY6  = 0x07,  ///< Access granted with Key 6
    DF_ACCESS_KEY7  = 0x08,  ///< Access granted with Key 7
    DF_ACCESS_KEY8  = 0x09,  ///< Access granted with Key 8
    DF_ACCESS_KEY9  = 0x0A,  ///< Access granted with Key 9
    DF_ACCESS_KEY10 = 0x0B,  ///< Access granted with Key 10
    DF_ACCESS_KEY11 = 0x0C,  ///< Access granted with Key 11
    DF_ACCESS_KEY12 = 0x0D,  ///< Access granted with Key 12
    DF_ACCESS_KEY13 = 0x0E,  ///< Access granted with Key 13
    DF_ACCESS_FREE  = 0x0F   ///< Access granted without authentication
};

/**
 * @brief File types
 */
enum DesfreFileType : uint8_t {
    DF_FILE_STANDARD = 0x00,  ///< Standard data file
    DF_FILE_BACKUP   = 0x01,  ///< Backup data file
    DF_FILE_VALUE    = 0x02,  ///< Value file for stored value
    DF_FILE_LINEAR   = 0x03,  ///< Linear record file
    DF_FILE_CYCLIC   = 0x04   ///< Cyclic record file
};

/**
 * @brief Key settings for applications and files
 */
enum DesfireKeySettings : uint8_t {
    // Application master key settings
    DF_KS_ALLOW_CHANGE_MK         = 0x01,  ///< Allow changing master key
    DF_KS_FREE_LISTING_WITHOUT_MK = 0x02,  ///< Allow directory listing without master key
    DF_KS_FREE_CREATE_DELETE_WITHOUT_MK =
        0x04,                               ///< Allow file creation/deletion without master key
    DF_KS_CONFIGURATION_CHANGEABLE = 0x08,  ///< Configuration settings are changeable

    // Application key settings
    DF_KS_ALLOW_CHANGE_KEYS   = 0x10,  ///< Allow changing keys (except master key)
    DF_KS_REQUIRE_CURRENT_KEY = 0x20,  ///< Require current key for key change

    // Default settings
    DF_KS_DEFAULT_APP  = 0x0F,  ///< Default application settings
    DF_KS_DEFAULT_PICC = 0x0F   ///< Default PICC settings
};

/**
 * @brief Access rights for files
 */
enum DesfireFileAccess : uint8_t {
    // Access rights bits
    DF_AR_READ_MASK  = 0x0F,  ///< Mask for read access rights
    DF_AR_WRITE_MASK = 0xF0,  ///< Mask for write access rights

    // Access rights values
    DF_AR_KEY0  = 0x00,  ///< Access with key 0
    DF_AR_KEY1  = 0x01,  ///< Access with key 1
    DF_AR_KEY2  = 0x02,  ///< Access with key 2
    DF_AR_KEY3  = 0x03,  ///< Access with key 3
    DF_AR_KEY4  = 0x04,  ///< Access with key 4
    DF_AR_KEY5  = 0x05,  ///< Access with key 5
    DF_AR_KEY6  = 0x06,  ///< Access with key 6
    DF_AR_KEY7  = 0x07,  ///< Access with key 7
    DF_AR_KEY8  = 0x08,  ///< Access with key 8
    DF_AR_KEY9  = 0x09,  ///< Access with key 9
    DF_AR_KEY10 = 0x0A,  ///< Access with key 10
    DF_AR_KEY11 = 0x0B,  ///< Access with key 11
    DF_AR_KEY12 = 0x0C,  ///< Access with key 12
    DF_AR_KEY13 = 0x0D,  ///< Access with key 13
    DF_AR_FREE  = 0x0E,  ///< Free access
    DF_AR_NEVER = 0x0F   ///< Access never allowed
};

/**
 * @brief DESFire EV2 specific commands
 */
enum DesfireEV2Command : uint8_t {
    // Authentication commands
    DF_CMD_AUTHENTICATE_EV2_FIRST    = 0x71,  ///< First part of EV2 authentication
    DF_CMD_AUTHENTICATE_EV2_NONFIRST = 0x77,  ///< Non-first part of EV2 authentication

    // Transaction MAC commands
    DF_CMD_COMMIT_TRANSACTION_MAC = 0xC7,  ///< Commit transaction with MAC
    DF_CMD_ABORT_TRANSACTION_MAC  = 0xA7,  ///< Abort transaction with MAC

    // Command counter
    DF_CMD_GET_COMMAND_COUNTER = 0x7A,  ///< Get command counter
    DF_CMD_SET_COMMAND_COUNTER = 0x7B   ///< Set command counter
};

/**
 * @brief DESFire EV2 specific constants
 */
enum DesfireEV2Constants : uint8_t {
    DF_EV2_MAC_LENGTH     = 8,  ///< Length of CMAC in bytes
    DF_EV2_COUNTER_LENGTH = 4,  ///< Length of command counter in bytes
    DF_EV2_TI_LENGTH      = 4   ///< Length of transaction identifier in bytes
};

#endif  // DESFIRE_TYPES_H