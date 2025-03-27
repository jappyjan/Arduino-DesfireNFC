/**
 * @file PN532Reader.h
 * @brief PN532 implementation of the NFC reader interface
 * 
 * This file implements the NFCReaderInterface for the PN532 NFC controller.
 */

#ifndef PN532_READER_H
#define PN532_READER_H

#include <Adafruit_PN532.h>
#include "NFCReaderInterface.h"

/**
 * @brief NFC reader implementation for PN532
 * 
 * This class implements the NFCReaderInterface for the PN532 NFC controller.
 */
class PN532Reader : public NFCReaderInterface {
public:
    /**
     * @brief Construct a new PN532Reader object with I2C communication
     * 
     * @param irq IRQ pin connected to the PN532
     * @param reset Reset pin connected to the PN532
     * @param wire Reference to the Wire I2C instance
     */
    PN532Reader(uint8_t irq, uint8_t reset, TwoWire &wire = Wire);
    
    /**
     * @brief Construct a new PN532Reader object with SPI communication
     * 
     * @param ss Slave select (CS) pin connected to the PN532
     */
    PN532Reader(uint8_t ss);
    
    /**
     * @brief Construct a new PN532Reader object with UART (HSU) communication
     * 
     * @param tx TX pin connected to the PN532
     * @param rx RX pin connected to the PN532
     */
    PN532Reader(uint8_t tx, uint8_t rx);
    
    /**
     * @brief Destroy the PN532Reader object
     */
    virtual ~PN532Reader();
    
    /**
     * @brief Initialize the PN532 NFC reader
     * 
     * @return true if initialization was successful
     * @return false if initialization failed
     */
    virtual bool begin() override;
    
    /**
     * @brief Get the firmware version of the PN532
     * 
     * @return uint32_t Version information (0 if failed)
     */
    virtual uint32_t getFirmwareVersion() override;
    
    /**
     * @brief Configure the PN532 for card communication
     * 
     * @return true if configuration was successful
     * @return false if configuration failed
     */
    virtual bool configure() override;
    
    /**
     * @brief Detect if an ISO14443A card is present
     * 
     * @param uid Buffer to store the card UID
     * @param uidLength Pointer to variable that will store the UID length
     * @return true if a card was detected
     * @return false if no card was detected
     */
    virtual bool detectCard(uint8_t *uid, uint8_t *uidLength) override;
    
    /**
     * @brief Transmit data to the card and receive a response
     * 
     * @param txData Data to transmit
     * @param txLength Length of data to transmit
     * @param rxData Buffer to store the response
     * @param rxLength Pointer to variable that will store the response length
     * @return true if transmission was successful
     * @return false if transmission failed
     */
    virtual bool transceive(const uint8_t *txData, uint8_t txLength, 
                           uint8_t *rxData, uint8_t *rxLength) override;
                           
    /**
     * @brief Get direct access to the underlying Adafruit_PN532 object
     * 
     * @return Adafruit_PN532* Pointer to the Adafruit_PN532 object
     */
    Adafruit_PN532* getPN532() { return _nfc; }

private:
    Adafruit_PN532 *_nfc;    // PN532 controller
    bool _ownNFC;            // Whether we created the _nfc instance
    uint8_t _connectionType; // Type of connection (0=I2C, 1=SPI, 2=HSU)
};

#endif // PN532_READER_H 