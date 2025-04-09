# Command Reference

This document provides a comprehensive reference for all commands supported by MIFARE DESFire EV1 and MIFARE Ultralight C cards. Each command includes its APDU format, parameters, response format, and potential error codes.

## MIFARE DESFire EV1 Commands

DESFire EV1 commands are organized into categories based on their scope and functionality.

### Security Commands

#### AUTHENTICATE_DES_2K3DES (0x0A)

Initiates mutual authentication using DES or 2K3DES.

```
Command: 90 0A 00 00 01 [keyNo]
Response: [8 bytes encrypted RndB] AF
```

- **keyNo**: Key number (0-13)
- **Error codes**: 0x1C, 0x40, 0xAE, 0xBE

#### AUTHENTICATE_3K3DES (0x1A)

Initiates mutual authentication using 3K3DES.

```
Command: 90 1A 00 00 01 [keyNo]
Response: [16 bytes encrypted RndB] AF
```

- **keyNo**: Key number (0-13)
- **Error codes**: 0x1C, 0x40, 0xAE, 0xBE

#### AUTHENTICATE_AES (0xAA)

Initiates mutual authentication using AES.

```
Command: 90 AA 00 00 01 [keyNo]
Response: [16 bytes encrypted RndB] AF
```

- **keyNo**: Key number (0-13)
- **Error codes**: 0x1C, 0x40, 0xAE, 0xBE

#### CHANGE_KEY_SETTINGS (0x54)

Changes the key settings for the current application.

```
Command: 90 54 00 00 01 [keySettings]
Response: 00 (success)
```

- **keySettings**: Bit-encoded key settings
  - Bit 0: Allow changing master key
  - Bit 1: Free directory list access
  - Bit 2: Allow changing key settings
  - Bit 3: Allow changing keys
  - Bit 4-7: Access rights for change
- **Error codes**: 0x1C, 0x1E, 0x9D, 0xAE

#### CHANGE_KEY (0xC4)

Changes a key in the current application.

```
Command: 90 C4 00 00 [length] [keyNo] [keyData]
Response: 00 (success)
```

- **keyNo**: Key number (0-13)
- **keyData**: Encrypted key data (format depends on key type)
- **Error codes**: 0x1C, 0x1E, 0x40, 0x7E, 0x9D, 0xAE, 0xBE

#### GET_KEY_VERSION (0x64)

Retrieves the version of a key.

```
Command: 90 64 00 00 01 [keyNo]
Response: [keyVersion] 00
```

- **keyNo**: Key number (0-13)
- **Error codes**: 0x1C, 0x40, 0x9D

### PICC Level Commands

#### CREATE_APPLICATION (0xCA)

Creates a new application on the card.

```
Command: 90 CA 00 00 05 [AID (3 bytes)] [keySettings] [numKeys]
Response: 00 (success)
```

- **AID**: 3-byte application identifier
- **keySettings**: Application master key settings
- **numKeys**: Number of keys (1-14)
- **Error codes**: 0x1C, 0x9D, 0xAE, 0xBE, 0xCE, 0xDE

#### DELETE_APPLICATION (0xDA)

Deletes an application from the card.

```
Command: 90 DA 00 00 03 [AID (3 bytes)]
Response: 00 (success)
```

- **AID**: 3-byte application identifier
- **Error codes**: 0x1C, 0x9D, 0xA0, 0xAE, 0xBE

#### GET_APPLICATIONS_IDS (0x6A)

Lists all application identifiers on the card.

```
Command: 90 6A 00 00 00
Response: [AID list] 00
```

- **Error codes**: 0x1C, 0x9D, 0xAE

#### SELECT_APPLICATION (0x5A)

Selects an application for subsequent commands.

```
Command: 90 5A 00 00 03 [AID (3 bytes)]
Response: 00 (success)
```

- **AID**: 3-byte application identifier
- **Error codes**: 0x1C, 0x9D, 0xA0, 0xAE, 0xBE

#### FORMAT_PICC (0xFC)

Formats the card, deleting all applications.

```
Command: 90 FC 00 00 00
Response: 00 (success)
```

- **Error codes**: 0x1C, 0x9D, 0xAE

#### GET_VERSION (0x60)

Retrieves version information about the card.

```
Command: 90 60 00 00 00
Response: [version data] 00
```

- **Version data**:
  - Byte 0: Hardware vendor ID
  - Byte 1: Hardware type
  - Byte 2: Hardware subtype
  - Byte 3: Hardware version major
  - Byte 4: Hardware version minor
  - Byte 5: Storage size
  - Byte 6: Protocol type
- **Error codes**: 0x1C, 0x9D, 0xAE

#### GET_CARD_UID (0x51)

Retrieves the unique identifier of the card.

```
Command: 90 51 00 00 00
Response: [UID (7 bytes)] 00
```

- **Error codes**: 0x1C, 0x9D, 0xAE

### Application Level Commands

#### GET_FILE_IDS (0x6F)

Lists all file identifiers in the current application.

```
Command: 90 6F 00 00 00
Response: [file ID list] 00
```

- **Error codes**: 0x1C, 0x9D, 0xAE

#### GET_FILE_SETTINGS (0xF5)

Retrieves the settings of a file.

```
Command: 90 F5 00 00 01 [fileNo]
Response: [file settings] 00
```

- **fileNo**: File number (0-31)
- **File settings**:
  - Byte 0: File type
  - Byte 1: Communication settings
  - Byte 2-3: Access rights
  - Additional bytes depend on file type
- **Error codes**: 0x1C, 0x9D, 0xAE, 0xF0

#### CHANGE_FILE_SETTINGS (0x5F)

Changes the settings of a file.

```
Command: 90 5F 00 00 [length] [fileNo] [commSettings] [accessRights]
Response: 00 (success)
```

- **fileNo**: File number (0-31)
- **commSettings**: Communication settings (0x00=Plain, 0x01=MACed, 0x03=Enciphered)
- **accessRights**: Access rights for reading/writing/etc. (2 bytes)
- **Error codes**: 0x1C, 0x1E, 0x9D, 0xAE, 0xF0

#### CREATE_STD_DATA_FILE (0xCD)

Creates a standard data file.

```
Command: 90 CD 00 00 07 [fileNo] [commSettings] [accessRights (2 bytes)] [fileSize (3 bytes)]
Response: 00 (success)
```

- **fileNo**: File number (0-31)
- **commSettings**: Communication settings
- **accessRights**: Access rights (2 bytes)
- **fileSize**: File size (3 bytes, LSB first)
- **Error codes**: 0x1C, 0x9D, 0xAE, 0xBE, 0xCE, 0xDE

#### CREATE_BACKUP_DATA_FILE (0xCB)

Creates a backup data file.

```
Command: 90 CB 00 00 07 [fileNo] [commSettings] [accessRights (2 bytes)] [fileSize (3 bytes)]
Response: 00 (success)
```

- **fileNo**: File number (0-31)
- **commSettings**: Communication settings
- **accessRights**: Access rights (2 bytes)
- **fileSize**: File size (3 bytes, LSB first)
- **Error codes**: 0x1C, 0x9D, 0xAE, 0xBE, 0xCE, 0xDE

#### CREATE_VALUE_FILE (0xCC)

Creates a value file.

```
Command: 90 CC 00 00 0D [fileNo] [commSettings] [accessRights (2 bytes)] [lowerLimit (4 bytes)] [upperLimit (4 bytes)] [value (4 bytes)] [limitedCreditEnabled]
Response: 00 (success)
```

- **fileNo**: File number (0-31)
- **commSettings**: Communication settings
- **accessRights**: Access rights (2 bytes)
- **lowerLimit**: Lower limit for the value (4 bytes)
- **upperLimit**: Upper limit for the value (4 bytes)
- **value**: Initial value (4 bytes)
- **limitedCreditEnabled**: Whether limited credit is enabled (0x00 or 0x01)
- **Error codes**: 0x1C, 0x9D, 0xAE, 0xBE, 0xCE, 0xDE

#### CREATE_LINEAR_RECORD_FILE (0xC1)

Creates a linear record file.

```
Command: 90 C1 00 00 0A [fileNo] [commSettings] [accessRights (2 bytes)] [recordSize (3 bytes)] [maxRecords (3 bytes)]
Response: 00 (success)
```

- **fileNo**: File number (0-31)
- **commSettings**: Communication settings
- **accessRights**: Access rights (2 bytes)
- **recordSize**: Size of each record (3 bytes)
- **maxRecords**: Maximum number of records (3 bytes)
- **Error codes**: 0x1C, 0x9D, 0xAE, 0xBE, 0xCE, 0xDE

#### CREATE_CYCLIC_RECORD_FILE (0xC0)

Creates a cyclic record file.

```
Command: 90 C0 00 00 0A [fileNo] [commSettings] [accessRights (2 bytes)] [recordSize (3 bytes)] [maxRecords (3 bytes)]
Response: 00 (success)
```

- **fileNo**: File number (0-31)
- **commSettings**: Communication settings
- **accessRights**: Access rights (2 bytes)
- **recordSize**: Size of each record (3 bytes)
- **maxRecords**: Maximum number of records (3 bytes)
- **Error codes**: 0x1C, 0x9D, 0xAE, 0xBE, 0xCE, 0xDE

#### DELETE_FILE (0xDF)

Deletes a file.

```
Command: 90 DF 00 00 01 [fileNo]
Response: 00 (success)
```

- **fileNo**: File number (0-31)
- **Error codes**: 0x1C, 0x9D, 0xAE, 0xF0

### File Data Manipulation Commands

#### READ_DATA (0xBD)

Reads data from a standard or backup data file.

```
Command: 90 BD 00 00 07 [fileNo] [offset (3 bytes)] [length (3 bytes)]
Response: [data] 00
```

- **fileNo**: File number (0-31)
- **offset**: Offset from the beginning of the file (3 bytes)
- **length**: Number of bytes to read (3 bytes)
- **Error codes**: 0x1C, 0x9D, 0xAE, 0xBE, 0xF0

#### WRITE_DATA (0x3D)

Writes data to a standard or backup data file.

```
Command: 90 3D 00 00 [length+7] [fileNo] [offset (3 bytes)] [dataLength (3 bytes)] [data]
Response: 00 (success)
```

- **fileNo**: File number (0-31)
- **offset**: Offset from the beginning of the file (3 bytes)
- **dataLength**: Number of bytes to write (3 bytes)
- **data**: Data to write
- **Error codes**: 0x1C, 0x9D, 0xAE, 0xBE, 0xF0

#### GET_VALUE (0x6C)

Reads the value from a value file.

```
Command: 90 6C 00 00 01 [fileNo]
Response: [value (4 bytes)] 00
```

- **fileNo**: File number (0-31)
- **Error codes**: 0x1C, 0x9D, 0xAE, 0xF0

#### CREDIT (0x0C)

Increases the value in a value file.

```
Command: 90 0C 00 00 05 [fileNo] [value (4 bytes)]
Response: 00 (success)
```

- **fileNo**: File number (0-31)
- **value**: Value to add (4 bytes)
- **Error codes**: 0x1C, 0x9D, 0xAE, 0xBE, 0xF0

#### DEBIT (0xDC)

Decreases the value in a value file.

```
Command: 90 DC 00 00 05 [fileNo] [value (4 bytes)]
Response: 00 (success)
```

- **fileNo**: File number (0-31)
- **value**: Value to subtract (4 bytes)
- **Error codes**: 0x1C, 0x9D, 0xAE, 0xBE, 0xF0

#### LIMITED_CREDIT (0x1C)

Performs a limited increase of a value.

```
Command: 90 1C 00 00 05 [fileNo] [value (4 bytes)]
Response: 00 (success)
```

- **fileNo**: File number (0-31)
- **value**: Value to add (4 bytes)
- **Error codes**: 0x1C, 0x9D, 0xAE, 0xBE, 0xF0

#### WRITE_RECORD (0x3B)

Writes a record to a record file.

```
Command: 90 3B 00 00 [length+7] [fileNo] [offset (3 bytes)] [dataLength (3 bytes)] [data]
Response: 00 (success)
```

- **fileNo**: File number (0-31)
- **offset**: Offset within the record (3 bytes)
- **dataLength**: Number of bytes to write (3 bytes)
- **data**: Data to write
- **Error codes**: 0x1C, 0x9D, 0xAE, 0xBE, 0xF0

#### READ_RECORDS (0xBB)

Reads records from a record file.

```
Command: 90 BB 00 00 07 [fileNo] [recordNo (3 bytes)] [recordCount (3 bytes)]
Response: [data] 00
```

- **fileNo**: File number (0-31)
- **recordNo**: Record number (3 bytes)
- **recordCount**: Number of records to read (3 bytes)
- **Error codes**: 0x1C, 0x9D, 0xAE, 0xBE, 0xF0

#### CLEAR_RECORD_FILE (0xEB)

Clears all records in a record file.

```
Command: 90 EB 00 00 01 [fileNo]
Response: 00 (success)
```

- **fileNo**: File number (0-31)
- **Error codes**: 0x1C, 0x9D, 0xAE, 0xF0

#### COMMIT_TRANSACTION (0xC7)

Commits a transaction.

```
Command: 90 C7 00 00 00
Response: 00 (success)
```

- **Error codes**: 0x1C, 0x9D, 0xAE

#### ABORT_TRANSACTION (0xA7)

Aborts a transaction.

```
Command: 90 A7 00 00 00
Response: 00 (success)
```

- **Error codes**: 0x1C, 0x9D, 0xAE

## MIFARE Ultralight C Commands

Ultralight C uses a different command set, primarily based on direct memory access.

### READ Command (0xB0)

Reads 16 bytes (4 pages) starting at the specified page.

```
Command: FF B0 00 [page] 10
Response: [16 bytes of data] 90 00
```

- **page**: Page number (0-47)
- **Error codes**: 0x6A 0x81 (Function not supported), 0x6A 0x82 (File not found)

### WRITE Command (0xD6)

Writes 4 bytes to the specified page.

```
Command: FF D6 00 [page] 04 [4 bytes of data]
Response: 90 00 (success)
```

- **page**: Page number (0-47)
- **Error codes**: 0x6A 0x82 (File not found), 0x69 0x82 (Security status not satisfied)

### AUTHENTICATE Command (Part 1) (0xEF)

Initiates authentication with the card.

```
Command: FF EF 00 00 02 1A 00
Response: AF [8 bytes encrypted RndB]
```

- **Error codes**: 0x69 0x82 (Security status not satisfied)

### AUTHENTICATE Command (Part 2) (0xAF)

Completes the authentication process.

```
Command: FF AF 00 00 11 [16 bytes encrypted (RndA || RndB')]
Response: 00 [8 bytes encrypted RndA']
```

- **Error codes**: 0x69 0x82 (Security status not satisfied)

### Setting Auth0 (0xD6 to Page 0x2A)

Sets the page from which authentication is required.

```
Command: FF D6 00 2A 04 [page] 00 00 00
Response: 90 00 (success)
```

- **page**: Page number (0-47, or 0xFF to disable authentication)

### Setting Auth1 (0xD6 to Page 0x2B)

Configures read/write access conditions.

```
Command: FF D6 00 2B 04 [value] 00 00 00
Response: 90 00 (success)
```

- **value**: 0x00 for read/write protection, 0x01 for write-only protection

### Setting Lock Bytes (0xD6 to Pages 0x02, 0x28, and 0x29)

Sets lock bits to make pages read-only.

```
Command: FF D6 00 [page] 04 [lock bytes] 00 00 00
Response: 90 00 (success)
```

## Response Status Codes

### DESFire EV1 Status Codes

| Code | Description           |
| ---- | --------------------- |
| 0x00 | Operation OK          |
| 0x0C | No changes            |
| 0x1C | Illegal command code  |
| 0x1E | Integrity error       |
| 0x40 | No such key           |
| 0x7E | Length error          |
| 0x9D | Permission denied     |
| 0x9E | Parameter error       |
| 0xA0 | Application not found |
| 0xAE | Authentication error  |
| 0xAF | Additional frame      |
| 0xBE | Boundary error        |
| 0xC1 | PICC integrity error  |
| 0xCA | Command aborted       |
| 0xCD | PICC disabled error   |
| 0xCE | Count error           |
| 0xDE | Duplicate error       |
| 0xEE | EEPROM error          |
| 0xF0 | File not found        |
| 0xF1 | File integrity error  |

### Ultralight C Status Codes (ISO 7816-4)

| Code  | Description                     |
| ----- | ------------------------------- |
| 90 00 | Success                         |
| 6A 81 | Function not supported          |
| 6A 82 | File or application not found   |
| 69 82 | Security status not satisfied   |
| 69 85 | Conditions of use not satisfied |
| 6B 00 | Wrong parameter(s) P1-P2        |
| 67 00 | Wrong length                    |
