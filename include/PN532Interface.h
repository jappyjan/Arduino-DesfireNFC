/**
 * @file PN532Interface.h
 * @brief Hardware abstraction layer for PN532 NFC reader
 *
 * This file provides an abstraction layer for the PN532 NFC reader
 * to enable different communication interfaces (I2C, SPI, UART).
 */

#ifndef PN532_INTERFACE_H
#define PN532_INTERFACE_H

#include <Adafruit_PN532.h>
#include <Arduino.h>

/**
 * @brief Hardware abstraction layer for PN532 NFC reader
 *
 * This class provides a unified interface for communicating with the PN532
 * NFC reader regardless of the underlying physical connection (I2C, SPI, UART).
 */
class PN532Interface {
public:
    /**
     * @brief Construct a new PN532Interface using I2C communication
     *
     * @param irq IRQ pin connected to the PN532
     * @param reset Reset pin connected to the PN532
     * @param wire Reference to the Wire I2C instance
     */
    static PN532Interface* createI2C(uint8_t irq, uint8_t reset, TwoWire& wire = Wire);

    /**
     * @brief Construct a new PN532Interface using SPI communication
     *
     * @param ss Slave select (CS) pin connected to the PN532
     */
    static PN532Interface* createSPI(uint8_t ss);

    /**
     * @brief Construct a new PN532Interface using UART (HSU) communication
     *
     * @param tx TX pin connected to the PN532
     * @param rx RX pin connected to the PN532
     */
    static PN532Interface* createHSU(uint8_t tx, uint8_t rx);

    /**
     * @brief Initialize the PN532 NFC reader
     *
     * @return true if initialization was successful
     * @return false if initialization failed
     */
    bool begin();

    /**
     * @brief Get the PN532 firmware version
     *
     * @return uint32_t Version information (0 if failed)
     */
    uint32_t getFirmwareVersion();

    /**
     * @brief Configure the SAM (Secure Access Module)
     *
     * @return true if configuration was successful
     * @return false if configuration failed
     */
    bool SAMConfig();

    /**
     * @brief Set the number of passive activation retries
     *
     * @param retries Number of retries
     */
    void setPassiveActivationRetries(uint8_t retries);

    /**
     * @brief Read a passive target ID
     *
     * @param cardBaudRate Baud rate to use for the card
     * @param uid Buffer to store the UID
     * @param uidLength Pointer to store the UID length
     * @return true if a target was detected
     * @return false if no target was detected
     */
    bool readPassiveTargetID(uint8_t cardBaudRate, uint8_t* uid, uint8_t* uidLength);

    /**
     * @brief Exchange data with the card
     *
     * @param send Data to send
     * @param sendLength Length of data to send
     * @param response Buffer to store the response
     * @param responseLength Pointer to store the response length
     * @return true if exchange was successful
     * @return false if exchange failed
     */
    bool inDataExchange(uint8_t* send,
                        uint8_t  sendLength,
                        uint8_t* response,
                        uint8_t* responseLength);

private:
    // Actual PN532 instance
    Adafruit_PN532* _nfc;

    // Private constructor for factory methods
    PN532Interface(Adafruit_PN532* nfc);
};

#endif  // PN532_INTERFACE_H