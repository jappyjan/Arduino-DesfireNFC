/**
 * @file PN532Reader.cpp
 * @brief Implementation of the PN532Reader class
 */

#include "PN532Reader.h"

// Constants
#define PN532_CONN_I2C 0
#define PN532_CONN_SPI 1
#define PN532_CONN_HSU 2

/**
 * @brief Construct a new PN532Reader object with I2C communication using default pins
 *
 * This constructor matches the style used in fabreader3
 */
PN532Reader::PN532Reader() {
    // Create PN532 with default pins (PN532_IRQ, PN532_RESET)
    _nfc            = new Adafruit_PN532(PN532_IRQ, PN532_RESET);
    _ownNFC         = true;
    _connectionType = PN532_CONN_I2C;

#if defined(ESP32) || defined(ESP8266)
    // Allow more time for the PN532 to initialize on ESP32
    delay(200);
#endif
}

/**
 * @brief Construct a new PN532Reader object with an existing Adafruit_PN532 instance
 *
 * @param nfc Existing PN532 instance that has already been initialized
 */
PN532Reader::PN532Reader(Adafruit_PN532* nfc) {
    // Use the provided PN532 instance
    _nfc            = nfc;
    _ownNFC         = false;  // We don't own this instance, so don't delete it in the destructor
    _connectionType = PN532_CONN_I2C;  // Assume I2C connection
}

/**
 * @brief Construct a new PN532Reader object with I2C communication
 *
 * @param irq IRQ pin connected to the PN532
 * @param reset Reset pin connected to the PN532
 * @param wire Reference to the Wire I2C instance
 */
PN532Reader::PN532Reader(uint8_t irq, uint8_t reset, TwoWire& wire) {
    _nfc            = new Adafruit_PN532(irq, reset, &wire);
    _ownNFC         = true;
    _connectionType = PN532_CONN_I2C;

#if defined(ESP32) || defined(ESP8266)
    // Allow more time for the PN532 to initialize on ESP32
    delay(200);
#endif
}

/**
 * @brief Construct a new PN532Reader object with SPI communication
 *
 * @param ss Slave select (CS) pin connected to the PN532
 */
PN532Reader::PN532Reader(uint8_t ss) {
    _nfc            = new Adafruit_PN532(ss);
    _ownNFC         = true;
    _connectionType = PN532_CONN_SPI;
}

/**
 * @brief Construct a new PN532Reader object with UART (HSU) communication
 *
 * @param tx TX pin connected to the PN532
 * @param rx RX pin connected to the PN532
 */
PN532Reader::PN532Reader(uint8_t tx, uint8_t rx) {
    _nfc            = new Adafruit_PN532(tx, rx);
    _ownNFC         = true;
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
    // For ESP32, use a longer delay before initialization to stabilize I2C
#if defined(ESP32) || defined(ESP8266)
    delay(100);
#endif

    // Initialize the PN532
    _nfc->begin();

    // Add retry mechanism for better reliability
    uint32_t versiondata = 0;
    int      retries     = 5;  // Try up to 5 times for ESP32 (was 3)

    while (retries > 0) {
#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
        // Output debug info
        Serial.print("Attempt ");
        Serial.print(6 - retries);
        Serial.print(" to get firmware version... ");
#endif

        versiondata = _nfc->getFirmwareVersion();
        if (versiondata) {
#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
            Serial.println("Success!");
            // Print firmware version details
            Serial.print("Found chip PN5");
            Serial.print((versiondata >> 24) & 0xFF, HEX);
            Serial.print(", Firmware ver. ");
            Serial.print((versiondata >> 16) & 0xFF, DEC);
            Serial.print('.');
            Serial.println((versiondata >> 8) & 0xFF, DEC);
#endif
            break;  // Successfully got firmware version
        }

#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
        Serial.println("Failed.");
#endif

        // Wait between retries with increasing delay
        int delay_time = 100 * (6 - retries);
#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
        Serial.print("Waiting ");
        Serial.print(delay_time);
        Serial.println("ms before retry...");
#endif
        delay(delay_time);
        retries--;
    }

    if (!versiondata) {
#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
        Serial.println("Failed to find PN53x board after multiple attempts");
#endif
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
    // Add retry mechanism for SAMConfig
    int  retries = 3;  // Try up to 3 times
    bool success = false;

    while (retries > 0) {
        success = _nfc->SAMConfig();
        if (success) {
            break;  // Successfully configured
        }

        // Wait between retries with increasing delay
        delay(100 * (4 - retries));
        retries--;
    }

    if (!success) {
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
bool PN532Reader::detectCard(uint8_t* uid, uint8_t* uidLength) {
    // Add retry mechanism for better reliability with card detection
    int  retries = 2;  // Try up to 2 times
    bool success = false;

    while (retries > 0) {
        success = _nfc->readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, uidLength, 1000);
        if (success) {
            break;  // Successfully detected card
        }

        // Wait between retries
        delay(50);
        retries--;
    }

    return success;
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
bool PN532Reader::transceive(const uint8_t* txData,
                             uint16_t       txLength,
                             uint8_t*       rxData,
                             uint16_t*      rxLength) {
    // The PN532 library expects non-const data for sending, so we need to cast away const
    // Note: We need to cast rxLength to uint8_t* for compatibility with the underlying library
    // This is safe as long as the rxLength value doesn't exceed 255
    uint8_t rxLen8 = *rxLength > 255 ? 255 : static_cast<uint8_t>(*rxLength);

    // Add retry mechanism for better reliability with transceive
    int  retries = 2;  // Try up to 2 times
    bool success = false;

    while (retries > 0) {
        success = _nfc->inDataExchange(const_cast<uint8_t*>(txData),
                                       static_cast<uint8_t>(txLength > 255 ? 255 : txLength),
                                       rxData,
                                       &rxLen8);
        if (success) {
            break;  // Successfully exchanged data
        }

        // Wait between retries
        delay(50);
        retries--;
    }

    *rxLength = rxLen8;
    return success;
}