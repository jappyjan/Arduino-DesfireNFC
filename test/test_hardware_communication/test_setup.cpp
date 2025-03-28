#include <Arduino.h>

// These functions are required by the Arduino framework
// but are defined in test_main.cpp with an ARDUINO guard.
// This file ensures they're available for the linker.

void setup() {
    delay(2000);  // Give the serial port time to initialize
    Serial.begin(115200);
    Serial.println("Hardware test setup");
}

void loop() {
    // Empty - tests are run in the process() function called from setup()
}