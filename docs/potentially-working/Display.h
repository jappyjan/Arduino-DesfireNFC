#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define DISPLAY_BUFFER_SIZE 30

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset)

class Display
{
private:
    Adafruit_SSD1306 *display;
    int readerid;
    char ReaderInfo[DISPLAY_BUFFER_SIZE];
    char Title[DISPLAY_BUFFER_SIZE];
    char Info[DISPLAY_BUFFER_SIZE];

    // Helper function to center text horizontally at a given y coordinate.
    void drawCenteredText(const char *text, int y);

public:
    // The constructor takes the SDA and SCL pins (which are used to initialize Wire)
    // and the reader ID.
    Display(int sda, int scl, int readerid);

    void createReaderInfo(const char *state);
    void clearReaderInfo();
    void writeReaderInfo(const char *text);
    void writeTitle(const char *text);
    void writeTitle(const String &text);
    void writeInfo(const char *text);
    void writeInfo(const String &text);
    void updateDisplay();
    void updateByMQTT(const char *topic, byte *payload, unsigned int length);
};

#endif
