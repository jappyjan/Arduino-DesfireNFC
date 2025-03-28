#include <Arduino.h>
// This file intentionally has poor formatting to test the linter
void badlyFormattedFunction(int x, int y) {
    if (x > 10) {
        int z = x + y;
        for (int i = 0; i < 10; i++) {
            z += i;
            if (z > 100)
                break;
        }
    }
    return;
}

class TestClass {
public:
    TestClass() {
        _value = 0;
    }

    void setValue(uint8_t value) {
        _value = value;
    }

private:
    uint8_t _value;
};