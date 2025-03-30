#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include <stdint.h>

class LedControl
{
public:
    // Initialize LED pins
    static void begin();

    // Basic control functions
    static void showError();
    static void showSuccess();
    static void clearAll();

    // Convenience functions
    static void blinkError(uint16_t duration_ms = 500);
    static void blinkSuccess(uint16_t duration_ms = 500);
};

#endif