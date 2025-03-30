/**
 * @file test_setup.cpp
 * @brief Setup file for ISO7816APDU command parsing tests
 */

#include <Arduino.h>

// This file is primarily used to set up and configure the test environment.
// For the ISO7816APDU tests, we don't need any special setup as we're
// mainly testing static methods.

// Implement required Arduino functions for non-Arduino environments
#ifndef ARDUINO
void delay(unsigned long) {
}
#endif