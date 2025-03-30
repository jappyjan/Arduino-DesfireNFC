#include "Display.h"
#include <Wire.h>
#include <stdio.h>
#include <string.h>
#include <ArduinoLog.h>

// Constructor: initializes I2C, creates the display object,
// initializes the display, clears it, and sets an initial rotation.
Display::Display(int sda, int scl, int readerid)
{
    this->readerid = readerid;

    // Initialize I2C with custom SDA and SCL pins.
    Wire.begin(sda, scl);

    // Create display object (128x64) using I2C and default I2C address 0x3C.
    display = new Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
    if (!display->begin(SSD1306_SWITCHCAPVCC, 0x3C))
    {
        Log.error(F("SSD1306 allocation failed"));
        while (1)
        {
            yield(); // Infinite loop if display initialization fails
        }
    }
    display->clearDisplay();

    // Flip the display vertically (if needed)
    display->setRotation(0);

    // Initialize text buffers.
    ReaderInfo[0] = '\0';      // Empty string
    strcpy(Title, "Boot ..."); // Initial title
    Info[0] = '\0';            // Empty string
}

void Display::clearReaderInfo()
{
    char buffer[DISPLAY_BUFFER_SIZE];
    // Format the reader id as a 5-digit number.
    sprintf(buffer, "%05d", readerid);
    writeReaderInfo(buffer);
}

void Display::createReaderInfo(const char *state)
{
    // Ensure there is enough room in the buffer (6 extra chars for the readerid)
    if (strlen(state) < DISPLAY_BUFFER_SIZE - 1 - 6)
    {
        char buffer[DISPLAY_BUFFER_SIZE];
        sprintf(buffer, "%s %05d", state, readerid);
        writeReaderInfo(buffer);
    }
}

void Display::writeReaderInfo(const char *text)
{
    if (strlen(text) < DISPLAY_BUFFER_SIZE - 1)
    {
        strcpy(ReaderInfo, text);
    }
    updateDisplay();
}

void Display::writeTitle(const char *text)
{
    if (strlen(text) < DISPLAY_BUFFER_SIZE - 1)
    {
        strcpy(Title, text);
    }

    Log.trace("Display Title: ", text);

    updateDisplay();
}

void Display::writeInfo(const char *text)
{
    if (strlen(text) < DISPLAY_BUFFER_SIZE - 1)
    {
        strcpy(Info, text);
    }

    Log.trace("Display Info: ", text);

    updateDisplay();
}

// Add a new writeInfo method that accepts String objects directly
void Display::writeInfo(const String &text)
{
    writeInfo(text.c_str());
}

// Add a new writeTitle method that accepts String objects directly
void Display::writeTitle(const String &text)
{
    writeTitle(text.c_str());
}

// Helper: Draws the given text centered horizontally at vertical position y.
void Display::drawCenteredText(const char *text, int y)
{
    int16_t x1, y1;
    uint16_t w, h;
    // Get the bounding box of the text.
    display->getTextBounds(text, 0, y, &x1, &y1, &w, &h);
    int x = (SCREEN_WIDTH - w) / 2;
    display->setCursor(x, y);
    display->print(text);
}

void Display::updateDisplay()
{
    display->clearDisplay();

    // Draw ReaderInfo (using text size 1)
    display->setTextSize(1);
    display->setTextColor(SSD1306_WHITE);
    drawCenteredText(ReaderInfo, 0);

    // Draw Title (using text size 2 for emphasis)
    display->setTextSize(2);
    // The y-coordinate is chosen to leave room for the first line.
    drawCenteredText(Title, 16);

    // Draw Info (using text size 1)
    display->setTextSize(1);
    drawCenteredText(Info, 40);

    display->display();
}

void Display::updateByMQTT(const char *topic, byte *payload, unsigned int length)
{
    // Build expected MQTT topics for title, info, and stopping OTA.
    char topic_displayTitle[32];
    snprintf(topic_displayTitle, sizeof(topic_displayTitle), "fabreader/%05d/display/title", readerid);

    char topic_displayInfo[32];
    snprintf(topic_displayInfo, sizeof(topic_displayInfo), "fabreader/%05d/display/info", readerid);

    char topic_stopOTA[32];
    snprintf(topic_stopOTA, sizeof(topic_stopOTA), "fabreader/%05d/stopOTA", readerid);

    // Copy the payload into a null-terminated string.
    char *buffer = new char[length + 1];
    memcpy(buffer, payload, length);
    buffer[length] = '\0';

    // Update display based on the topic.
    if (strcmp(topic, topic_displayTitle) == 0)
    {
        writeTitle(buffer);
    }
    else if (strcmp(topic, topic_displayInfo) == 0)
    {
        writeInfo(buffer);
    }
    else if (strcmp(topic, topic_stopOTA) == 0)
    {
        clearReaderInfo();
    }
    delete[] buffer;
}
