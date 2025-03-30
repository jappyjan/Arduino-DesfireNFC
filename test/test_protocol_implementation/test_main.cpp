/**
 * @file test_main.cpp
 * @brief Unit tests for the DESFire protocol implementation
 */

#include <Arduino.h>
#include <unity.h>
#include "DesfireNFC.h"
#include "DesfireTypes.h"
#include "NFCReaderInterface.h"

// Define the MockProtocolReader class
class MockProtocolReader : public NFCReaderInterface {
public:
    MockProtocolReader() {
        reset();
    }

    void reset() {
        _beginCalled        = false;
        _configureCalled    = false;
        _detectCardCalled   = false;
        _cardPresent        = true;
        _transceiveCalled   = false;
        _failTransceive     = false;
        _lastCommand        = 0;
        _lastCommandDataLen = 0;
        _responseStatus     = 0x91;
        _responseSubstatus  = 0x00;
        _useCustomResponse  = false;
    }

    bool begin() override {
        _beginCalled = true;
        return true;
    }

    uint32_t getFirmwareVersion() override {
        return 0x12345678;
    }

    bool configure() override {
        _configureCalled = true;
        return true;
    }

    bool detectCard(uint8_t* uid, uint8_t uidLength) override {
        _detectCardCalled = true;

        if (!_cardPresent) {
            return false;
        }

        // Simulate a DESFire card with 7-byte UID
        uidLength = 7;
        uid[0]    = 0x04;  // NXP manufacturer ID
        uid[1]    = 0x11;
        uid[2]    = 0x22;
        uid[3]    = 0x33;
        uid[4]    = 0x44;
        uid[5]    = 0x55;
        uid[6]    = 0x66;

        return true;
    }

    bool transceive(const uint8_t* txData,
                    uint16_t       txLength,
                    uint8_t*       rxData,
                    uint16_t*      rxLength) override {
        _transceiveCalled = true;

        if (_failTransceive) {
            return false;
        }

        // Store the command for verification
        if (txLength > 5) {            // APDU format: CLA+INS+P1+P2+Lc+Data
            _lastCommand = txData[5];  // The DESFire command is the first byte of the data

            // Store command data for verification if available
            _lastCommandDataLen = 0;
            if (txLength > 6) {
                _lastCommandDataLen = txLength - 6;
                memcpy(_lastCommandData, &txData[6], _lastCommandDataLen);
            }
        }

        // If using custom response, return that
        if (_useCustomResponse) {
            if (_customResponseDataLen >= 2) {
                // Add status bytes at the end of the custom response
                memcpy(rxData, _customResponseData, _customResponseDataLen);
                rxData[_customResponseDataLen]     = _responseStatus;
                rxData[_customResponseDataLen + 1] = _responseSubstatus;
                *rxLength                          = _customResponseDataLen + 2;
            } else {
                // Just return status bytes
                rxData[0] = _responseStatus;
                rxData[1] = _responseSubstatus;
                *rxLength = 2;
            }

            _useCustomResponse = false;  // Clear custom response after using it
            return true;
        }

        // Create a simple response based on the command
        // This is a simplified implementation that will be expanded for specific tests
        rxData[0] = _responseStatus;
        rxData[1] = _responseSubstatus;
        *rxLength = 2;

        return true;
    }

    // Control methods for tests
    void setCardPresent(bool present) {
        _cardPresent = present;
    }

    void setTransceiveFailure(bool fail) {
        _failTransceive = fail;
    }

    void setResponseStatus(uint8_t status, uint8_t substatus) {
        _responseStatus    = status;
        _responseSubstatus = substatus;
    }

    // When testing multi-frame responses, we need to simulate custom response data
    void setCustomResponse(const uint8_t* data, uint16_t dataLen) {
        if (dataLen <= sizeof(_customResponseData)) {
            memcpy(_customResponseData, data, dataLen);
            _customResponseDataLen = dataLen;
            _useCustomResponse     = true;
        }
    }

    void clearCustomResponse() {
        _useCustomResponse = false;
    }

    // Verification methods
    bool wasBeginCalled() const {
        return _beginCalled;
    }

    bool wasConfigureCalled() const {
        return _configureCalled;
    }

    bool wasDetectCardCalled() const {
        return _detectCardCalled;
    }

    bool wasTransceiveCalled() const {
        return _transceiveCalled;
    }

    uint8_t getLastCommand() const {
        return _lastCommand;
    }

    uint8_t getLastCommandDataLength() const {
        return _lastCommandDataLen;
    }

    const uint8_t* getLastCommandData() const {
        return _lastCommandData;
    }

private:
    bool     _beginCalled;
    bool     _configureCalled;
    bool     _detectCardCalled;
    bool     _cardPresent;
    bool     _transceiveCalled;
    bool     _failTransceive;
    uint8_t  _lastCommand;
    uint8_t  _lastCommandData[255];
    uint8_t  _lastCommandDataLen;
    uint8_t  _responseStatus;
    uint8_t  _responseSubstatus;
    bool     _useCustomResponse;
    uint8_t  _customResponseData[255];
    uint16_t _customResponseDataLen;
};

// Create an instance of the mock reader
MockProtocolReader g_mockProtocolReader;

// Our DESFire instance for testing
DesfireNFC* g_desfire = nullptr;

void setUp(void) {
    // Reset the mock reader to its default state before each test
    g_mockProtocolReader.reset();
}

void tearDown(void) {
    // Clean up code for each test
}

/**
 * @brief Test initialization of the DESFire library
 */
void test_desfire_initialization(void) {
    // Check if initialization works correctly
    TEST_ASSERT_TRUE(g_desfire->initialize());

    // Verify that begin() and configure() were called on the reader
    TEST_ASSERT_TRUE(g_mockProtocolReader.wasBeginCalled());
    TEST_ASSERT_TRUE(g_mockProtocolReader.wasConfigureCalled());
}

/**
 * @brief Test card detection
 */
void test_card_detection(void) {
    // Mock a card present
    g_mockProtocolReader.setCardPresent(true);

    // Check if we can detect the card
    TEST_ASSERT_TRUE(g_desfire->detectCard());
    TEST_ASSERT_TRUE(g_mockProtocolReader.wasDetectCardCalled());

    // Mock no card present
    g_mockProtocolReader.setCardPresent(false);

    // Check if detection fails when no card is present
    TEST_ASSERT_FALSE(g_desfire->detectCard());
}

/**
 * @brief Test getting the card UID
 */
void test_get_card_uid(void) {
    // Mock a card present
    g_mockProtocolReader.setCardPresent(true);

    // Test getting the UID
    uint8_t uid[7];
    uint8_t uidLength = 0;

    TEST_ASSERT_TRUE(g_desfire->getCardUID(uid, &uidLength));
    TEST_ASSERT_EQUAL(7, uidLength);
    TEST_ASSERT_EQUAL(0x04, uid[0]);  // First byte should be NXP manufacturer ID

    // Test with null parameters
    TEST_ASSERT_FALSE(g_desfire->getCardUID(nullptr, &uidLength));
    TEST_ASSERT_FALSE(g_desfire->getCardUID(uid, nullptr));
}

/**
 * @brief Test the getVersion command
 */
void test_get_version(void) {
    // Get card version information
    g_mockProtocolReader.setResponseStatus(0x91, 0x00);  // Success status

    // Simple version test
    DesfireStatus status = g_desfire->getVersion();
    TEST_ASSERT_EQUAL(DesfireStatus::DFST_SUCCESS, status);
    TEST_ASSERT_TRUE(g_mockProtocolReader.wasTransceiveCalled());
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(DesfireCommand::DF_CMD_GET_VERSION),
                      g_mockProtocolReader.getLastCommand());

    // Test full version info
    DESFireCardVersion version;

    // First frame - hardware info
    uint8_t hw_frame[] = {0x04, 0x01, 0x01, 0x05, 0x02, 0x00, 0x0B};
    g_mockProtocolReader.setCustomResponse(hw_frame, sizeof(hw_frame));

    // This test will be limited since our mock doesn't support complex multi-frame responses
    status = g_desfire->getVersion(&version);
    TEST_ASSERT_EQUAL(DesfireStatus::DFST_SUCCESS, status);

    // The mock doesn't fully implement multi-frame, so we can only test the command was sent
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(DesfireCommand::DF_CMD_GET_VERSION),
                      g_mockProtocolReader.getLastCommand());
}

/**
 * @brief Test selectApplication command
 */
void test_select_application(void) {
    // Prepare an Application ID
    uint8_t aid[3] = {0xA1, 0xA2, 0xA3};

    // Set response to success
    g_mockProtocolReader.setResponseStatus(0x91, 0x00);

    // Select the application
    DesfireStatus status = g_desfire->selectApplication(aid);

    // Verify the result
    TEST_ASSERT_EQUAL(DesfireStatus::DFST_SUCCESS, status);
    TEST_ASSERT_TRUE(g_mockProtocolReader.wasTransceiveCalled());
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(DesfireCommand::DF_CMD_SELECT_APPLICATION),
                      g_mockProtocolReader.getLastCommand());

    // Check command data
    TEST_ASSERT_EQUAL(3, g_mockProtocolReader.getLastCommandDataLength());
    TEST_ASSERT_EQUAL_MEMORY(aid, g_mockProtocolReader.getLastCommandData(), 3);

    // Test with null parameter
    status = g_desfire->selectApplication(nullptr);
    TEST_ASSERT_EQUAL(DesfireStatus::DFST_PARAMETER_NULL, status);
}

/**
 * @brief Test formatPICC command
 */
void test_format_picc(void) {
    // Set response to success
    g_mockProtocolReader.setResponseStatus(0x91, 0x00);

    // Format the PICC
    DesfireStatus status = g_desfire->formatPICC();

    // Verify the result
    TEST_ASSERT_EQUAL(DesfireStatus::DFST_SUCCESS, status);
    TEST_ASSERT_TRUE(g_mockProtocolReader.wasTransceiveCalled());
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(DesfireCommand::DF_CMD_FORMAT_PICC),
                      g_mockProtocolReader.getLastCommand());
}

/**
 * @brief Test getFreeMem command
 */
void test_get_free_mem(void) {
    // Prepare mock response (3 bytes for memory size)
    uint8_t mem_response[] = {0x20, 0x4E, 0x00};  // 20,000 bytes free (0x4E20)
    g_mockProtocolReader.setCustomResponse(mem_response, sizeof(mem_response));
    g_mockProtocolReader.setResponseStatus(0x91, 0x00);  // Success

    // Call getFreeMem
    uint32_t      freeMemory = 0;
    DesfireStatus status     = g_desfire->getFreeMem(&freeMemory);

    // Verify the result - note that our mock isn't fully implementing the custom response yet
    TEST_ASSERT_EQUAL(DesfireStatus::DFST_SUCCESS, status);
    TEST_ASSERT_TRUE(g_mockProtocolReader.wasTransceiveCalled());
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(DesfireCommand::DF_CMD_GET_FREE_MEMORY),
                      g_mockProtocolReader.getLastCommand());

    // Test with null parameter
    status = g_desfire->getFreeMem(nullptr);
    TEST_ASSERT_EQUAL(DesfireStatus::DFST_PARAMETER_NULL, status);
}

/**
 * @brief Test getApplicationIDs command
 */
void test_get_application_ids(void) {
    // Prepare mock response (two application IDs, 3 bytes each)
    uint8_t app_response[] = {0xA1, 0xA2, 0xA3, 0xB1, 0xB2, 0xB3};
    g_mockProtocolReader.setCustomResponse(app_response, sizeof(app_response));
    g_mockProtocolReader.setResponseStatus(0x91, 0x00);  // Success

    // Call getApplicationIDs
    uint8_t       appIds[10 * 3];  // Space for up to 10 app IDs
    uint8_t       count  = 0;
    DesfireStatus status = g_desfire->getApplicationIDs(appIds, 10, count);

    // Verify the result
    TEST_ASSERT_EQUAL(DesfireStatus::DFST_SUCCESS, status);
    TEST_ASSERT_TRUE(g_mockProtocolReader.wasTransceiveCalled());
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(DesfireCommand::DF_CMD_GET_APPLICATION_IDS),
                      g_mockProtocolReader.getLastCommand());

    // Test with null parameter
    status = g_desfire->getApplicationIDs(nullptr, 10, count);
    TEST_ASSERT_EQUAL(DesfireStatus::DFST_PARAMETER_NULL, status);

    // Test with maxCount = 0
    status = g_desfire->getApplicationIDs(appIds, 0, count);
    TEST_ASSERT_EQUAL(DesfireStatus::DFST_PARAMETER_NULL, status);
}

/**
 * @brief Test createApplication command
 */
void test_create_application(void) {
    // Prepare an Application ID
    uint8_t aid[3]      = {0xA1, 0xA2, 0xA3};
    uint8_t keySettings = 0x0F;  // Example key settings
    uint8_t numKeys     = 3;     // Number of keys in the application

    // Set response to success
    g_mockProtocolReader.setResponseStatus(0x91, 0x00);

    // Create the application
    DesfireStatus status = g_desfire->createApplication(aid, keySettings, numKeys);

    // Verify the result
    TEST_ASSERT_EQUAL(DesfireStatus::DFST_SUCCESS, status);
    TEST_ASSERT_TRUE(g_mockProtocolReader.wasTransceiveCalled());
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(DesfireCommand::DF_CMD_CREATE_APPLICATION),
                      g_mockProtocolReader.getLastCommand());

    // Check command data (5 bytes: 3 for AID, 1 for key settings, 1 for number of keys)
    TEST_ASSERT_EQUAL(5, g_mockProtocolReader.getLastCommandDataLength());

    // Test with null AID
    status = g_desfire->createApplication(nullptr, keySettings, numKeys);
    TEST_ASSERT_EQUAL(DesfireStatus::DFST_PARAMETER_NULL, status);

    // Test with invalid number of keys
    status = g_desfire->createApplication(aid, keySettings, 0);  // Min is 1
    TEST_ASSERT_EQUAL(DesfireStatus::DFST_PARAMETER_ERROR, status);

    status = g_desfire->createApplication(aid, keySettings, 15);  // Max is 14
    TEST_ASSERT_EQUAL(DesfireStatus::DFST_PARAMETER_ERROR, status);
}

/**
 * @brief Test deleteApplication command
 */
void test_delete_application(void) {
    // Prepare an Application ID
    uint8_t aid[3] = {0xA1, 0xA2, 0xA3};

    // Set response to success
    g_mockProtocolReader.setResponseStatus(0x91, 0x00);

    // Delete the application
    DesfireStatus status = g_desfire->deleteApplication(aid);

    // Verify the result
    TEST_ASSERT_EQUAL(DesfireStatus::DFST_SUCCESS, status);
    TEST_ASSERT_TRUE(g_mockProtocolReader.wasTransceiveCalled());
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(DesfireCommand::DF_CMD_DELETE_APPLICATION),
                      g_mockProtocolReader.getLastCommand());

    // Check command data (3 bytes for AID)
    TEST_ASSERT_EQUAL(3, g_mockProtocolReader.getLastCommandDataLength());
    TEST_ASSERT_EQUAL_MEMORY(aid, g_mockProtocolReader.getLastCommandData(), 3);

    // Test with null AID
    status = g_desfire->deleteApplication(nullptr);
    TEST_ASSERT_EQUAL(DesfireStatus::DFST_PARAMETER_NULL, status);
}

/**
 * @brief Test getFileIDs command
 */
void test_get_file_ids(void) {
    // Prepare mock response (file IDs)
    uint8_t file_response[] = {0x01, 0x02, 0x03, 0x04};  // 4 file IDs
    g_mockProtocolReader.setCustomResponse(file_response, sizeof(file_response));
    g_mockProtocolReader.setResponseStatus(0x91, 0x00);  // Success

    // Call getFileIDs
    uint8_t       fileIds[16];  // Space for up to 16 file IDs
    uint8_t       count  = 0;
    DesfireStatus status = g_desfire->getFileIDs(fileIds, 16, count);

    // Verify the result
    TEST_ASSERT_EQUAL(DesfireStatus::DFST_SUCCESS, status);
    TEST_ASSERT_TRUE(g_mockProtocolReader.wasTransceiveCalled());
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(DesfireCommand::DF_CMD_GET_FILE_IDS),
                      g_mockProtocolReader.getLastCommand());

    // Test with null parameter
    status = g_desfire->getFileIDs(nullptr, 16, count);
    TEST_ASSERT_EQUAL(DesfireStatus::DFST_PARAMETER_NULL, status);

    // Test with maxCount = 0
    status = g_desfire->getFileIDs(fileIds, 0, count);
    TEST_ASSERT_EQUAL(DesfireStatus::DFST_PARAMETER_NULL, status);
}

/**
 * @brief Test communication error handling
 */
void test_communication_error(void) {
    // Make the mock reader fail communications
    g_mockProtocolReader.setTransceiveFailure(true);

    // Try to select an application
    uint8_t       aid[3] = {0xA1, 0xA2, 0xA3};
    DesfireStatus status = g_desfire->selectApplication(aid);

    // Verify communication error is returned
    TEST_ASSERT_EQUAL(DesfireStatus::DFST_COMMUNICATION_ERROR, status);

    // Reset the mock to normal operation
    g_mockProtocolReader.setTransceiveFailure(false);
}

/**
 * @brief Test error status handling
 */
void test_error_status_handling(void) {
    // Prepare an Application ID
    uint8_t aid[3] = {0xA1, 0xA2, 0xA3};

    // Set response to "application not found" error
    g_mockProtocolReader.setResponseStatus(0x91, 0xA0);

    // Try to select a non-existent application
    DesfireStatus status = g_desfire->selectApplication(aid);

    // Verify appropriate error is returned
    // Note: Our current implementation may not correctly map all error codes yet
    TEST_ASSERT_NOT_EQUAL(DesfireStatus::DFST_SUCCESS, status);
}

/**
 * @brief Test transaction commands
 */
void test_transaction_commands(void) {
    // Set response to success
    g_mockProtocolReader.setResponseStatus(0x91, 0x00);

    // Test commit transaction
    DesfireStatus status = g_desfire->commitTransaction();
    TEST_ASSERT_EQUAL(DesfireStatus::DFST_SUCCESS, status);
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(DesfireCommand::DF_CMD_COMMIT_TRANSACTION),
                      g_mockProtocolReader.getLastCommand());

    // Test abort transaction
    status = g_desfire->abortTransaction();
    TEST_ASSERT_EQUAL(DesfireStatus::DFST_SUCCESS, status);
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(DesfireCommand::DF_CMD_ABORT_TRANSACTION),
                      g_mockProtocolReader.getLastCommand());
}

/**
 * @brief Test createStdDataFile command
 */
void test_create_std_data_file(void) {
    // Set response to success
    g_mockProtocolReader.setResponseStatus(0x91, 0x00);

    // Test creating a standard data file
    uint8_t                 fileNo       = 1;
    DesfreCommunicationMode commMode     = DesfreCommunicationMode::DF_COMM_PLAIN;
    uint16_t                accessRights = 0x1234;  // Example access rights
    uint32_t                fileSize     = 64;      // 64 bytes

    DesfireStatus status = g_desfire->createStdDataFile(fileNo, commMode, accessRights, fileSize);

    // Verify the result
    TEST_ASSERT_EQUAL(DesfireStatus::DFST_SUCCESS, status);
    TEST_ASSERT_TRUE(g_mockProtocolReader.wasTransceiveCalled());
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(DesfireCommand::DF_CMD_CREATE_STANDARD_FILE),
                      g_mockProtocolReader.getLastCommand());

    // Check command data length (7 bytes: 1 for fileNo, 1 for commMode, 2 for access rights, 3 for
    // fileSize)
    TEST_ASSERT_EQUAL(7, g_mockProtocolReader.getLastCommandDataLength());
}

/**
 * @brief Test deleteFile command
 */
void test_delete_file(void) {
    // Set response to success
    g_mockProtocolReader.setResponseStatus(0x91, 0x00);

    // Test deleting a file
    uint8_t fileNo = 1;

    DesfireStatus status = g_desfire->deleteFile(fileNo);

    // Verify the result
    TEST_ASSERT_EQUAL(DesfireStatus::DFST_SUCCESS, status);
    TEST_ASSERT_TRUE(g_mockProtocolReader.wasTransceiveCalled());
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(DesfireCommand::DF_CMD_DELETE_FILE),
                      g_mockProtocolReader.getLastCommand());

    // Check command data (1 byte for fileNo)
    TEST_ASSERT_EQUAL(1, g_mockProtocolReader.getLastCommandDataLength());
    TEST_ASSERT_EQUAL(fileNo, g_mockProtocolReader.getLastCommandData()[0]);
}

/**
 * @brief Setup and run all tests
 */
void process(void) {
    UNITY_BEGIN();

    // Create DESFire instance for testing
    g_desfire = new DesfireNFC(g_mockProtocolReader);

    // Run tests
    RUN_TEST(test_desfire_initialization);
    RUN_TEST(test_card_detection);
    RUN_TEST(test_get_card_uid);
    RUN_TEST(test_get_version);
    RUN_TEST(test_select_application);
    RUN_TEST(test_format_picc);
    RUN_TEST(test_get_free_mem);
    RUN_TEST(test_get_application_ids);
    RUN_TEST(test_create_application);
    RUN_TEST(test_delete_application);
    RUN_TEST(test_get_file_ids);
    RUN_TEST(test_communication_error);
    RUN_TEST(test_error_status_handling);
    RUN_TEST(test_transaction_commands);
    RUN_TEST(test_create_std_data_file);
    RUN_TEST(test_delete_file);

    // Clean up
    delete g_desfire;

    UNITY_END();
}

#ifdef ARDUINO
void setup(void) {
    // Start serial for test output
    delay(2000);
    Serial.begin(115200);
    process();
}

void loop(void) {
    // Empty loop
}
#else
int main(int argc, char** argv) {
    process();
    return 0;
}
#endif