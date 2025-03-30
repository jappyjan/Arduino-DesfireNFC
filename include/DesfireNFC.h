/**
 * @file DesfireNFC.h
 * @brief Main header file for the DesfireNFC library
 *
 * This library provides a comprehensive implementation for communicating with
 * MIFARE DESFire EV1/EV2 cards using the PN532 NFC controller.
 */

#ifndef DESFIRE_NFC_H
#define DESFIRE_NFC_H

#include <Arduino.h>
#include "DesfireStatus.h"
#include "DesfireTypes.h"
#include "ISO7816APDU.h"
#include "ISO7816Constants.h"
#include "NFCReaderInterface.h"

/**
 * @brief Main class for DESFire NFC operations
 *
 * This class provides the high-level interface for all DESFire card operations
 * including authentication, file operations, and secure messaging.
 */
class DesfireNFC {
public:
    // Debug level enum
    enum DebugLevel {
        DEBUG_NONE    = 0,  // No debug output
        DEBUG_ERROR   = 1,  // Only errors
        DEBUG_INFO    = 2,  // Errors and general info
        DEBUG_VERBOSE = 3   // All debug info including raw communication
    };

    /**
     * @brief Construct a new DesfireNFC object
     *
     * @param reader Reference to an NFC reader implementation
     * @param debugLevel Initial debug level (default: no debugging)
     */
    DesfireNFC(NFCReaderInterface& reader, DebugLevel debugLevel = DEBUG_NONE);

    /**
     * @brief Set the debug level for this instance
     *
     * @param level Debug level to set
     */
    void setDebugLevel(DebugLevel level) {
        _debugLevel = level;
    }

    /**
     * @brief Get the current debug level
     *
     * @return DebugLevel Current debug level
     */
    DebugLevel getDebugLevel() const {
        return _debugLevel;
    }

    /**
     * @brief Initialize the NFC hardware
     *
     * @return true if initialization was successful
     * @return false if initialization failed
     */
    bool initialize();

    /**
     * @brief Detect if a DESFire card is present in the field
     *
     * @return true if a card was detected
     * @return false if no card was detected
     */
    bool detectCard();

    /**
     * @brief Get the UID of the currently selected card
     *
     * @param uid Buffer to store the UID (should be at least 7 bytes)
     * @param uidLength Pointer to variable that will store the UID length
     * @return bool true if successful, false if unsuccessful
     */
    bool getCardUID(uint8_t* uid, uint8_t* uidLength);

    /**
     * @brief Get version information from the card
     *
     * @return DesfireStatus Status code of the operation
     */
    DesfireStatus getVersion();

    /**
     * @brief Get version information from the card
     *
     * @param version Pointer to a DESFireCardVersion struct to store version info
     * @return DesfireStatus Status code of the operation
     */
    DesfireStatus getVersion(DESFireCardVersion* version);

    /**
     * @brief Select a DESFire application by its ID
     *
     * @param aid Application ID (3 bytes)
     * @return DesfireStatus Status code of the operation
     */
    DesfireStatus selectApplication(uint8_t* aid);

    /**
     * @brief Authenticate with the specified key
     *
     * @param keyNo Key number to authenticate with
     * @param key Pointer to the key data
     * @param keySize Size of the key in bytes
     * @return DesfireStatus Status code of the operation
     */
    DesfireStatus authenticate(uint8_t keyNo, const uint8_t* key, uint8_t keySize);

    /**
     * @brief Format the PICC (card)
     *
     * @return DesfireStatus Status code of the operation
     * @note This requires authentication with the PICC master key
     */
    DesfireStatus formatPICC();

    /**
     * @brief Get free memory available on the card
     *
     * @param freeMemory Pointer to store the free memory in bytes
     * @return DesfireStatus Status code of the operation
     */
    DesfireStatus getFreeMem(uint32_t* freeMemory);

    /**
     * @brief Get application IDs present on card
     *
     * @param appIds Buffer to store the application IDs (3 bytes each)
     * @param maxCount Maximum number of application IDs to retrieve
     * @param count Reference to variable that will store the actual count
     * @return DesfireStatus Status code of the operation
     */
    DesfireStatus getApplicationIDs(uint8_t* appIds, uint8_t maxCount, uint8_t& count);

    /**
     * @brief Create a new application on the card
     *
     * @param aid Application ID (3 bytes)
     * @param keySettings Key settings for the application
     * @param numKeys Number of keys in the application (1-14)
     * @return DesfireStatus Status code of the operation
     */
    DesfireStatus createApplication(const uint8_t* aid, uint8_t keySettings, uint8_t numKeys);

    /**
     * @brief Delete an application from the card
     *
     * @param aid Application ID (3 bytes)
     * @return DesfireStatus Status code of the operation
     * @note This requires authentication with the PICC master key
     */
    DesfireStatus deleteApplication(const uint8_t* aid);

    /**
     * @brief Change key settings for the current application
     *
     * @param keySettings New key settings
     * @return DesfireStatus Status code of the operation
     * @note This requires authentication with the application master key
     */
    DesfireStatus changeKeySettings(uint8_t keySettings);

    /**
     * @brief Get key settings for the current application
     *
     * @param keySettings Reference to variable that will store the key settings
     * @param maxKeyNo Reference to variable that will store the maximum key number
     * @return DesfireStatus Status code of the operation
     */
    DesfireStatus getKeySettings(uint8_t& keySettings, uint8_t& maxKeyNo);

    /**
     * @brief Change a key in the current application
     *
     * @param keyNo Key number to change
     * @param newKey Pointer to the new key data
     * @param newKeySize Size of the new key in bytes
     * @param oldKey Pointer to the old key data (required for non-master keys)
     * @param oldKeySize Size of the old key in bytes
     * @return DesfireStatus Status code of the operation
     * @note For changing the master key (0), authentication with the master key is required
     */
    DesfireStatus changeKey(uint8_t        keyNo,
                            const uint8_t* newKey,
                            uint8_t        newKeySize,
                            const uint8_t* oldKey     = nullptr,
                            uint8_t        oldKeySize = 0);

    /**
     * @brief Get the key version for a specific key
     *
     * @param keyNo Key number
     * @param version Reference to variable that will store the key version
     * @return DesfireStatus Status code of the operation
     */
    DesfireStatus getKeyVersion(uint8_t keyNo, uint8_t& version);

    /**
     * @brief Get the file IDs in the current application
     *
     * @param fileIds Buffer to store the file IDs
     * @param maxCount Maximum number of file IDs to retrieve
     * @param count Reference to variable that will store the actual count
     * @return DesfireStatus Status code of the operation
     */
    DesfireStatus getFileIDs(uint8_t* fileIds, uint8_t maxCount, uint8_t& count);

    /**
     * @brief Get file settings for a specific file
     *
     * @param fileNo File number
     * @param fileType Reference to variable that will store the file type
     * @param commMode Reference to variable that will store the communication mode
     * @param accessRights Reference to variable that will store the access rights
     * @param fileSize Reference to variable that will store the file size
     * @return DesfireStatus Status code of the operation
     */
    DesfireStatus getFileSettings(uint8_t                  fileNo,
                                  DesfreFileType&          fileType,
                                  DesfreCommunicationMode& commMode,
                                  uint16_t&                accessRights,
                                  uint32_t&                fileSize);

    /**
     * @brief Change file settings for a specific file
     *
     * @param fileNo File number
     * @param commMode New communication mode
     * @param accessRights New access rights
     * @return DesfireStatus Status code of the operation
     */
    DesfireStatus changeFileSettings(uint8_t                 fileNo,
                                     DesfreCommunicationMode commMode,
                                     uint16_t                accessRights);

    /**
     * @brief Create a standard data file
     *
     * @param fileNo File number
     * @param commMode Communication mode
     * @param accessRights Access rights
     * @param fileSize File size in bytes
     * @return DesfireStatus Status code of the operation
     */
    DesfireStatus createStdDataFile(uint8_t                 fileNo,
                                    DesfreCommunicationMode commMode,
                                    uint16_t                accessRights,
                                    uint32_t                fileSize);

    /**
     * @brief Create a backup data file
     *
     * @param fileNo File number
     * @param commMode Communication mode
     * @param accessRights Access rights
     * @param fileSize File size in bytes
     * @return DesfireStatus Status code of the operation
     */
    DesfireStatus createBackupFile(uint8_t                 fileNo,
                                   DesfreCommunicationMode commMode,
                                   uint16_t                accessRights,
                                   uint32_t                fileSize);

    /**
     * @brief Create a value file
     *
     * @param fileNo File number
     * @param commMode Communication mode
     * @param accessRights Access rights
     * @param lowerLimit Lower limit for the value
     * @param upperLimit Upper limit for the value
     * @param initialValue Initial value
     * @param limitedCreditEnabled Whether limited credit is enabled
     * @return DesfireStatus Status code of the operation
     */
    DesfireStatus createValueFile(uint8_t                 fileNo,
                                  DesfreCommunicationMode commMode,
                                  uint16_t                accessRights,
                                  int32_t                 lowerLimit,
                                  int32_t                 upperLimit,
                                  int32_t                 initialValue,
                                  bool                    limitedCreditEnabled);

    /**
     * @brief Create a linear record file
     *
     * @param fileNo File number
     * @param commMode Communication mode
     * @param accessRights Access rights
     * @param recordSize Size of each record in bytes
     * @param maxRecords Maximum number of records
     * @return DesfireStatus Status code of the operation
     */
    DesfireStatus createLinearRecordFile(uint8_t                 fileNo,
                                         DesfreCommunicationMode commMode,
                                         uint16_t                accessRights,
                                         uint32_t                recordSize,
                                         uint32_t                maxRecords);

    /**
     * @brief Create a cyclic record file
     *
     * @param fileNo File number
     * @param commMode Communication mode
     * @param accessRights Access rights
     * @param recordSize Size of each record in bytes
     * @param maxRecords Maximum number of records
     * @return DesfireStatus Status code of the operation
     */
    DesfireStatus createCyclicRecordFile(uint8_t                 fileNo,
                                         DesfreCommunicationMode commMode,
                                         uint16_t                accessRights,
                                         uint32_t                recordSize,
                                         uint32_t                maxRecords);

    /**
     * @brief Delete a file from the current application
     *
     * @param fileNo File number
     * @return DesfireStatus Status code of the operation
     */
    DesfireStatus deleteFile(uint8_t fileNo);

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
    DesfireStatus readData(uint8_t   fileNo,
                           uint32_t  offset,
                           uint32_t  length,
                           uint8_t*  data,
                           uint32_t& bytesRead);

    /**
     * @brief Write data to a standard or backup file
     *
     * @param fileNo File number
     * @param offset Offset in bytes
     * @param length Length of data to write
     * @param data Data to write
     * @return DesfireStatus Status code of the operation
     */
    DesfireStatus writeData(uint8_t fileNo, uint32_t offset, uint32_t length, const uint8_t* data);

    /**
     * @brief Get value from a value file
     *
     * @param fileNo File number
     * @param value Reference to variable that will store the value
     * @return DesfireStatus Status code of the operation
     */
    DesfireStatus getValue(uint8_t fileNo, int32_t& value);

    /**
     * @brief Credit a value file
     *
     * @param fileNo File number
     * @param value Amount to credit
     * @return DesfireStatus Status code of the operation
     */
    DesfireStatus credit(uint8_t fileNo, int32_t value);

    /**
     * @brief Debit a value file
     *
     * @param fileNo File number
     * @param value Amount to debit
     * @return DesfireStatus Status code of the operation
     */
    DesfireStatus debit(uint8_t fileNo, int32_t value);

    /**
     * @brief Limited credit to a value file
     *
     * @param fileNo File number
     * @param value Amount to credit
     * @return DesfireStatus Status code of the operation
     */
    DesfireStatus limitedCredit(uint8_t fileNo, int32_t value);

    /**
     * @brief Write record to a record file
     *
     * @param fileNo File number
     * @param offset Offset in records (ignored for cyclic files)
     * @param length Length of data to write
     * @param data Data to write
     * @return DesfireStatus Status code of the operation
     */
    DesfireStatus writeRecord(uint8_t        fileNo,
                              uint32_t       offset,
                              uint32_t       length,
                              const uint8_t* data);

    /**
     * @brief Read records from a record file
     *
     * @param fileNo File number
     * @param recordOffset Record offset
     * @param recordCount Number of records to read
     * @param data Buffer to store the read data
     * @param bytesRead Reference to variable that will store the actual bytes read
     * @return DesfireStatus Status code of the operation
     */
    DesfireStatus readRecords(uint8_t   fileNo,
                              uint32_t  recordOffset,
                              uint32_t  recordCount,
                              uint8_t*  data,
                              uint32_t& bytesRead);

    /**
     * @brief Clear a record file
     *
     * @param fileNo File number
     * @return DesfireStatus Status code of the operation
     */
    DesfireStatus clearRecordFile(uint8_t fileNo);

    /**
     * @brief Commit transaction
     *
     * @return DesfireStatus Status code of the operation
     */
    DesfireStatus commitTransaction();

    /**
     * @brief Abort transaction
     *
     * @return DesfireStatus Status code of the operation
     */
    DesfireStatus abortTransaction();

private:
    /** Reference to the NFC reader implementation */
    NFCReaderInterface& _reader;

    /** UID of the detected card */
    uint8_t _uid[10];

    /** Length of the detected card UID */
    uint8_t _uidLength;

    /** Flag indicating if a card has been detected */
    bool _cardDetected;

    /** Current session key after authentication */
    uint8_t _sessionKey[24];

    /** Flag indicating if authentication was successful */
    bool _authenticated;

    /** Current cryptographic mode (DES, 3DES, AES) */
    DesfreCryptoMode _cryptoMode;

    /** Buffer for APDU construction */
    uint8_t _apduBuffer[ISO7816Constants::ISO_MAX_APDU_SIZE];

    /** Buffer for card responses */
    uint8_t _responseBuffer[ISO7816Constants::ISO_MAX_APDU_SIZE];

    /** Debug level */
    DebugLevel _debugLevel;

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
    DesfireStatus handleMultiFrameTransfer(DesfireCommand command,
                                           const uint8_t* data,
                                           uint8_t        dataLen,
                                           uint8_t*       response,
                                           uint16_t&      responseLen,
                                           uint16_t       maxResponseLen);

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
    DesfireStatus transmit(DesfireCommand command,
                           const uint8_t* data,
                           uint8_t        dataLen,
                           uint8_t*       response,
                           uint16_t&      responseLen);

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
    uint16_t buildAPDU(ISO7816Class       cla,
                       ISO7816Instruction ins,
                       uint8_t            p1,
                       uint8_t            p2,
                       const uint8_t*     data,
                       uint8_t            dataLen,
                       uint8_t            le,
                       uint8_t*           apdu);

    // Helper methods for debugging
    void debugPrintHex(const char* prefix, const uint8_t* data, uint16_t length);
    void debugPrintError(const char* message, DesfireStatus status);
    void debugPrintInfo(const char* message);
    void debugPrintVerbose(const char* message);
};

#endif  // DESFIRE_NFC_H