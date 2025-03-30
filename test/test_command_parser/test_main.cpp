/**
 * @file test_main.cpp
 * @brief Unit tests for the ISO7816APDU command building and parsing functionality
 */

#include <Arduino.h>
#include <unity.h>
#include "DesfireStatus.h"
#include "DesfireTypes.h"
#include "ISO7816APDU.h"
#include "ISO7816Constants.h"

void setUp(void) {
    // Set up code for each test
}

void tearDown(void) {
    // Clean up code for each test
}

/**
 * @brief Test basic command building functionality
 */
void test_build_command_basic(void) {
    // Set up a simple command
    ISO7816APDU::Command cmd;
    cmd.cla        = 0x90;  // DESFire class
    cmd.ins        = 0x00;  // DESFire native command
    cmd.p1         = 0x00;
    cmd.p2         = 0x00;
    cmd.data[0]    = 0x60;  // GET_VERSION command
    cmd.dataLength = 1;
    cmd.le         = 0x00;  // Request all available data

    // Build the APDU
    uint8_t       apdu[ISO7816Constants::ISO_MAX_APDU_SIZE];
    uint8_t       apduLength = 0;
    DesfireStatus status     = ISO7816APDU::buildCommand(cmd, apdu, &apduLength);

    // Check status
    TEST_ASSERT_EQUAL(DesfireStatus::DFST_SUCCESS, status);

    // Check APDU length (CLA+INS+P1+P2+Lc+Data+Le = 7 bytes)
    TEST_ASSERT_EQUAL(6, apduLength);

    // Check APDU content
    TEST_ASSERT_EQUAL(0x90, apdu[0]);  // CLA
    TEST_ASSERT_EQUAL(0x00, apdu[1]);  // INS
    TEST_ASSERT_EQUAL(0x00, apdu[2]);  // P1
    TEST_ASSERT_EQUAL(0x00, apdu[3]);  // P2
    TEST_ASSERT_EQUAL(0x01, apdu[4]);  // Lc
    TEST_ASSERT_EQUAL(0x60, apdu[5]);  // Data
    // No Le field is present
}

/**
 * @brief Test command building with no data
 */
void test_build_command_no_data(void) {
    // Set up a command with no data
    ISO7816APDU::Command cmd;
    cmd.cla        = 0x90;
    cmd.ins        = 0x00;
    cmd.p1         = 0x00;
    cmd.p2         = 0x00;
    cmd.dataLength = 0;
    cmd.le         = 0x00;

    // Build the APDU
    uint8_t       apdu[ISO7816Constants::ISO_MAX_APDU_SIZE];
    uint8_t       apduLength = 0;
    DesfireStatus status     = ISO7816APDU::buildCommand(cmd, apdu, &apduLength);

    // Check status
    TEST_ASSERT_EQUAL(DesfireStatus::DFST_SUCCESS, status);

    // Check APDU length (CLA+INS+P1+P2+Le = 4 bytes, no Lc or data)
    TEST_ASSERT_EQUAL(4, apduLength);

    // Check APDU content
    TEST_ASSERT_EQUAL(0x90, apdu[0]);  // CLA
    TEST_ASSERT_EQUAL(0x00, apdu[1]);  // INS
    TEST_ASSERT_EQUAL(0x00, apdu[2]);  // P1
    TEST_ASSERT_EQUAL(0x00, apdu[3]);  // P2
    // No Le field is present
}

/**
 * @brief Test command building with data but no Le
 */
void test_build_command_no_le(void) {
    // Set up a command with data but no Le
    ISO7816APDU::Command cmd;
    cmd.cla        = 0x90;
    cmd.ins        = 0x5A;  // SELECT_APPLICATION
    cmd.p1         = 0x00;
    cmd.p2         = 0x00;
    cmd.data[0]    = 0xAA;  // Sample AID (3 bytes)
    cmd.data[1]    = 0xBB;
    cmd.data[2]    = 0xCC;
    cmd.dataLength = 3;
    cmd.le         = 0;  // No expected response length

    // Build the APDU
    uint8_t       apdu[ISO7816Constants::ISO_MAX_APDU_SIZE];
    uint8_t       apduLength = 0;
    DesfireStatus status     = ISO7816APDU::buildCommand(cmd, apdu, &apduLength);

    // Check status
    TEST_ASSERT_EQUAL(DesfireStatus::DFST_SUCCESS, status);

    // Check APDU length (CLA+INS+P1+P2+Lc+Data = 8 bytes, no Le)
    TEST_ASSERT_EQUAL(8, apduLength);

    // Check APDU content
    TEST_ASSERT_EQUAL(0x90, apdu[0]);  // CLA
    TEST_ASSERT_EQUAL(0x5A, apdu[1]);  // INS
    TEST_ASSERT_EQUAL(0x00, apdu[2]);  // P1
    TEST_ASSERT_EQUAL(0x00, apdu[3]);  // P2
    TEST_ASSERT_EQUAL(0x03, apdu[4]);  // Lc
    TEST_ASSERT_EQUAL(0xAA, apdu[5]);  // Data[0]
    TEST_ASSERT_EQUAL(0xBB, apdu[6]);  // Data[1]
    TEST_ASSERT_EQUAL(0xCC, apdu[7]);  // Data[2]
}

/**
 * @brief Test command building with null parameters
 */
void test_build_command_null_params(void) {
    // Set up a simple command
    ISO7816APDU::Command cmd;
    cmd.cla        = 0x90;
    cmd.ins        = 0x00;
    cmd.p1         = 0x00;
    cmd.p2         = 0x00;
    cmd.dataLength = 0;
    cmd.le         = 0x00;

    // Test with null APDU
    uint8_t       apduLength = 0;
    DesfireStatus status     = ISO7816APDU::buildCommand(cmd, nullptr, &apduLength);
    TEST_ASSERT_EQUAL(DesfireStatus::DFST_PARAMETER_NULL, status);

    // Test with null length pointer
    uint8_t apdu[ISO7816Constants::ISO_MAX_APDU_SIZE];
    status = ISO7816APDU::buildCommand(cmd, apdu, nullptr);
    TEST_ASSERT_EQUAL(DesfireStatus::DFST_PARAMETER_NULL, status);
}

/**
 * @brief Test response parsing functionality
 */
void test_parse_response_basic(void) {
    // Create a sample response (data + status word)
    uint8_t responseData[] = {
        0x01, 0x02, 0x03, 0x91, 0x00};  // 3 bytes data + SW1(0x91) + SW2(0x00)
    uint8_t responseLength = sizeof(responseData);

    // Parse the response
    ISO7816APDU::Response result;
    DesfireStatus         status = ISO7816APDU::parseResponse(responseData, responseLength, result);

    // Check status
    TEST_ASSERT_EQUAL(DesfireStatus::DFST_SUCCESS, status);

    // Check parsed values
    TEST_ASSERT_EQUAL(3, result.dataLength);
    TEST_ASSERT_EQUAL(0x01, result.data[0]);
    TEST_ASSERT_EQUAL(0x02, result.data[1]);
    TEST_ASSERT_EQUAL(0x03, result.data[2]);
    TEST_ASSERT_EQUAL(0x9100, result.status);  // SW1-SW2 combined
}

/**
 * @brief Test response parsing with only status word (no data)
 */
void test_parse_response_no_data(void) {
    // Create a sample response (only status word)
    uint8_t responseData[] = {0x91, 0x00};  // SW1(0x91) + SW2(0x00)
    uint8_t responseLength = sizeof(responseData);

    // Parse the response
    ISO7816APDU::Response result;
    DesfireStatus         status = ISO7816APDU::parseResponse(responseData, responseLength, result);

    // Check status
    TEST_ASSERT_EQUAL(DesfireStatus::DFST_SUCCESS, status);

    // Check parsed values
    TEST_ASSERT_EQUAL(0, result.dataLength);
    TEST_ASSERT_EQUAL(0x9100, result.status);
}

/**
 * @brief Test response parsing with null parameters
 */
void test_parse_response_null_params(void) {
    // Create a sample response
    uint8_t responseData[] = {0x91, 0x00};
    uint8_t responseLength = sizeof(responseData);

    // Test with null response data
    ISO7816APDU::Response result;
    DesfireStatus         status = ISO7816APDU::parseResponse(nullptr, responseLength, result);
    TEST_ASSERT_EQUAL(DesfireStatus::DFST_PARAMETER_ERROR, status);
}

/**
 * @brief Test response parsing with invalid length
 */
void test_parse_response_invalid_length(void) {
    // Create a sample response with insufficient length
    uint8_t responseData[] = {0x91};  // Only SW1, missing SW2
    uint8_t responseLength = sizeof(responseData);

    // Parse the response
    ISO7816APDU::Response result;
    DesfireStatus         status = ISO7816APDU::parseResponse(responseData, responseLength, result);

    // Check status
    TEST_ASSERT_EQUAL(DesfireStatus::DFST_PARAMETER_ERROR, status);
}

/**
 * @brief Test status code conversion
 */
void test_status_conversion(void) {
    // Test some common status words
    TEST_ASSERT_EQUAL(DesfireStatus::DFST_SUCCESS, ISO7816APDU::convertStatus(0x9100));
    TEST_ASSERT_EQUAL(DesfireStatus::DFST_MORE_FRAMES, ISO7816APDU::convertStatus(0x91AF));
    TEST_ASSERT_EQUAL(DesfireStatus::DFST_AUTHENTICATION_ERROR, ISO7816APDU::convertStatus(0x91AE));
    TEST_ASSERT_EQUAL(DesfireStatus::DFST_PARAMETER_ERROR, ISO7816APDU::convertStatus(0x911F));
    TEST_ASSERT_EQUAL(DesfireStatus::DFST_FILE_NOT_FOUND, ISO7816APDU::convertStatus(0x91F0));

    // Test ISO7816 general status words
    TEST_ASSERT_EQUAL(DesfireStatus::DFST_SUCCESS, ISO7816APDU::convertStatus(0x9000));
    TEST_ASSERT_EQUAL(DesfireStatus::DFST_ISO_WRONG_LENGTH, ISO7816APDU::convertStatus(0x6700));
    TEST_ASSERT_EQUAL(DesfireStatus::DFST_ISO_SECURITY_STATUS_ERROR,
                      ISO7816APDU::convertStatus(0x6982));
    TEST_ASSERT_EQUAL(DesfireStatus::DFST_ISO_FILE_NOT_FOUND, ISO7816APDU::convertStatus(0x6A82));
    TEST_ASSERT_EQUAL(DesfireStatus::DFST_ISO_UNKNOWN_INSTRUCTION,
                      ISO7816APDU::convertStatus(0x6D00));

    // Test unknown status word
    TEST_ASSERT_EQUAL(DesfireStatus::DFST_LIBRARY_ERROR, ISO7816APDU::convertStatus(0x6999));
}

/**
 * @brief Test success status check
 */
void test_is_success(void) {
    // Test success status words
    TEST_ASSERT_TRUE(ISO7816APDU::isSuccess(0x9000));
    TEST_ASSERT_TRUE(ISO7816APDU::isSuccess(0x9100));

    // Test non-success status words
    TEST_ASSERT_FALSE(ISO7816APDU::isSuccess(0x91AE));  // Authentication error
    TEST_ASSERT_FALSE(ISO7816APDU::isSuccess(0x6982));  // Security not satisfied
    TEST_ASSERT_FALSE(ISO7816APDU::isSuccess(0x6A82));  // File not found
}

/**
 * @brief Run all command parser tests
 */
void process(void) {
    UNITY_BEGIN();

    // Command building tests
    RUN_TEST(test_build_command_basic);
    RUN_TEST(test_build_command_no_data);
    RUN_TEST(test_build_command_no_le);
    RUN_TEST(test_build_command_null_params);

    // Response parsing tests
    RUN_TEST(test_parse_response_basic);
    RUN_TEST(test_parse_response_no_data);
    RUN_TEST(test_parse_response_null_params);
    RUN_TEST(test_parse_response_invalid_length);

    // Status code tests
    RUN_TEST(test_status_conversion);
    RUN_TEST(test_is_success);

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