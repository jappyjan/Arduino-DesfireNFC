/**
 * @file DesfireErrorHandler.h
 * @brief Error handling and status code interpretation for DESFire operations
 *
 * This file provides utilities to interpret and handle DESFire status codes
 * and convert them to user-friendly error messages.
 */

#ifndef DESFIRE_ERROR_HANDLER_H
#define DESFIRE_ERROR_HANDLER_H

#include <Arduino.h>
#include "DesfireStatus.h"

/**
 * @brief Class for handling DESFire errors and status code interpretation
 */
class DesfireErrorHandler {
public:
    /**
     * @brief Get a user-friendly error message for a DESFire status code
     *
     * @param status DESFire status code to interpret
     * @return const char* Pointer to a string containing the error message
     */
    static const char* getErrorMessage(DesfireStatus status);

    /**
     * @brief Check if the status code indicates success
     *
     * @param status DESFire status code to check
     * @return true if the status indicates success
     * @return false if the status indicates an error
     */
    static bool isSuccess(DesfireStatus status);

    /**
     * @brief Check if the status code indicates that more data frames are expected
     *
     * @param status DESFire status code to check
     * @return true if more data frames are expected
     * @return false if no more data frames are expected
     */
    static bool isMoreFrames(DesfireStatus status);

    /**
     * @brief Get the generic category of an error
     *
     * @param status DESFire status code to categorize
     * @return const char* Category name (e.g., "Authentication", "File", "Application")
     */
    static const char* getErrorCategory(DesfireStatus status);

    /**
     * @brief Check if the error is related to authentication
     *
     * @param status DESFire status code to check
     * @return true if the error is authentication-related
     * @return false if the error is not authentication-related
     */
    static bool isAuthenticationError(DesfireStatus status);

    /**
     * @brief Check if the error is related to file operations
     *
     * @param status DESFire status code to check
     * @return true if the error is file-related
     * @return false if the error is not file-related
     */
    static bool isFileError(DesfireStatus status);

    /**
     * @brief Check if the error is related to application operations
     *
     * @param status DESFire status code to check
     * @return true if the error is application-related
     * @return false if the error is not application-related
     */
    static bool isApplicationError(DesfireStatus status);

    /**
     * @brief Check if the error is a communication error
     *
     * @param status DESFire status code to check
     * @return true if the error is communication-related
     * @return false if the error is not communication-related
     */
    static bool isCommunicationError(DesfireStatus status);

    /**
     * @brief Print a detailed error report to the serial output
     *
     * @param status DESFire status code to report
     * @param operation Optional name of the operation that caused the error
     */
    static void printErrorReport(DesfireStatus status, const char* operation = nullptr);
};

#endif  // DESFIRE_ERROR_HANDLER_H