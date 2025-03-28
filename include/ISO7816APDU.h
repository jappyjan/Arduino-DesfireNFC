/**
 * @file ISO7816APDU.h
 * @brief ISO 7816-4 APDU command and response handling
 *
 * This file defines the ISO7816APDU class for handling ISO 7816-4 protocol
 * operations, including command building and response parsing.
 */

#ifndef ISO7816_APDU_H
#define ISO7816_APDU_H

#include <Arduino.h>
#include "DesfireStatus.h"
#include "ISO7816Constants.h"

/**
 * @brief Maximum size for APDU command data
 */
#define ISO7816_MAX_DATA_LENGTH 255

/**
 * @brief Maximum size for APDU response data
 */
#define ISO7816_MAX_RESPONSE_LENGTH 255

/**
 * @brief Class for handling ISO 7816-4 APDU operations
 */
class ISO7816APDU {
public:
    /**
     * @brief Structure to hold APDU command data
     */
    struct Command {
        uint8_t cla;                                        ///< Class byte
        uint8_t ins;                                        ///< Instruction byte
        uint8_t p1;                                         ///< Parameter 1
        uint8_t p2;                                         ///< Parameter 2
        uint8_t data[ISO7816Constants::ISO_MAX_DATA_SIZE];  ///< Command data
        uint8_t dataLength;                                 ///< Length of command data
        uint8_t le;                                         ///< Expected response length
    };

    /**
     * @brief Structure to hold APDU response data
     */
    struct Response {
        uint8_t  data[ISO7816Constants::ISO_MAX_DATA_SIZE];  ///< Response data
        uint8_t  dataLength;                                 ///< Length of response data
        uint16_t status;                                     ///< Status word (SW1-SW2)
    };

    /**
     * @brief Build an ISO 7816-4 APDU command
     *
     * @param cmd Command structure containing the APDU parameters
     * @param apdu Buffer to store the constructed APDU
     * @param apduLength Pointer to variable that will store the APDU length
     * @return DesfireStatus Status code of the operation
     */
    static DesfireStatus buildCommand(const Command& cmd, uint8_t* apdu, uint8_t* apduLength);

    /**
     * @brief Parse an ISO 7816-4 APDU response
     *
     * @param response Buffer containing the APDU response
     * @param responseLength Length of the response buffer
     * @param result Response structure to store the parsed data
     * @return DesfireStatus Status code of the operation
     */
    static DesfireStatus parseResponse(const uint8_t* response,
                                       uint8_t        responseLength,
                                       Response&      result);

    /**
     * @brief Check if a response indicates success
     *
     * @param status Status word (SW1-SW2)
     * @return true if the status indicates success
     * @return false if the status indicates an error
     */
    static bool isSuccess(uint16_t status);

    /**
     * @brief Convert ISO 7816-4 status words to DESFire status code
     *
     * @param status Status word (SW1-SW2)
     * @return DesfireStatus Corresponding DESFire status code
     */
    static DesfireStatus convertStatus(uint16_t status);

private:
    /**
     * @brief Check if a command is valid
     *
     * @param cmd Command structure to validate
     * @return true if the command is valid
     * @return false if the command is invalid
     */
    static bool validateCommand(const Command& cmd);
};

#endif  // ISO7816_APDU_H