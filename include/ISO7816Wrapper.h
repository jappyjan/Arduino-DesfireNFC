/**
 * @file ISO7816Wrapper.h
 * @brief ISO 7816-4 APDU wrapping functionality for DESFire cards
 *
 * This file provides enhanced ISO 7816-4 APDU wrapping functionality for
 * DESFire cards, including support for multi-frame commands, extended APDUs,
 * and proper response handling.
 */

#ifndef ISO7816_WRAPPER_H
#define ISO7816_WRAPPER_H

#include <Arduino.h>
#include "DesfireStatus.h"
#include "DesfireTypes.h"
#include "ISO7816APDU.h"
#include "ISO7816Constants.h"
#include "NFCReaderInterface.h"

/**
 * @brief Class for ISO 7816-4 APDU wrapping functionality
 */
class ISO7816Wrapper {
public:
    /**
     * @brief Construct a new ISO7816Wrapper object
     *
     * @param reader Reference to NFC reader interface
     */
    ISO7816Wrapper(NFCReaderInterface& reader);

    /**
     * @brief Transmit a DESFire native command wrapped in ISO 7816-4 APDU
     *
     * @param command DESFire command code
     * @param data Command data (if any)
     * @param dataLen Length of command data
     * @param response Buffer to store response
     * @param responseLen Reference to variable that will hold response length
     * @return DesfireStatus Status of the operation
     */
    DesfireStatus transmitCommand(DesfireCommand command,
                                  const uint8_t* data,
                                  uint8_t        dataLen,
                                  uint8_t*       response,
                                  uint16_t&      responseLen);

    /**
     * @brief Transmit an ISO 7816-4 APDU directly
     *
     * @param apdu APDU buffer
     * @param apduLen Length of APDU
     * @param response Buffer to store response
     * @param responseLen Reference to variable that will hold response length
     * @return DesfireStatus Status of the operation
     */
    DesfireStatus transmitAPDU(const uint8_t* apdu,
                               uint16_t       apduLen,
                               uint8_t*       response,
                               uint16_t&      responseLen);

    /**
     * @brief Build a DESFire native command wrapped in ISO 7816-4 APDU
     *
     * @param command DESFire command code
     * @param data Command data (if any)
     * @param dataLen Length of command data
     * @param apdu Buffer to store the APDU
     * @param apduLen Reference to variable that will hold APDU length
     * @return DesfireStatus Status of the operation
     */
    DesfireStatus buildWrappedCommand(DesfireCommand command,
                                      const uint8_t* data,
                                      uint8_t        dataLen,
                                      uint8_t*       apdu,
                                      uint16_t&      apduLen);

    /**
     * @brief Build an ISO 7816-4 select application APDU
     *
     * @param aid Application ID (3 bytes)
     * @param apdu Buffer to store the APDU
     * @param apduLen Reference to variable that will hold APDU length
     * @return DesfireStatus Status of the operation
     */
    DesfireStatus buildSelectApplicationAPDU(const uint8_t* aid, uint8_t* apdu, uint16_t& apduLen);

    /**
     * @brief Handle a multi-frame DESFire response
     *
     * @param initialResponse Initial response buffer
     * @param initialResponseLen Length of initial response
     * @param completeResponse Buffer to store complete response
     * @param completeResponseLen Reference to variable that will hold complete response length
     * @return DesfireStatus Status of the operation
     */
    DesfireStatus handleMultiFrameResponse(const uint8_t* initialResponse,
                                           uint16_t       initialResponseLen,
                                           uint8_t*       completeResponse,
                                           uint16_t&      completeResponseLen);

    /**
     * @brief Parse an ISO 7816-4 response
     *
     * @param response Response buffer
     * @param responseLen Length of response
     * @param data Buffer to store response data
     * @param dataLen Reference to variable that will hold data length
     * @param statusWord Reference to variable that will hold status word
     * @return DesfireStatus Status of the operation
     */
    DesfireStatus parseResponse(const uint8_t* response,
                                uint16_t       responseLen,
                                uint8_t*       data,
                                uint16_t&      dataLen,
                                uint16_t&      statusWord);

    /**
     * @brief Check if a response indicates more frames are available
     *
     * @param statusWord Status word from response
     * @return true if more frames are available
     * @return false if no more frames are available
     */
    bool hasMoreFrames(uint16_t statusWord) const;

    /**
     * @brief Get additional frame from card
     *
     * @param response Buffer to store response
     * @param responseLen Reference to variable that will hold response length
     * @return DesfireStatus Status of the operation
     */
    DesfireStatus getAdditionalFrame(uint8_t* response, uint16_t& responseLen);

private:
    /** Reference to the NFC reader implementation */
    NFCReaderInterface& _reader;

    /** Buffer for APDU construction */
    uint8_t _apduBuffer[ISO7816Constants::ISO_MAX_APDU_SIZE];

    /** Status word from last response */
    uint16_t _lastStatusWord;

    /**
     * @brief Convert ISO 7816-4 status word to DESFire status
     *
     * @param statusWord Status word
     * @return DesfireStatus Corresponding DESFire status
     */
    DesfireStatus convertStatusWord(uint16_t statusWord) const;
};

#endif  // ISO7816_WRAPPER_H