#ifndef HELPERS_H
#define HELPERS_H

#include <Arduino.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    // Convert two hex characters to a byte.
    void char2byte(const char *str, uint8_t *value);

    // Convert a hex string (2 characters per byte) into a byte array.
    // If msb is true, the first two characters go into array[0];
    // if false, the resulting bytes are stored in reverse order.
    void chars2bytes(const char *str, uint8_t *array, bool msb);

    // Convert a single byte into two hex characters.
    void byte2char(uint8_t value, char *str);

    // Convert an array of bytes to a hex string.
    // If msb is true, the order is maintained;
    // if false, the byte order is reversed.
    void bytes2chars(const uint8_t *array, size_t array_len, char *str, bool msb);

    // Print a byte array in hexadecimal format.
    void printbytes(const uint8_t *array, size_t array_len);

#ifdef __cplusplus
}
#endif

#endif // HELPERS_H
