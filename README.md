# Arduino-DesfireNFC

A comprehensive library for MIFARE DESFire NFC cards communication with ESP32 microcontrollers. This library provides a complete implementation of the DESFire protocol, including authentication, secure messaging, and file operations.

## Features

- Support for DESFire EV1 and EV2 cards
- Multiple authentication methods (Legacy, ISO, AES)
- Secure messaging with encryption and MAC
- Complete application and file management
- Transaction mechanism support
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

## Documentation

For detailed documentation on the DESFire protocol and API reference, see the [docs](./docs) directory.

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
   
   These tools are mandatory for development, and commits will be blocked if they're not installed. See [CODING_STYLE.md](docs/CODING_STYLE.md) for installation instructions.

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