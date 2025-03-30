# APDU Command Structure and Status Codes

This documentation provides detailed information about the Application Protocol Data Unit (APDU) command structure and status codes used in the Arduino-DesfireNFC library. It covers both the ISO 7816-4 APDU format and DESFire-specific commands and status codes.

## 1. ISO 7816-4 APDU Command Structure

### 1.1 Command APDU Format

ISO 7816-4 defines the format for command APDUs, which are used to communicate with smart cards:

| Field | Length | Description |
|-------|--------|-------------|
| CLA   | 1 byte | Class byte - identifies the command category |
| INS   | 1 byte | Instruction byte - specifies the command |
| P1    | 1 byte | Parameter 1 - command-specific parameter |
| P2    | 1 byte | Parameter 2 - command-specific parameter |
| Lc    | 0-3 bytes | Length of command data (our implementation uses 1 byte) |
| Data  | Variable | Command data (0 to 255 bytes in short APDU format) |
| Le    | 0-3 bytes | Expected length of response (our implementation uses 1 byte) |

In our library, we implement the short APDU format with limitations:
- Maximum data field size: 255 bytes
- Maximum APDU size: 261 bytes (CLA+INS+P1+P2+Lc+Data+Le)

### 1.2 Response APDU Format

| Field | Length | Description |
|-------|--------|-------------|
| Data  | Variable | Response data (if any) |
| SW1   | 1 byte | Status Word 1 |
| SW2   | 1 byte | Status Word 2 |

The Status Word (SW1-SW2) indicates the result of the command execution.

### 1.3 APDU Command Implementation

In our library, the ISO7816APDU class handles APDU construction and processing:

```cpp
// Building a command APDU
ISO7816APDU::Command cmd;
cmd.cla = ISO7816Class::ISO_CLA_DESFIRE;  // 0x90
cmd.ins = ISO7816Instruction::ISO_INS_DESFIRE_NATIVE;  // 0x00
cmd.p1 = 0x00;
cmd.p2 = 0x00;
cmd.data[0] = static_cast<uint8_t>(DesfireCommand::DF_CMD_GET_VERSION);
cmd.dataLength = 1;
cmd.le = 0x00;  // Request all available data

// Parse response
ISO7816APDU::Response response;
parseResponse(responseData, responseLength, response);
```

## 2. DESFire Command Codes

DESFire commands are wrapped in ISO 7816-4 APDUs. The DESFire command code is placed in the data field of the APDU, followed by any command-specific parameters.

### 2.1 Card Level Commands

| Command | Code | Description | Parameters |
|---------|------|-------------|------------|
| GET_VERSION | 0x60 | Get card manufacturing related data | None |
| GET_CARD_UID | 0x51 | Get card UID (requires authentication) | None |
| GET_APPLICATION_DIRECTORY | 0x6A | Get application identifiers present on card | None |
| GET_ADDITIONAL_FRAME | 0xAF | Get additional data if response doesn't fit into buffer | None |
| SELECT_APPLICATION | 0x5A | Select one application for further access | 3-byte AID (LSB first) |
| FORMAT_PICC | 0xFC | Erase all applications and files on card | None |
| GET_FREE_MEMORY | 0x6E | Get available memory on card | None |

### 2.2 Authentication Commands

| Command | Code | Description | Parameters |
|---------|------|-------------|------------|
| AUTHENTICATE | 0x0A | Authenticate with DES/3DES keys | 1-byte Key Number |
| AUTHENTICATE_ISO | 0x1A | Authenticate with 3DES keys (ISO mode) | 1-byte Key Number |
| AUTHENTICATE_AES | 0xAA | Authenticate with AES keys | 1-byte Key Number |
| CHANGE_KEY_SETTINGS | 0x54 | Change key settings for application | 1-byte Key Settings |
| SET_CONFIGURATION | 0x5C | Change card configuration settings | Configuration-specific data |
| CHANGE_KEY | 0xC4 | Change encryption key | Key-specific data |
| GET_KEY_VERSION | 0x64 | Get encryption key version | 1-byte Key Number |

### 2.3 Application Management Commands

| Command | Code | Description | Parameters |
|---------|------|-------------|------------|
| CREATE_APPLICATION | 0xCA | Create a new application on card | 3-byte AID, 1-byte Key Settings, 1-byte Num Keys |
| DELETE_APPLICATION | 0xDA | Delete application and all related files | 3-byte AID |
| GET_APPLICATION_IDS | 0x6A | Get identifier of applications on card | None |

### 2.4 File Management Commands

| Command | Code | Description | Parameters |
|---------|------|-------------|------------|
| CREATE_STANDARD_FILE | 0xCD | Create a standard data file | File parameters |
| CREATE_BACKUP_FILE | 0xCB | Create a backup data file | File parameters |
| CREATE_VALUE_FILE | 0xCC | Create a value file | File parameters |
| CREATE_LINEAR_RECORD_FILE | 0xC1 | Create a linear record file | File parameters |
| CREATE_CYCLIC_RECORD_FILE | 0xC0 | Create a cyclic record file | File parameters |
| DELETE_FILE | 0xDF | Delete a file | 1-byte File ID |
| GET_FILE_IDS | 0x6F | Get file identifiers | None |
| GET_FILE_SETTINGS | 0xF5 | Get file settings | 1-byte File ID |
| CHANGE_FILE_SETTINGS | 0x5F | Change file settings | 1-byte File ID + settings |

### 2.5 Data Manipulation Commands

| Command | Code | Description | Parameters |
|---------|------|-------------|------------|
| READ_DATA | 0xBD | Read data from standard/backup files | 1-byte File ID, 3-byte Offset, 3-byte Length |
| WRITE_DATA | 0x3D | Write data to standard/backup files | 1-byte File ID, 3-byte Offset, 3-byte Length, Data |
| GET_VALUE | 0x6C | Get value from value file | 1-byte File ID |
| CREDIT | 0x0C | Increase value in value file | 1-byte File ID, 4-byte Value |
| DEBIT | 0xDC | Decrease value in value file | 1-byte File ID, 4-byte Value |
| LIMITED_CREDIT | 0x1C | Limited increase of value in value file | 1-byte File ID, 4-byte Value |
| WRITE_RECORD | 0x3B | Write record to record file | 1-byte File ID, 3-byte Offset, 3-byte Length, Data |
| READ_RECORDS | 0xBB | Read record(s) from record file | 1-byte File ID, 3-byte Offset, 3-byte Length |
| CLEAR_RECORD_FILE | 0xEB | Clear a record file | 1-byte File ID |

### 2.6 Transaction Commands

| Command | Code | Description | Parameters |
|---------|------|-------------|------------|
| COMMIT_TRANSACTION | 0xC7 | Commit previous write access | None |
| ABORT_TRANSACTION | 0xA7 | Abort previous write access | None |

### 2.7 EV2-Specific Commands

| Command | Code | Description | Parameters |
|---------|------|-------------|------------|
| AUTHENTICATE_EV2_FIRST | 0x71 | First part of EV2 authentication | 1-byte Key Number |
| AUTHENTICATE_EV2_NONFIRST | 0x77 | Non-first part of EV2 authentication | 1-byte Key Number |
| GET_COMMAND_COUNTER | 0x7A | Get command counter | None |
| SET_COMMAND_COUNTER | 0x7B | Set command counter | 4-byte Counter Value |

## 3. Status Codes

### 3.1 ISO 7816-4 Status Words

| Status Word | Name | Description |
|------------|------|-------------|
| 0x9000 | ISO_SW_SUCCESS | Command successful |
| 0x9100 | ISO_SW_SUCCESS_DESFIRE | DESFire success |
| 0x6100 | ISO_SW_BYTES_REMAINING | Warning: Bytes still available |
| 0x6200 | ISO_SW_WARNING_NVM | Warning: NVM unchanged |
| 0x6281 | ISO_SW_WARNING_MEMORY | Warning: Memory unchanged |
| 0x6282 | ISO_SW_WARNING_EOF | Warning: End of file reached |
| 0x6283 | ISO_SW_WARNING_INVALIDATED | Warning: File invalidated |
| 0x6284 | ISO_SW_WARNING_FCI | Warning: FCI not formatted |
| 0x6700 | ISO_SW_WRONG_LENGTH | Wrong length |
| 0x6C00 | ISO_SW_WRONG_LE | Incorrect Le byte |
| 0x6982 | ISO_SW_SECURITY_NOT_SATISFIED | Security status not satisfied |
| 0x6983 | ISO_SW_AUTH_METHOD_BLOCKED | Authentication method blocked |
| 0x6984 | ISO_SW_DATA_INVALID | Referenced data invalid |
| 0x6985 | ISO_SW_CONDITIONS_NOT_SATISFIED | Conditions of use not satisfied |
| 0x6986 | ISO_SW_COMMAND_NOT_ALLOWED | Command not allowed |
| 0x6A80 | ISO_SW_WRONG_DATA | Incorrect data |
| 0x6A82 | ISO_SW_FILE_NOT_FOUND | File not found |
| 0x6A83 | ISO_SW_RECORD_NOT_FOUND | Record not found |
| 0x6A86 | ISO_SW_WRONG_P1P2 | Incorrect P1-P2 parameters |
| 0x6A87 | ISO_SW_WRONG_DATA_LENGTH | Wrong data length |
| 0x6D00 | ISO_SW_INS_NOT_SUPPORTED | Instruction code not supported |
| 0x6E00 | ISO_SW_CLASS_NOT_SUPPORTED | Class not supported |
| 0x6F00 | ISO_SW_UNKNOWN | Unknown error |

### 3.2 DESFire Status Codes (as SW1-SW2)

| Status Word | Name | Description |
|------------|------|-------------|
| 0x9100 | DFST_SUCCESS | Successful operation |
| 0x91AF | DFST_MORE_FRAMES | Additional data frames are expected |
| 0x910C | DFST_NO_CHANGES | No changes made to backup files |
| 0x910E | DFST_OUT_OF_EEPROM | Insufficient memory for operation |
| 0x911C | DFST_ILLEGAL_COMMAND | Command code not supported |
| 0x911E | DFST_INTEGRITY_ERROR | CRC or MAC does not match data |
| 0x911F | DFST_PARAMETER_ERROR | Invalid command parameter |
| 0x9140 | DFST_NO_SUCH_KEY | Key specified in command does not exist |
| 0x917E | DFST_LENGTH_ERROR | Length of command does not match with its specification |
| 0x919D | DFST_PERMISSION_DENIED | Current configuration/status does not allow the operation |
| 0x91A0 | DFST_APPLICATION_NOT_FOUND | Requested AID not present on PICC |
| 0x91A1 | DFST_APPLICATION_INTEGRITY_ERROR | Unrecoverable error within application |
| 0x91AE | DFST_AUTHENTICATION_ERROR | Current authentication status does not allow the operation |
| 0x91BE | DFST_BOUNDARY_ERROR | Attempted to read/write beyond the file's/record's limit |
| 0x91C1 | DFST_PICC_INTEGRITY_ERROR | Unrecoverable error within the PICC |
| 0x91CA | DFST_COMMAND_ABORTED | Previous command was not fully completed |
| 0x91CD | DFST_CARD_INTEGRITY_ERROR | Unrecoverable error within PICC |
| 0x91DE | DFST_DUPLICATE_ERROR | Attempted creation of file/application that already exists |
| 0x91EE | DFST_EEPROM_ERROR | Error related to EEPROM |
| 0x91F0 | DFST_FILE_NOT_FOUND | Specified file does not exist |
| 0x91F1 | DFST_FILE_INTEGRITY_ERROR | Unrecoverable error within file |

## 4. Multi-Frame Communication

When data doesn't fit into a single APDU frame, multi-frame communication is used:

1. Send the initial command
2. Card responds with status `0x91AF` (more frames expected)
3. Send the `GET_ADDITIONAL_FRAME` command (0xAF)
4. Repeat until card responds with a final status (0x9100 for success)

Example:
```cpp
// Send initial command
DesfireStatus status = transmit(DesfireCommand::DF_CMD_READ_DATA, params, paramsLen, response, respLen);

// If more frames are needed
while (status == DesfireStatus::DFST_MORE_FRAMES) {
    // Send GET_ADDITIONAL_FRAME command
    status = transmit(DesfireCommand::DF_CMD_GET_ADDITIONAL_FRAME, nullptr, 0, additionalResp, additionalRespLen);
    
    // Process additional response data
    // ...
}
```

## 5. Secure Messaging

After authentication, commands can be sent in three communication modes:

| Mode | Value | Description |
|------|-------|-------------|
| PLAIN | 0x00 | No encryption or MACing |
| MAC | 0x01 | Data sent in clear with MAC appended |
| ENCRYPT | 0x03 | Data is encrypted using the session key |

The choice of communication mode affects how commands are constructed and how responses are processed.

## 6. Error Handling

When processing responses, the library converts ISO and DESFire status codes to the `DesfireStatus` enum:

```cpp
DesfireStatus status = ISO7816APDU::convertStatus(response.status);
if (status != DesfireStatus::DFST_SUCCESS) {
    // Handle error
}
```

Common error handling strategies:
- Authentication errors: Reset the authentication state
- Communication errors: Retry the command
- Parameter errors: Check input parameters
- Permission denied: Verify authentication and access rights

## 7. Example Command Flow

### 7.1 Reading a Standard File

```cpp
// Select application
uint8_t aid[] = {0xEE, 0xFF, 0xC0}; // AID in LSB format
transmit(DesfireCommand::DF_CMD_SELECT_APPLICATION, aid, 3, response, respLen);

// Authenticate with key 0 (if required)
uint8_t keyNum = 0;
transmit(DesfireCommand::DF_CMD_AUTHENTICATE, &keyNum, 1, response, respLen);
// ... complete authentication process ...

// Read file data
uint8_t params[7];
params[0] = fileId;           // File ID
params[1] = offsetLSB;        // Offset (LSB first)
params[2] = offsetMSB;
params[3] = offsetMSB2;
params[4] = lengthLSB;        // Length (LSB first)
params[5] = lengthMSB;
params[6] = lengthMSB2;
transmit(DesfireCommand::DF_CMD_READ_DATA, params, 7, response, respLen);
``` 