#include "LedControl.h"
#include "Pins.h"
#include <Arduino.h>

void LedControl::begin()
{
    pinMode(PIN_ERROR_LED, OUTPUT);
    pinMode(PIN_SUCCESS_LED, OUTPUT);
    // Initialize both LEDs as off
    digitalWrite(PIN_ERROR_LED, LOW);
    digitalWrite(PIN_SUCCESS_LED, LOW);
}

void LedControl::showError()
{
    digitalWrite(PIN_ERROR_LED, HIGH);
    digitalWrite(PIN_SUCCESS_LED, LOW);
}

void LedControl::showSuccess()
{
    digitalWrite(PIN_ERROR_LED, LOW);
    digitalWrite(PIN_SUCCESS_LED, HIGH);
}

void LedControl::clearAll()
{
    digitalWrite(PIN_ERROR_LED, LOW);
    digitalWrite(PIN_SUCCESS_LED, LOW);
}

void LedControl::blinkError(uint16_t duration_ms)
{
    showError();
    delay(duration_ms);
    clearAll();
}

void LedControl::blinkSuccess(uint16_t duration_ms)
{
    showSuccess();
    delay(duration_ms);
    clearAll();
}