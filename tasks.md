# Arduino-DesfireNFC Library Implementation Tasks

i have prepared a task list and protocol definitions for you.

please work on the tasks one by one.
when you finished a task, check it off and ask for review. do not continue until you got an approval.

## Setup & Project Structure
- [x] Create library.json manifest file with proper metadata
- [x] Set up basic library directory structure (src, include, examples)
- [x] Set up platformio.ini with proper ESP32 configuration
- [x] Create initial README.md with library description and usage examples
- [x] Set up unit test directory structure
- [x] Create contribution guidelines for community involvement
- [x] Set up CI/CD pipeline for automated testing

## Dependencies & Hardware Layer
- [x] Research and select PN532 driver library for ESP32
- [x] Create hardware abstraction layer for PN532 communication
- [x] Implement low-level NFC reader interface class
- [x] Test basic communication with PN532 reader
- [x] Implement ISO14443A protocol layer for tag communication
- [x] Create basic card detection and anti-collision functionality
- [x] Document hardware abstraction layer API
- [x] Create example application demonstrating basic tag detection
- [x] Write unit tests for hardware communication layer
- [x] Fix linter errors in DesfireStatus.h (ISO status codes)
- [x] Fix linter errors in DesfireNFC.cpp (API method errors)
- [x] Fix build issues in test code
- [x] Run hardware unit tests and validate functionality


## Lint and Build Issues
- [x] Fix ISO7816Constants.h enumerator value overflow issue
  - [x] Change ISO_MAX_APDU_SIZE type from uint8_t to uint16_t in ISO7816Constants enum
  - [x] Update all code using ISO_MAX_APDU_SIZE to handle uint16_t instead of uint8_t
  - [x] Verify compatibility with existing buffer sizes
- [x] Resolve DesfireNFC.cpp enum usage issues
  - [x] Fix macro/enum conflict in cryptographic mode definitions
  - [x] Properly use DesfireCommand enum instead of macro constants
  - [x] Fix member naming in DESFireCardVersion struct (productionBatchNumber vs productionWeek)
  - [x] Update all code using these enums to use the correct enum member syntax
- [x] Fix ISO7816APDU.cpp duplicate case value
  - [x] Remove duplicated case for ISO_SW_WRONG_LENGTH in convertStatus method
  - [x] Add proper comments to clarify status code handling
- [x] Resolve PN532Reader.cpp redefined constant warning
  - [x] Remove local PN532_MIFARE_ISO14443A definition
  - [x] Use the Adafruit_PN532 library constant directly
- [x] Fix platformio.ini issues
  - [x] Update src_filter configuration to use build_src_filter as recommended
  - [x] Add proper linting configuration
- [x] Update project organization and build process
  - [x] Implement consistent naming conventions for enums and constants
    - Follow C++ style enum class for all enumerations (enum class)
    - Use UPPER_SNAKE_CASE for enum members
    - Use consistent prefixes: DF_ (DESFire), ISO_ (ISO7816), etc.
    - Avoid #define macros for constants, use constexpr or enum class
    - Follow Hungarian notation for variable names (e.g., uint8_t ui8Value)
  - [x] Enforce uniform coding style
  - [x] Add pre-commit linting hooks

## Core DESFire Protocol Implementation
- [x] Implement APDU command builder and parser
- [x] Create DESFire command constants and status code definitions
- [ ] Implement error handling and status code interpretation
- [ ] Implement ISO 7816-4 APDU wrapping functionality
- [ ] Create DESFire card version detection and information retrieval
- [ ] Implement DESFire native command wrappers
- [ ] Create multi-frame data transfer support for large payloads
- [ ] Document APDU command structure and status codes
- [ ] Create example for card information retrieval
- [ ] Write unit tests for command building and parsing
- [ ] Run protocol implementation unit tests
- [ ] Optimize command execution flow for speed

## Authentication & Cryptography
- [ ] Implement DES/3DES encryption and decryption functions
- [ ] Implement AES encryption and decryption functions with ESP32 hardware acceleration
- [ ] Create legacy authentication (D40) implementation
- [ ] Create ISO authentication (EV1) implementation
- [ ] Implement AES authentication (EV1) implementation
- [ ] Implement session key derivation for different cryptographic modes
- [ ] Create IV management functionality for secure messaging
- [ ] Implement MAC generation and verification
- [ ] Create secure messaging wrapper for encrypted commands
- [ ] Document authentication processes with sequence diagrams
- [ ] Create authentication example applications
- [ ] Write unit tests for cryptographic functions
- [ ] Run cryptography unit tests
- [ ] Profile and optimize authentication performance

## Application & File Management
- [ ] Implement application selection functionality
- [ ] Create application creation and deletion methods
- [ ] Implement application listing and enumeration
- [ ] Create standard data file operations (create, read, write)
- [ ] Implement backup data file operations
- [ ] Create value file operations (credit, debit)
- [ ] Implement record file operations (linear and cyclic)
- [ ] Create file permission management functions
- [ ] Implement transaction mechanism (commit, abort)
- [ ] Document file types and operations
- [ ] Create example applications for file operations
- [ ] Write integration tests for file management
- [ ] Run file management tests
- [ ] Optimize memory usage for file operations

## ESP32-Specific Optimizations & Features
- [ ] Implement multi-core task distribution for cryptographic operations
- [ ] Create FreeRTOS task handling for asynchronous operations
- [ ] Optimize memory allocation patterns for ESP32 constraints
- [ ] Implement power management features for NFC operations
- [ ] Create connection pooling for multiple card handling
- [ ] Implement command caching for repeated operations
- [ ] Document ESP32-specific optimizations
- [ ] Create example demonstrating async NFC operations
- [ ] Write unit tests for ESP32-specific features
- [ ] Run ESP32 optimization tests
- [ ] Test and benchmark ESP32-specific features

## Deployment & Release
- [ ] Finalize comprehensive API documentation
- [ ] Create error handling and troubleshooting guide
- [ ] Create installation and dependency guide
- [ ] Finalize versioning and release notes
- [ ] Create comprehensive changelog
- [ ] Prepare library for PlatformIO Registry submission
- [ ] Test installation from PlatformIO Registry 