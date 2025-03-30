# Arduino-DesfireNFC

A comprehensive library for MIFARE DESFire NFC cards communication with ESP32 microcontrollers. This library provides a complete implementation of the DESFire protocol, including authentication, secure messaging, and file operations.

## Features

- Support for DESFire EV1 and EV2 cards
- Multiple authentication methods (Legacy, ISO, AES)
- Secure messaging with encryption and MAC
- Complete application and file management
- Transaction mechanism support
- Comprehensive error handling and diagnostic tools
- Optimized for ESP32 hardware

## Dependencies

- [Adafruit PN532 Library](https://github.com/adafruit/Adafruit-PN532)
- Arduino framework for ESP32

## Installation

### Using PlatformIO

```
lib_deps = 
    username/Arduino-DesfireNFC
```

### Manual Installation

1. Download this repository as a ZIP file
2. In the Arduino IDE, go to Sketch -> Include Library -> Add .ZIP Library...
3. Select the downloaded ZIP file

## Getting Started

### Basic Example

```cpp
#include <Arduino.h>
#include <DesfireNFC.h>

// Initialize hardware abstraction for PN532
PN532_I2C pn532_i2c(Wire);
PN532 nfc(pn532_i2c);

// Initialize DESFire library with the hardware abstraction
DesfireNFC desfire(nfc);

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  
  Serial.println("Arduino-DesfireNFC Basic Example");
  
  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.println("Didn't find PN53x board");
    while (1) delay(10);
  }
  
  // Configure PN532
  nfc.SAMConfig();
  Serial.println("Waiting for a card...");
}

void loop() {
  if (desfire.detectCard()) {
    Serial.println("Card detected!");
    Serial.print("UID: ");
    
    uint8_t uid[7];
    uint8_t uidLength = desfire.getCardUID(uid, sizeof(uid));
    
    for (uint8_t i = 0; i < uidLength; i++) {
      Serial.print(uid[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
    
    // Get card version information
    DESFireCardVersion version;
    if (desfire.getVersion(&version)) {
      Serial.print("Card type: ");
      Serial.println(version.hardwareType);
      Serial.print("Software version: ");
      Serial.print(version.softwareVersionMajor);
      Serial.print(".");
      Serial.println(version.softwareVersionMinor);
    }
    
    delay(1000);
  }
  
  delay(100);
}
```

### Error Handling Example

The library includes comprehensive error handling with human-readable error messages and troubleshooting suggestions:

```cpp
#include <DesfireNFC.h>
#include <DesfireErrorHandler.h>

// Initialize your NFC hardware...
DesfireNFC desfire(nfc);

void example() {
  // Attempt an operation
  uint8_t nonExistentAID[3] = {0xFF, 0xFF, 0xFF};
  DesfireStatus status = desfire.selectApplication(nonExistentAID);
  
  // Handle the status code
  if (!DesfireErrorHandler::isSuccess(status)) {
    // Option 1: Get the error message
    Serial.print("Error: ");
    Serial.println(DesfireErrorHandler::getErrorMessage(status));
    
    // Option 2: Print a detailed error report with troubleshooting tips
    DesfireErrorHandler::printErrorReport(status, "Select Application");
  }
}
```

## Documentation

For detailed documentation on the DESFire protocol and API reference, see the [docs](./docs) directory.

## Error Handling

The library provides a `DesfireErrorHandler` class that helps you interpret and handle error codes returned by DESFire operations:

- **Human-readable error messages**: Convert status codes to descriptive text
- **Error categorization**: Group errors by type (authentication, file, application, etc.)
- **Troubleshooting assistance**: Get suggestions for resolving common issues
- **Detailed error reports**: Print comprehensive error information for debugging

See the [ErrorHandlingExample](examples/ErrorHandlingExample/ErrorHandlingExample.ino) for a complete demonstration.

## Development

### Code Style

This project uses a consistent coding style documented in [CODING_STYLE.md](docs/CODING_STYLE.md). Key points include:

- Using `enum class` for type safety
- Consistent naming conventions for enums, constants, and variables
- Proper documentation with Doxygen-style comments

### Setup Development Environment

1. Clone this repository
2. Install PlatformIO (recommended) or Arduino IDE
3. Run the setup script to configure Git hooks:
   ```
   ./setup-git-hooks.sh
   ```
4. **Install required development tools:**
   - **clang-format** (required for code formatting)
   - **cppcheck** (required for static analysis)
   - **PlatformIO** (required for building and testing)
   
   These tools are mandatory for development, and commits will be blocked if they're not installed or if tests fail. See [CODING_STYLE.md](docs/CODING_STYLE.md) for installation instructions.

### Build Process

```bash
# Build the library
pio run

# Run tests
pio test -e hardware_test

# Lint code
pio check
```

## License

This library is released under the MIT License. See [LICENSE](LICENSE) for details.

## Contributing

Contributions are welcome! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for details on how to contribute to this project.

## Card Information

### Version Detection

The library provides comprehensive version information retrieval from DESFire cards. The `getVersion()` method retrieves hardware and software details, storage capacity, card type, and manufacturing information.

```cpp
// Get card version information
DESFireCardVersion version;
if (desfire.getVersion(&version)) {
  // Access information using the version struct
  Serial.print("Card Type: ");
  Serial.println(version.getCardTypeName());
  
  Serial.print("Storage Size: ");
  Serial.print(version.getStorageSize());
  Serial.println(" bytes");
}
```

More extensive examples of retrieving card information can be found in the `examples/CardVersionInfo` directory. 

## Multi-Frame Data Transfer

The library supports multi-frame data transfer for handling large data payloads that exceed the maximum capacity of a single APDU frame. This is particularly useful for operations like reading or writing large files or retrieving extensive datasets from the card.

### How Multi-Frame Transfers Work

1. When data exceeds the maximum single-frame size, it's automatically split into multiple frames.
2. For reading operations, the library handles receiving multiple response frames and concatenates them into a single response buffer.
3. For writing operations, the library splits large data into smaller chunks and sends them sequentially.

### Example Usage

```cpp
// Reading a large file (multi-frame response)
uint8_t largeBuffer[2048];
uint32_t bytesRead = 0;
DesfireStatus status = desfire.readData(fileNo, 0, 2048, largeBuffer, bytesRead);

// Writing a large file (multi-frame request)
uint8_t largeData[1024]; // Data to write
// Fill largeData with your information...
status = desfire.writeData(fileNo, 0, 1024, largeData);
```

A complete example demonstrating multi-frame data transfer can be found in the `examples/MultiFrameDataTransfer` directory. 