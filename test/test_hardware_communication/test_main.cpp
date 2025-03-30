/**
 * @file test_main.cpp
 * @brief Unit tests for the DesfireNFC library using mock PN532 hardware
 */

#include <Arduino.h>
#include <SPI.h>
#include <unity.h>
#include "DesfireNFC.h"
#include "NFCReaderInterface.h"

// Create a custom mock for Adafruit_PN532 with controlled behavior
class MockPN532 {
public:
    MockPN532() {
        _firmwareVersion       = 0x12345678;  // Default
        _failDetectCard        = false;
        _failDataExchange      = false;
        _beginCalled           = false;
        _samConfigCalled       = false;
        _detectCardCallCount   = 0;
        _dataExchangeCallCount = 0;
        _lastCommandSent       = 0;  // Initialize the member variable
        _responseStatus        = 0x00;
        _responseSubstatus     = 0x00;
    }

    void reset() {
        _beginCalled           = false;
        _samConfigCalled       = false;
        _detectCardCallCount   = 0;
        _dataExchangeCallCount = 0;

        // Default response for commands
        _responseStatus    = 0x91;
        _responseSubstatus = 0x00;
    }

    bool begin() {
        _beginCalled = true;
        return true;
    }

    uint32_t getFirmwareVersion() {
        return _firmwareVersion;
    }

    bool SAMConfig() {
        _samConfigCalled = true;
        return true;
    }

    bool readPassiveTargetID(uint8_t cardbaudrate, uint8_t* uid, uint8_t* uidLength) {
        _detectCardCallCount++;

        if (_failDetectCard) {
            return false;
        }

        // Simulate a DESFire card
        *uidLength = 7;
        uid[0]     = 0x04;  // Manufacturer ID for NXP
        uid[1]     = 0xAA;
        uid[2]     = 0xBB;
        uid[3]     = 0xCC;
        uid[4]     = 0xDD;
        uid[5]     = 0xEE;
        uid[6]     = 0xFF;

        return true;
    }

    bool inDataExchange(uint8_t* send,
                        uint8_t  sendLength,
                        uint8_t* response,
                        uint8_t* responseLength) {
        _dataExchangeCallCount++;
        if (sendLength >= 2) {
            _lastCommandSent = send[1];  // Store the command byte for verification
        }

        if (_failDataExchange) {
            return false;
        }

        // Process based on DESFire command
        if (sendLength >= 2) {
            uint8_t cmd = send[1];
            switch (cmd) {
                case 0x60:  // Get Version command
                    response[0]     = _responseStatus;
                    response[1]     = _responseSubstatus;
                    *responseLength = 2;
                    break;
                case 0x5A:  // Select Application
                    response[0]     = _responseStatus;
                    response[1]     = _responseSubstatus;
                    *responseLength = 2;
                    break;
                case 0x45:  // Get Free Memory
                    // Return some memory value (20000 bytes) plus status
                    response[0]     = 0x20;
                    response[1]     = 0x4E;
                    response[2]     = 0x00;
                    response[3]     = _responseStatus;
                    response[4]     = _responseSubstatus;
                    *responseLength = 5;
                    break;
                default:
                    // Default response
                    response[0]     = _responseStatus;
                    response[1]     = _responseSubstatus;
                    *responseLength = 2;
                    break;
            }
        } else {
            // Invalid command
            response[0]     = _responseStatus;
            response[1]     = _responseSubstatus;
            *responseLength = 2;
        }

        return true;
    }

    // Testing control functions
    void setFirmwareVersion(uint32_t version) {
        _firmwareVersion = version;
    }
    void setFailDetectCard(bool fail) {
        _failDetectCard = fail;
    }
    void setFailDataExchange(bool fail) {
        _failDataExchange = fail;
    }
    void setResponseStatus(uint8_t status, uint8_t substatus) {
        _responseStatus    = status;
        _responseSubstatus = substatus;
    }

    // Verification functions
    bool wasBeginCalled() {
        return _beginCalled;
    }
    bool wasSAMConfigCalled() {
        return _samConfigCalled;
    }
    int getDetectCardCallCount() {
        return _detectCardCallCount;
    }
    int getDataExchangeCallCount() {
        return _dataExchangeCallCount;
    }
    uint8_t getLastCommandSent() {
        return _lastCommandSent;
    }

private:
    uint32_t _firmwareVersion;
    bool     _failDetectCard;
    bool     _failDataExchange;
    bool     _beginCalled;
    bool     _samConfigCalled;
    int      _detectCardCallCount;
    int      _dataExchangeCallCount;
    uint8_t  _lastCommandSent;
    uint8_t  _responseStatus;
    uint8_t  _responseSubstatus;
};

// Custom PN532Reader implementation that uses our mock
class TestPN532Reader : public NFCReaderInterface {
public:
    TestPN532Reader(MockPN532* mock) : _mock(mock), _lastCommandSent(0) {
    }

    bool begin() override {
        return _mock->begin();
    }

    uint32_t getFirmwareVersion() override {
        return _mock->getFirmwareVersion();
    }

    bool configure() override {
        return _mock->SAMConfig();
    }

    bool detectCard(uint8_t* uid, uint8_t* uidLength) override {
        return _mock->readPassiveTargetID(0, uid, uidLength);
    }

    bool transceive(const uint8_t* txData,
                    uint16_t       txLength,
                    uint8_t*       rxData,
                    uint16_t*      rxLength) override {
        // In an ISO7816 APDU format for DESFire, the command structure is typically:
        // [CLA, INS, P1, P2, Lc, Data...]
        // For DESFire commands, the DESFire command is the first byte of the Data...
        if (txLength >= 6) {
            // Extract command
            uint8_t cmdPos = 5;               // Position of the first data byte
            uint8_t dfCmd  = txData[cmdPos];  // DESFire command

            // Create buffer for MockPN532
            uint8_t sendBuf[255];
            uint8_t sendLen = 2;  // At minimum, includes command

            // In a real PN532, there would be a header before the DESFire command
            // For our mock, we just need the DESFire command at position 1
            sendBuf[0] = 0xD4;   // Dummy header
            sendBuf[1] = dfCmd;  // DESFire command

            // Copy any data that follows the command
            if (txLength > cmdPos + 1) {
                uint8_t dataLen = txLength - (cmdPos + 1);
                if (dataLen > 0) {
                    memcpy(&sendBuf[2], &txData[cmdPos + 1], dataLen);
                    sendLen += dataLen;
                }
            }

            // Call the mock
            uint8_t mockResponse[255];
            uint8_t mockResponseLen = 0;

            bool success = _mock->inDataExchange(sendBuf, sendLen, mockResponse, &mockResponseLen);

            if (success && mockResponseLen > 0) {
                // Copy the response
                memcpy(rxData, mockResponse, mockResponseLen);
                *rxLength = mockResponseLen;
                return true;
            } else {
                *rxLength = 0;
                return false;
            }
        }

        // Default case - shouldn't reach here in normal operation
        rxData[0] = 0x91;  // DESFire success status
        rxData[1] = 0x00;  // No error
        *rxLength = 2;
        return true;
    }

    // Expose the mock for test verification
    MockPN532* getMock() {
        return _mock;
    }

private:
    MockPN532* _mock;
    uint8_t    _lastCommandSent;
};

// Test fixture
MockPN532*       mockPN532;
TestPN532Reader* pn532Reader;
DesfireNFC*      nfc;

void setUp(void) {
    mockPN532   = new MockPN532();
    pn532Reader = new TestPN532Reader(mockPN532);
    nfc         = new DesfireNFC(*pn532Reader);
    mockPN532->reset();
}

void tearDown(void) {
    delete nfc;
    delete pn532Reader;
    delete mockPN532;
    nfc         = nullptr;
    pn532Reader = nullptr;
    mockPN532   = nullptr;
}

void test_initialize_hardware(void) {
    // Test initialization
    TEST_ASSERT_TRUE(nfc->initialize());
    TEST_ASSERT_TRUE(mockPN532->wasBeginCalled());
    TEST_ASSERT_TRUE(mockPN532->wasSAMConfigCalled());
}

void test_detect_card(void) {
    // Test successful card detection
    mockPN532->setFailDetectCard(false);
    TEST_ASSERT_TRUE(nfc->detectCard());
    TEST_ASSERT_EQUAL(1, mockPN532->getDetectCardCallCount());

    // Test failed card detection
    mockPN532->setFailDetectCard(true);
    TEST_ASSERT_FALSE(nfc->detectCard());
    TEST_ASSERT_EQUAL(2, mockPN532->getDetectCardCallCount());
}

void test_get_card_uid(void) {
    // First detect a card
    mockPN532->setFailDetectCard(false);
    TEST_ASSERT_TRUE(nfc->detectCard());

    // Now get the UID
    uint8_t uid[10];
    uint8_t uidLength;
    TEST_ASSERT_TRUE(nfc->getCardUID(uid, &uidLength));

    // Check UID values
    TEST_ASSERT_EQUAL(7, uidLength);
    TEST_ASSERT_EQUAL_HEX8(0x04, uid[0]);
    TEST_ASSERT_EQUAL_HEX8(0xAA, uid[1]);
    TEST_ASSERT_EQUAL_HEX8(0xBB, uid[2]);
    TEST_ASSERT_EQUAL_HEX8(0xCC, uid[3]);
    TEST_ASSERT_EQUAL_HEX8(0xDD, uid[4]);
    TEST_ASSERT_EQUAL_HEX8(0xEE, uid[5]);
    TEST_ASSERT_EQUAL_HEX8(0xFF, uid[6]);
}

void test_version_info(void) {
    // First detect a card
    mockPN532->setFailDetectCard(false);
    TEST_ASSERT_TRUE(nfc->detectCard());

    // Set response for version command
    mockPN532->setResponseStatus(0x91, 0x00);

    // Get version info
    DesfireStatus status = nfc->getVersion();
    Serial.print("Version test status: ");
    Serial.println((int)status);

    TEST_ASSERT_EQUAL(DesfireStatus::DFST_SUCCESS, status);
    TEST_ASSERT_TRUE(mockPN532->getDataExchangeCallCount() > 0);
}

void process(void) {
    UNITY_BEGIN();

    RUN_TEST(test_initialize_hardware);
    RUN_TEST(test_detect_card);
    RUN_TEST(test_get_card_uid);
    RUN_TEST(test_version_info);

    UNITY_END();
}

#ifdef ARDUINO
void setup(void) {
    delay(2000);  // Give the serial port time to initialize
    process();
}

void loop(void) {
    // Empty
}
#else
int main(int argc, char** argv) {
    process();
    return 0;
}
#endif