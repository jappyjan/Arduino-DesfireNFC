# Hardware Abstraction Layer for DESFire NFC Library

This document describes the hardware abstraction layer (HAL) used in the Arduino-DesfireNFC library.

## Overview

The hardware abstraction layer provides a consistent interface for the DESFire protocol implementation to communicate with the NFC hardware, regardless of the specific hardware used. This allows the library to be used with different NFC readers without modifying the core protocol implementation.

## Architecture

The HAL consists of several layers:

1. **NFCReaderInterface**: An abstract interface that defines the operations required for NFC communication.
2. **PN532Reader**: A concrete implementation of the NFCReaderInterface for the PN532 NFC controller.
3. **PN532Interface**: A utility class that simplifies the use of the Adafruit PN532 library.

### NFCReaderInterface

The NFCReaderInterface is an abstract class that defines the following operations:

- `begin()`: Initialize the NFC reader
- `getFirmwareVersion()`: Get the firmware version of the NFC reader
- `configure()`: Configure the NFC reader for card communication
- `detectCard()`: Detect if an ISO14443A card is present
- `transceive()`: Transmit data to the card and receive a response

Any NFC reader implementation must implement these methods to be compatible with the library.

### PN532Reader

The PN532Reader is a concrete implementation of the NFCReaderInterface for the PN532 NFC controller. It uses the Adafruit PN532 library to communicate with the PN532 chip.

The PN532Reader supports different communication interfaces:

- I2C
- SPI
- UART (HSU)

The appropriate constructor is used depending on the desired communication interface.

### PN532Interface

The PN532Interface is a utility class that simplifies the use of the Adafruit PN532 library. It provides a factory method pattern for creating PN532 instances with different communication interfaces.

## Usage

### Creating a PN532Reader with I2C

```cpp
#include "PN532Reader.h"

// For I2C
#define PN532_IRQ   (2)
#define PN532_RESET (3)

// Create a PN532Reader with I2C
PN532Reader reader(PN532_IRQ, PN532_RESET);

void setup() {
  Serial.begin(115200);
  
  // Initialize the reader
  if (!reader.begin()) {
    Serial.println("Failed to initialize the NFC reader");
    while (1);
  }
  
  // Configure the reader
  if (!reader.configure()) {
    Serial.println("Failed to configure the NFC reader");
    while (1);
  }
  
  Serial.println("NFC reader initialized");
}

void loop() {
  uint8_t uid[7];
  uint8_t uidLength;
  
  // Detect a card
  if (reader.detectCard(uid, &uidLength)) {
    Serial.println("Card detected!");
    Serial.print("UID: ");
    for (uint8_t i = 0; i < uidLength; i++) {
      Serial.print(uid[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
    
    // Transceive data
    uint8_t txData[] = {0x90, 0x00, 0x00, 0x00, 0x00};
    uint8_t rxData[32];
    uint8_t rxLength = sizeof(rxData);
    
    if (reader.transceive(txData, sizeof(txData), rxData, &rxLength)) {
      Serial.println("Transceive successful");
      // Process the response
    } else {
      Serial.println("Transceive failed");
    }
  }
  
  delay(100);
}
```

### Creating a PN532Reader with SPI

```cpp
#include "PN532Reader.h"

// For SPI
#define PN532_SS    (10)

// Create a PN532Reader with SPI
PN532Reader reader(PN532_SS);

// Rest of the code is the same
```

### Creating a PN532Reader with UART (HSU)

```cpp
#include "PN532Reader.h"

// For UART (HSU)
#define PN532_TX    (2)
#define PN532_RX    (3)

// Create a PN532Reader with UART
PN532Reader reader(PN532_TX, PN532_RX);

// Rest of the code is the same
```

## Future Expansion

The hardware abstraction layer is designed to be easily extended to support additional NFC reader hardware. To add support for a new reader:

1. Create a new class that implements the NFCReaderInterface
2. Implement the required methods for the new hardware
3. Use the new class in place of the PN532Reader

This design ensures that the core DESFire protocol implementation remains unchanged when adding support for new hardware.

## ISO14443A Protocol Layer

The ISO14443A protocol layer is implemented within the PN532Reader class, leveraging the capabilities of the PN532 chip. The PN532 handles the low-level ISO14443A protocol details, including:

- Card activation
- Anti-collision
- UID retrieval
- APDU exchange

The PN532Reader class provides a simplified interface for these operations, making them accessible to the higher-level DESFire protocol implementation. 