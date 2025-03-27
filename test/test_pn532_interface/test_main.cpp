/**
 * @file test_main.cpp
 * @brief Unit tests for the PN532Reader implementation
 */

#include <Arduino.h>
#include <unity.h>
#include "NFCReaderInterface.h"
#include <SPI.h>

// Create a custom test version of PN532Reader that we can control for testing
class TestReaderPN532 : public NFCReaderInterface {
public:
    TestReaderPN532() {
        _firmwareVersion = 0x12345678;
        _failDetect = false;
        _failTransceive = false;
        _retries = 0;
    }
    
    bool begin() override {
        return true;
    }
    
    uint32_t getFirmwareVersion() override {
        return _firmwareVersion;
    }
    
    bool configure() override {
        _retries = 0xFF; // Store the configured retries value
        return true;
    }
    
    bool detectCard(uint8_t *uid, uint8_t *uidLength) override {
        if (_failDetect) {
            return false;
        }
        
        // Simulate a DESFire card
        *uidLength = 7;
        uid[0] = 0x04;
        uid[1] = 0xE5;
        uid[2] = 0xF2;
        uid[3] = 0x3A;
        uid[4] = 0x89;
        uid[5] = 0xC5;
        uid[6] = 0xD1;
        
        return true;
    }
    
    bool transceive(const uint8_t *txData, uint8_t txLength, 
                   uint8_t *rxData, uint8_t *rxLength) override {
        if (_failTransceive) {
            return false;
        }
        
        // Simulate a simple response
        rxData[0] = 0x90;
        rxData[1] = 0x00;
        *rxLength = 2;
        
        return true;
    }
    
    // Test control functions
    void setFailDetect(bool fail) { _failDetect = fail; }
    void setFailTransceive(bool fail) { _failTransceive = fail; }
    uint8_t getRetries() { return _retries; }
    void setFirmwareVersion(uint32_t version) { _firmwareVersion = version; }
    
private:
    bool _failDetect;
    bool _failTransceive;
    uint8_t _retries;
    uint32_t _firmwareVersion;
};

// Test fixture
TestReaderPN532 *reader;

void setUp(void) {
    // Create a new TestReaderPN532 for each test
    reader = new TestReaderPN532();
}

void tearDown(void) {
    delete reader;
    reader = nullptr;
}

void test_begin_success(void) {
    TEST_ASSERT_TRUE(reader->begin());
}

void test_firmware_version(void) {
    TEST_ASSERT_EQUAL_HEX32(0x12345678, reader->getFirmwareVersion());
}

void test_configure(void) {
    TEST_ASSERT_TRUE(reader->configure());
    TEST_ASSERT_EQUAL(0xFF, reader->getRetries());
}

void test_detect_card_success(void) {
    uint8_t uid[7];
    uint8_t uidLength;
    
    reader->setFailDetect(false);
    TEST_ASSERT_TRUE(reader->detectCard(uid, &uidLength));
    TEST_ASSERT_EQUAL(7, uidLength);
    TEST_ASSERT_EQUAL_HEX8(0x04, uid[0]);
    TEST_ASSERT_EQUAL_HEX8(0xE5, uid[1]);
    TEST_ASSERT_EQUAL_HEX8(0xF2, uid[2]);
    TEST_ASSERT_EQUAL_HEX8(0x3A, uid[3]);
    TEST_ASSERT_EQUAL_HEX8(0x89, uid[4]);
    TEST_ASSERT_EQUAL_HEX8(0xC5, uid[5]);
    TEST_ASSERT_EQUAL_HEX8(0xD1, uid[6]);
}

void test_detect_card_failure(void) {
    uint8_t uid[7];
    uint8_t uidLength;
    
    reader->setFailDetect(true);
    TEST_ASSERT_FALSE(reader->detectCard(uid, &uidLength));
}

void test_transceive_success(void) {
    const uint8_t txData[] = {0x90, 0x00, 0x00, 0x00, 0x00};
    uint8_t rxData[32];
    uint8_t rxLength = sizeof(rxData);
    
    reader->setFailTransceive(false);
    TEST_ASSERT_TRUE(reader->transceive(txData, sizeof(txData), rxData, &rxLength));
    TEST_ASSERT_EQUAL(2, rxLength);
    TEST_ASSERT_EQUAL_HEX8(0x90, rxData[0]);
    TEST_ASSERT_EQUAL_HEX8(0x00, rxData[1]);
}

void test_transceive_failure(void) {
    const uint8_t txData[] = {0x90, 0x00, 0x00, 0x00, 0x00};
    uint8_t rxData[32];
    uint8_t rxLength = sizeof(rxData);
    
    reader->setFailTransceive(true);
    TEST_ASSERT_FALSE(reader->transceive(txData, sizeof(txData), rxData, &rxLength));
}

void process(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_begin_success);
    RUN_TEST(test_firmware_version);
    RUN_TEST(test_configure);
    RUN_TEST(test_detect_card_success);
    RUN_TEST(test_detect_card_failure);
    RUN_TEST(test_transceive_success);
    RUN_TEST(test_transceive_failure);
    
    UNITY_END();
}

#ifdef ARDUINO
void setup(void) {
    delay(2000); // Give the serial port time to initialize
    process();
}

void loop(void) {
    // Empty
}
#else
int main(int argc, char **argv) {
    process();
    return 0;
}
#endif 