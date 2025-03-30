#include "helpers.h"

// Helper: convert a single hex char into a nibble (4 bits)
static inline uint8_t hex2nibble(char c)
{
    if (c >= '0' && c <= '9')
    {
        return c - '0';
    }
    if (c >= 'A' && c <= 'F')
    {
        return c - 'A' + 10;
    }
    if (c >= 'a' && c <= 'f')
    {
        return c - 'a' + 10;
    }
    return 0; // or handle error
}

// Convert two hex characters to a byte.
void char2byte(const char *str, uint8_t *value)
{
    *value = (hex2nibble(str[0]) << 4) | hex2nibble(str[1]);
}

// Convert a hex string (2 chars per byte) into a byte array.
// If msb is true, the first two chars go into array[0]; if false, the byte order is reversed.
void chars2bytes(const char *str, uint8_t *array, bool msb)
{
    size_t len = strlen(str);
    size_t byteCount = len / 2;

    if (msb)
    {
        for (size_t i = 0; i < byteCount; i++)
        {
            array[i] = (hex2nibble(str[2 * i]) << 4) | hex2nibble(str[2 * i + 1]);
        }
    }
    else
    {
        for (size_t i = 0; i < byteCount; i++)
        {
            array[byteCount - 1 - i] = (hex2nibble(str[2 * i]) << 4) | hex2nibble(str[2 * i + 1]);
        }
    }
}

// Convert a single byte into two hex characters.
void byte2char(uint8_t value, char *str)
{
    const char hexDigits[] = "0123456789abcdef";
    str[0] = hexDigits[value >> 4];
    str[1] = hexDigits[value & 0x0F];
    str[2] = '\0';
}

// Convert an array of bytes to a hex string.
// If msb is true, the order is maintained; if false, the order is reversed.
void bytes2chars(const uint8_t *array, size_t array_len, char *str, bool msb)
{
    if (msb)
    {
        for (size_t i = 0; i < array_len; i++)
        {
            byte2char(array[i], &str[i * 2]);
        }
    }
    else
    {
        for (size_t i = 0; i < array_len; i++)
        {
            byte2char(array[array_len - 1 - i], &str[i * 2]);
        }
    }
    str[array_len * 2] = '\0';
}

// Print a byte array in hexadecimal format.
void printbytes(const uint8_t *array, size_t array_len)
{
    Serial.print("0x");
    for (size_t i = 0; i < array_len; i++)
    {
        Serial.printf("%02x", array[i]);
    }
    Serial.println();
}
