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
    /**
     * @brief Construct a new DesfireNFC object
     *
     * @param reader Reference to an NFC reader implementation
     */
    DesfireNFC(NFCReaderInterface& reader);

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
     * @return true if the command was successful
     * @return false if the command failed
     */
    bool getVersion();

    /**
     * @brief Get version information from the card
     *
     * @param version Pointer to a DESFireCardVersion struct to store version info
     * @return true if the command was successful
     * @return false if the command failed
     */
    bool getVersion(DESFireCardVersion* version);

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
};

#endif  // DESFIRE_NFC_H