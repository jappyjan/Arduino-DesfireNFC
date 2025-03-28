# Arduino-DesfireNFC Coding Style Guide

This document describes the coding style and naming conventions used in the Arduino-DesfireNFC library.

## Naming Conventions

### Enums and Constants

- Use `enum class` instead of plain `enum` for type safety and scope isolation:
  ```cpp
  enum class DesfireCommand : uint8_t {
      // members here
  };
  ```

- Enum names should use PascalCase:
  ```cpp
  enum class DesfireCommand {};
  enum class ISO7816Class {};
  ```

- Enum members should use UPPER_SNAKE_CASE with appropriate prefixes indicating their category:
  ```cpp
  enum class DesfireCommand : uint8_t {
      DF_CMD_GET_VERSION = 0x60,
      DF_CMD_GET_CARD_UID = 0x51
  };
  
  enum class ISO7816Class : uint8_t {
      ISO_CLA_STANDARD = 0x00,
      ISO_CLA_DESFIRE = 0x90
  };
  ```

- Avoid #define macros for constants. Use `constexpr` or `enum class` members instead:
  ```cpp
  // Instead of:
  #define ISO7816_MAX_APDU_SIZE 261
  
  // Use:
  constexpr uint16_t ISO7816_MAX_APDU_SIZE = 261;
  
  // Or as part of an enum class:
  enum class ISO7816Constants : uint16_t {
      MAX_APDU_SIZE = 261
  };
  ```

### Variable Names

- Use Hungarian notation for primitive types to indicate their type:
  ```cpp
  uint8_t ui8Value;
  uint16_t ui16Length;
  bool bSuccess;
  ```

- Member variables should be prefixed with an underscore:
  ```cpp
  class MyClass {
  private:
      uint8_t _ui8Value;
      bool _bAuthenticated;
  };
  ```

- Function parameters should follow the same convention as local variables (without underscore):
  ```cpp
  void myFunction(uint8_t ui8Parameter, bool bEnable);
  ```

### Function Names

- Use camelCase for function names:
  ```cpp
  bool initializeReader();
  uint8_t getCardType();
  ```

- Boolean functions should ask a question:
  ```cpp
  bool isAuthenticated();
  bool hasValidKey();
  ```

## Code Organization

### Headers

- Every header file should have include guards using the format:
  ```cpp
  #ifndef DESFIRE_NFC_H
  #define DESFIRE_NFC_H
  
  // Code here
  
  #endif // DESFIRE_NFC_H
  ```

- Group includes in the following order:
  1. C++ standard library
  2. Arduino core libraries
  3. Third-party libraries
  4. Project-specific headers

### Comments

- Use Doxygen-style comments for documenting classes, methods, and enumerations:
  ```cpp
  /**
   * @brief Brief description
   * 
   * Detailed description if needed
   * 
   * @param paramName Description of parameter
   * @return Description of return value
   */
  ```

- Use inline comments for explaining complex code sections:
  ```cpp
  // Calculate checksum
  uint16_t ui16Checksum = 0;
  for (uint8_t ui8I = 0; ui8I < ui8Length; ui8I++) {
      ui16Checksum ^= ui8Data[ui8I];
  }
  ```

## Error Handling

- Use the `DesfireStatus` enum class for indicating operation results:
  ```cpp
  DesfireStatus transmitCommand() {
      if (paramError) {
          return DesfireStatus::DFST_PARAMETER_ERROR;
      }
      // ...
  }
  ```

- Check input parameters at the beginning of functions:
  ```cpp
  bool readData(uint8_t* pui8Buffer, uint16_t ui16BufferSize) {
      if (pui8Buffer == nullptr || ui16BufferSize == 0) {
          return false;
      }
      // ...
  }
  ```

## Memory Management

- Avoid dynamic memory allocation when possible
- If dynamic allocation is needed, ensure proper deallocation
- Use fixed-size buffers with appropriate length checks

## Commits and Pull Requests

- Write clear, descriptive commit messages
- Keep commits focused on a single logical change
- Use feature branches for new features or major changes
- Run linters before committing

## Build Process

- Use PlatformIO for building and dependency management
- Run tests before submitting pull requests
- Ensure code passes all linter checks

## Required Development Tools

The following tools are **required** for development and are enforced through pre-commit hooks:

1. **clang-format** - Code formatting
   ```bash
   # macOS
   brew install clang-format
   
   # Ubuntu/Debian
   apt-get install clang-format
   
   # Windows (via Chocolatey)
   choco install llvm
   ```

2. **cppcheck** - Static code analysis
   ```bash
   # macOS
   brew install cppcheck
   
   # Ubuntu/Debian
   apt-get install cppcheck
   
   # Windows (via Chocolatey)
   choco install cppcheck
   ```

These tools are **not optional** and must be installed to contribute to this project. The pre-commit hook will validate their presence before allowing commits. 