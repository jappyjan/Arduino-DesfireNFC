/**
 * @file PN532Reader.cpp
 * @brief Implementation of the PN532Reader class
 */

#include "PN532Reader.h"

// Constants
#define PN532_CONN_I2C 0
#define PN532_CONN_SPI 1
#define PN532_CONN_HSU 2

#define PN532_MIFARE_ISO14443A 0x00  // Card type for readPassiveTargetID

/**
 * @brief Construct a new PN532Reader object with I2C communication
 * 
 * @param irq IRQ pin connected to the PN532
 * @param reset Reset pin connected to the PN532
 * @param wire Reference to the Wire I2C instance
 */
PN532Reader::PN532Reader(uint8_t irq, uint8_t reset, TwoWire &wire) {
    _nfc = new Adafruit_PN532(irq, reset, &wire);
    _ownNFC = true;
    _connectionType = PN532_CONN_I2C;
}

/**
 * @brief Construct a new PN532Reader object with SPI communication
 * 
 * @param ss Slave select (CS) pin connected to the PN532
 */
PN532Reader::PN532Reader(uint8_t ss) {
    _nfc = new Adafruit_PN532(ss);
    _ownNFC = true;
    _connectionType = PN532_CONN_SPI;
}

/**
 * @brief Construct a new PN532Reader object with UART (HSU) communication
 * 
 * @param tx TX pin connected to the PN532
 * @param rx RX pin connected to the PN532
 */
PN532Reader::PN532Reader(uint8_t tx, uint8_t rx) {
    _nfc = new Adafruit_PN532(tx, rx);
    _ownNFC = true;
    _connectionType = PN532_CONN_HSU;
}

/**
 * @brief Destroy the PN532Reader object
 */
PN532Reader::~PN532Reader() {
    if (_ownNFC && _nfc) {
        delete _nfc;
        _nfc = nullptr;
    }
}

/**
 * @brief Initialize the PN532 NFC reader
 * 
 * @return true if initialization was successful
 * @return false if initialization failed
 */
bool PN532Reader::begin() {
    _nfc->begin();
    
    uint32_t versiondata = _nfc->getFirmwareVersion();
    if (!versiondata) {
        return false;
    }
    
    return true;
}

/**
 * @brief Get the firmware version of the PN532
 * 
 * @return uint32_t Version information (0 if failed)
 */
uint32_t PN532Reader::getFirmwareVersion() {
    return _nfc->getFirmwareVersion();
}

/**
 * @brief Configure the PN532 for card communication
 * 
 * @return true if configuration was successful
 * @return false if configuration failed
 */
bool PN532Reader::configure() {
    if (!_nfc->SAMConfig()) {
        return false;
    }
    
    // Set the max number of retry attempts to read from a card
    _nfc->setPassiveActivationRetries(0xFF);
    
    return true;
}

/**
 * @brief Detect if an ISO14443A card is present
 * 
 * @param uid Buffer to store the card UID
 * @param uidLength Pointer to variable that will store the UID length
 * @return true if a card was detected
 * @return false if no card was detected
 */
bool PN532Reader::detectCard(uint8_t *uid, uint8_t *uidLength) {
    return _nfc->readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, uidLength);
}

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
bool PN532Reader::transceive(const uint8_t *txData, uint8_t txLength, 
                           uint8_t *rxData, uint8_t *rxLength) {
    // The PN532 library expects non-const data for sending, so we need to cast away const
    return _nfc->inDataExchange(const_cast<uint8_t*>(txData), txLength, rxData, rxLength);
} 