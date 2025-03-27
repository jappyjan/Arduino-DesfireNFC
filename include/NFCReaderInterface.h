/**
 * @file NFCReaderInterface.h
 * @brief Low-level NFC reader interface
 * 
 * This file defines the interface for low-level NFC reader operations.
 * It abstracts the specific NFC reader hardware and provides a consistent
 * interface for the higher-level DESFire protocol operations.
 */

#ifndef NFC_READER_INTERFACE_H
#define NFC_READER_INTERFACE_H

#include <Arduino.h>

/**
 * @brief Interface for NFC reader operations
 * 
 * This abstract class defines the interface for low-level NFC reader operations.
 * Concrete implementations will be provided for specific NFC reader hardware.
 */
class NFCReaderInterface {
public:
    /**
     * @brief Virtual destructor
     */
    virtual ~NFCReaderInterface() {}
    
    /**
     * @brief Initialize the NFC reader
     * 
     * @return true if initialization was successful
     * @return false if initialization failed
     */
    virtual bool begin() = 0;
    
    /**
     * @brief Get the firmware version of the NFC reader
     * 
     * @return uint32_t Version information (0 if failed)
     */
    virtual uint32_t getFirmwareVersion() = 0;
    
    /**
     * @brief Configure the NFC reader for card communication
     * 
     * @return true if configuration was successful
     * @return false if configuration failed
     */
    virtual bool configure() = 0;
    
    /**
     * @brief Detect if an ISO14443A card is present
     * 
     * @param uid Buffer to store the card UID
     * @param uidLength Pointer to variable that will store the UID length
     * @return true if a card was detected
     * @return false if no card was detected
     */
    virtual bool detectCard(uint8_t *uid, uint8_t *uidLength) = 0;
    
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
                           uint8_t *rxData, uint8_t *rxLength) = 0;
};

#endif // NFC_READER_INTERFACE_H 