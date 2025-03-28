/**
 * @file PN532Interface.cpp
 * @brief Implementation of the hardware abstraction layer for PN532 NFC reader
 */

#include "PN532Interface.h"

/**
 * @brief Private constructor for PN532Interface
 *
 * @param nfc Pointer to the Adafruit_PN532 instance
 */
PN532Interface::PN532Interface(Adafruit_PN532* nfc) {
    _nfc = nfc;
}

/**
 * @brief Create a PN532Interface using I2C communication
 *
 * @param irq IRQ pin connected to the PN532
 * @param reset Reset pin connected to the PN532
 * @param wire Reference to the Wire I2C instance
 * @return PN532Interface* Pointer to the new interface instance
 */
PN532Interface* PN532Interface::createI2C(uint8_t irq, uint8_t reset, TwoWire& wire) {
    Adafruit_PN532* nfc = new Adafruit_PN532(irq, reset, &wire);
    return new PN532Interface(nfc);
}

/**
 * @brief Create a PN532Interface using SPI communication
 *
 * @param ss Slave select (CS) pin connected to the PN532
 * @return PN532Interface* Pointer to the new interface instance
 */
PN532Interface* PN532Interface::createSPI(uint8_t ss) {
    Adafruit_PN532* nfc = new Adafruit_PN532(ss);
    return new PN532Interface(nfc);
}

/**
 * @brief Create a PN532Interface using UART (HSU) communication
 *
 * @param tx TX pin connected to the PN532
 * @param rx RX pin connected to the PN532
 * @return PN532Interface* Pointer to the new interface instance
 */
PN532Interface* PN532Interface::createHSU(uint8_t tx, uint8_t rx) {
    Adafruit_PN532* nfc = new Adafruit_PN532(tx, rx);
    return new PN532Interface(nfc);
}

/**
 * @brief Initialize the PN532 NFC reader
 *
 * @return true if initialization was successful
 * @return false if initialization failed
 */
bool PN532Interface::begin() {
    _nfc->begin();

    uint32_t versiondata = _nfc->getFirmwareVersion();
    if (!versiondata) {
        return false;
    }

    return true;
}

/**
 * @brief Get the PN532 firmware version
 *
 * @return uint32_t Version information (0 if failed)
 */
uint32_t PN532Interface::getFirmwareVersion() {
    return _nfc->getFirmwareVersion();
}

/**
 * @brief Configure the SAM (Secure Access Module)
 *
 * @return true if configuration was successful
 * @return false if configuration failed
 */
bool PN532Interface::SAMConfig() {
    return _nfc->SAMConfig();
}

/**
 * @brief Set the number of passive activation retries
 *
 * @param retries Number of retries
 */
void PN532Interface::setPassiveActivationRetries(uint8_t retries) {
    _nfc->setPassiveActivationRetries(retries);
}

/**
 * @brief Read a passive target ID
 *
 * @param cardBaudRate Baud rate to use for the card
 * @param uid Buffer to store the UID
 * @param uidLength Pointer to store the UID length
 * @return true if a target was detected
 * @return false if no target was detected
 */
bool PN532Interface::readPassiveTargetID(uint8_t cardBaudRate, uint8_t* uid, uint8_t* uidLength) {
    return _nfc->readPassiveTargetID(cardBaudRate, uid, uidLength);
}

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
bool PN532Interface::inDataExchange(uint8_t* send,
                                    uint8_t  sendLength,
                                    uint8_t* response,
                                    uint8_t* responseLength) {
    return _nfc->inDataExchange(send, sendLength, response, responseLength);
}