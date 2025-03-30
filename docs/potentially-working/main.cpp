#include <Arduino.h>
#include <ArduinoLog.h>
#include "config.h"
#include <Pins.h>
#include <NFC.h>
#include <OTAProxy.h>
#include <helpers.h>
#include <Display.h>
#include <LedControl.h>

#include <WiFi.h>
#include <WebServer.h>

#include <WiFiClient.h>
#include <PubSubClient.h>

WiFiClient espClient;

PubSubClient *mqtt;
NFC *nfc;
OTAProxy *ota;
Display *display;

unsigned long otatimeout = 6000;
unsigned long lastotatime;

// Helper function to decode WiFi status codes
const char *getWifiStatusString(wl_status_t status)
{
    switch (status)
    {
    case WL_CONNECTED:
        return "Connected";
    case WL_NO_SHIELD:
        return "No WiFi shield";
    case WL_IDLE_STATUS:
        return "Idle";
    case WL_NO_SSID_AVAIL:
        return "No SSID available";
    case WL_SCAN_COMPLETED:
        return "Scan completed";
    case WL_CONNECT_FAILED:
        return "Connection failed";
    case WL_CONNECTION_LOST:
        return "Connection lost";
    case WL_DISCONNECTED:
        return "Disconnected";
    default:
        return "Unknown status";
    }
}

// WiFi event handler
void WiFiEventHandler(WiFiEvent_t event)
{
    Log.trace(F("[WiFi-event] event: %d"), event);

    switch (event)
    {
    case ARDUINO_EVENT_WIFI_STA_START:
        Log.trace(F("WiFi station mode started"));
        break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
        Log.trace(F("WiFi connected to AP"));
        LedControl::blinkSuccess(200); // Quick success blink when WiFi connects
        break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
        Log.error(F("WiFi lost connection"));
        LedControl::showError(); // Show error LED when WiFi disconnects
        break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
        Log.trace(F("WiFi got IP address"));
        LedControl::showSuccess(); // Show success LED when IP is obtained
        break;
    case ARDUINO_EVENT_WIFI_STA_LOST_IP:
        Log.error(F("WiFi lost IP address"));
        break;
    default:
        Log.trace(F("Other WiFi event: %d"), event);
        break;
    }
}

void setup_wifi()
{
    delay(10);

    // First verify that credentials are properly defined
    if (strlen(WLAN_SSID) == 0)
    {
        Log.error(F("WLAN_SSID is not defined or empty. Please check config.h"));
        display->writeTitle("WiFi Error");
        display->writeInfo("SSID not defined");
        delay(5000);
        return;
    }

    if (strlen(WLAN_PASS) == 0)
    {
        Log.warning(F("WLAN_PASS is empty. Attempting to connect to an open network"));
    }

    Log.trace(F("Connecting Wifi to SSID: %s"), WLAN_SSID);
    display->writeTitle("WiFi Setup");
    display->writeInfo(String("SSID: ") + WLAN_SSID);

    // Register WiFi event handler
    WiFi.onEvent(WiFiEventHandler);

    // Print current WiFi status before we begin
    wl_status_t currentStatus = WiFi.status();
    Log.trace(F("Initial WiFi status: %s (%d)"), getWifiStatusString(currentStatus), currentStatus);

    // Print WiFi information
    Log.trace(F("WiFi MAC address: %s"), WiFi.macAddress().c_str());

    // Disconnect from any previous connections
    Log.trace(F("Disconnecting from any previous WiFi connections"));
    WiFi.disconnect(true, true); // Disconnect, and clear credentials
    delay(1000);

    // Set WiFi mode to station (client)
    Log.trace(F("Setting WiFi to station mode"));
    WiFi.mode(WIFI_STA);

    // Set hostname for easier identification on network
    String hostname = "FabReader-";
    hostname += String(FABREADERID);
    WiFi.setHostname(hostname.c_str());
    Log.trace(F("Setting hostname to: %s"), hostname.c_str());

    // Scan for networks to verify the target SSID is available
    Log.trace(F("Scanning for WiFi networks..."));
    display->writeInfo("Scanning networks");

    int numNetworks = WiFi.scanNetworks();
    Log.trace(F("Found %d networks"), numNetworks);

    bool foundTargetNetwork = false;
    int targetNetworkRssi = -100; // Default low value
    int targetNetworkEncryption = -1;

    for (int i = 0; i < numNetworks; i++)
    {
        int encType = (int)WiFi.encryptionType(i);
        String encryptionType;

        // Decode encryption type
        switch (encType)
        {
        case WIFI_AUTH_OPEN:
            encryptionType = "Open";
            break;
        case WIFI_AUTH_WEP:
            encryptionType = "WEP";
            break;
        case WIFI_AUTH_WPA_PSK:
            encryptionType = "WPA_PSK";
            break;
        case WIFI_AUTH_WPA2_PSK:
            encryptionType = "WPA2_PSK";
            break;
        case WIFI_AUTH_WPA_WPA2_PSK:
            encryptionType = "WPA_WPA2_PSK";
            break;
        case WIFI_AUTH_WPA2_ENTERPRISE:
            encryptionType = "WPA2_ENTERPRISE";
            break;
        default:
            encryptionType = String(encType);
            break;
        }

        Log.trace(F("Network %d: SSID=%s, RSSI=%d dBm, Encryption=%s"),
                  i + 1, WiFi.SSID(i).c_str(), WiFi.RSSI(i), encryptionType.c_str());

        if (WiFi.SSID(i) == WLAN_SSID)
        {
            foundTargetNetwork = true;
            targetNetworkRssi = WiFi.RSSI(i);
            targetNetworkEncryption = encType;
            Log.trace(F("Target network '%s' found! Signal strength: %d dBm, Encryption: %s"),
                      WLAN_SSID, targetNetworkRssi, encryptionType.c_str());
        }
    }

    if (foundTargetNetwork)
    {
        Log.trace(F("Target network found with RSSI of %d dBm"), targetNetworkRssi);
        if (targetNetworkRssi < -80)
        {
            Log.warning(F("Signal strength is weak. This may cause connection issues"));
            display->writeInfo("Weak signal!");
            delay(1000);
        }
        display->writeInfo("Network found");
    }
    else
    {
        Log.error(F("Target network '%s' not found in scan results!"), WLAN_SSID);
        display->writeInfo("SSID not found!");
        delay(2000);
    }

    // Begin connection attempt
    Log.trace(F("Starting WiFi connection attempt"));
    WiFi.begin(WLAN_SSID, WLAN_PASS);
    Log.trace(F("WiFi credentials: SSID=%s, Password length=%d"), WLAN_SSID, strlen(WLAN_PASS));

    // With the password, let's check if it matches the encryption type
    if (targetNetworkEncryption != -1)
    {
        if (targetNetworkEncryption == WIFI_AUTH_OPEN && strlen(WLAN_PASS) > 0)
        {
            Log.warning(F("Network is open but password is provided"));
        }
        else if (targetNetworkEncryption != WIFI_AUTH_OPEN && strlen(WLAN_PASS) == 0)
        {
            Log.error(F("Network requires password but none is provided"));
            display->writeInfo("Need password!");
            delay(2000);
        }
    }

    unsigned long startAttemptTime = millis();
    int connectionAttempts = 0;
    int totalReconnectAttempts = 0;
    const int MAX_RECONNECT_ATTEMPTS = 3; // Maximum number of connection retry cycles

    while (WiFi.status() != WL_CONNECTED)
    {
        connectionAttempts++;
        wl_status_t wifiStatus = WiFi.status();

        // Check if we've been trying too long (30 seconds)
        if (millis() - startAttemptTime > 30000)
        {
            totalReconnectAttempts++;

            Log.error(F("WiFi connection timeout after %d attempts. Last status: %s (%d). Reconnect attempt %d of %d"),
                      connectionAttempts, getWifiStatusString(wifiStatus), wifiStatus,
                      totalReconnectAttempts, MAX_RECONNECT_ATTEMPTS);

            // If we've reached the maximum number of reconnect attempts, enter recovery mode
            if (totalReconnectAttempts >= MAX_RECONNECT_ATTEMPTS)
            {
                Log.error(F("Maximum reconnect attempts reached. Entering recovery mode"));
                LedControl::showError(); // Show error LED in recovery mode

                display->writeTitle("WiFi Failed");
                display->writeInfo("Check Config");

                // Print troubleshooting information
                Log.error(F("Troubleshooting steps:"));
                Log.error(F("1. Check SSID and password in config.h"));
                Log.error(F("2. Verify the router is on and in range"));
                Log.error(F("3. Restart the device after fixing issues"));

                // Sleep for a while before continuing
                delay(10000);

                // Return from function to let the main code continue
                // (it might decide to restart the device or take other actions)
                return;
            }

            // If we got WL_CONNECT_FAILED, there might be a password issue
            if (wifiStatus == WL_CONNECT_FAILED)
            {
                Log.error(F("Connection failed - possible password issue. Check WLAN_PASS in config.h"));
                display->writeInfo("Wrong password?");
                delay(2000);
            }
            // If we got WL_NO_SSID_AVAIL, the SSID might be out of range or misspelled
            else if (wifiStatus == WL_NO_SSID_AVAIL)
            {
                Log.error(F("SSID not available - check if network name is correct or in range"));
                display->writeInfo("SSID unavailable");
                delay(2000);
            }

            Log.trace(F("Restarting connection process..."));
            display->writeInfo("Retrying...");

            WiFi.disconnect(true);
            delay(1000);
            WiFi.begin(WLAN_SSID, WLAN_PASS);
            startAttemptTime = millis();
            connectionAttempts = 0;
            continue;
        }

        // Log status every 500ms
        Log.trace(F("WiFi status: %s (%d), attempt %d, time elapsed: %dms"),
                  getWifiStatusString(wifiStatus), wifiStatus,
                  connectionAttempts, millis() - startAttemptTime);

        // Update display with current status
        String statusMsg = String(getWifiStatusString(wifiStatus));
        statusMsg += " #";
        statusMsg += String(connectionAttempts);
        display->writeInfo(statusMsg);

        delay(500);
    }

    randomSeed(micros());

    // Successfully connected - log details
    IPAddress ip = WiFi.localIP();
    IPAddress gateway = WiFi.gatewayIP();
    IPAddress subnet = WiFi.subnetMask();
    IPAddress dns = WiFi.dnsIP();

    Log.trace(F("WiFi connected successfully after %d attempts and %d reconnect cycles"),
              connectionAttempts, totalReconnectAttempts);
    Log.trace(F("IP address: %d.%d.%d.%d"), ip[0], ip[1], ip[2], ip[3]);
    Log.trace(F("Gateway: %d.%d.%d.%d"), gateway[0], gateway[1], gateway[2], gateway[3]);
    Log.trace(F("Subnet mask: %d.%d.%d.%d"), subnet[0], subnet[1], subnet[2], subnet[3]);
    Log.trace(F("DNS server: %d.%d.%d.%d"), dns[0], dns[1], dns[2], dns[3]);
    Log.trace(F("Signal strength (RSSI): %d dBm"), WiFi.RSSI());

    String ipString = ip.toString();
    display->writeTitle("Connected");
    display->writeInfo(ipString);
    delay(2000);
}

void reconnect()
{
    while (!mqtt->connected())
    {
        String clientId = "FabReader_";
        clientId += String(FABREADERID);

        Log.trace(F("Connecting MQTT ..."));
        bool connected = false;
        if (strcmp(MQTT_USERNAME, "") == 0)
        {
            connected = mqtt->connect(clientId.c_str());
        }
        else
        {
            connected = mqtt->connect(clientId.c_str(), MQTT_USERNAME, MQTT_PASSWORD);
        }

        if (connected)
        {
            Log.trace(F("MQTT connected"));
            LedControl::showSuccess(); // Show success LED when MQTT connects
            char id[6] = "00000";
            sprintf(id, "%05d", FABREADERID);
            mqtt->publish("fabreader", id);

            char topic_requestOTA[] = "fabreader/00000/requestOTA";
            sprintf(topic_requestOTA, "fabreader/%05d/requestOTA", FABREADERID);
            mqtt->subscribe(topic_requestOTA);

            char topic_stopOTA[] = "fabreader/00000/stopOTA";
            sprintf(topic_stopOTA, "fabreader/%05d/stopOTA", FABREADERID);
            mqtt->subscribe(topic_stopOTA);

            // Subscribe to display topics
            char topic_displayTitle[] = "fabreader/00000/display/title";
            sprintf(topic_displayTitle, "fabreader/%05d/display/title", FABREADERID);
            mqtt->subscribe(topic_displayTitle);

            char topic_displayInfo[] = "fabreader/00000/display/info";
            sprintf(topic_displayInfo, "fabreader/%05d/display/info", FABREADERID);
            mqtt->subscribe(topic_displayInfo);

            display->writeTitle("Connected");
            display->writeInfo("");
        }
        else
        {
            display->writeTitle("Reconnect");
            display->writeInfo("MQTT");
            LedControl::showError(); // Show error LED when MQTT connection fails

            Log.error(F("MQTT connection failed, rc=%d, try again in 5 seconds"), mqtt->state());
            delay(5000);
        }
    }
}

void callback(char *topic, byte *payload, unsigned int length)
{
    Log.trace(F("Receive Message: %s"), topic);
    if (ota->hasActiveOTA())
    {
        ota->continueOTA(topic, payload, length);
    }
    else
    {
        // Only handle display updates and OTA start when not already in OTA mode
        display->updateByMQTT(topic, payload, length);
    }
}

void printSuffix(Print *_logOutput, int logLevel)
{
    _logOutput->print("\n");
}

// Check for configuration issues
bool hasConfigurationIssues()
{
    bool hasIssues = false;

    // Check WiFi configuration
    if (strlen(WLAN_SSID) == 0)
    {
        Log.error(F("WLAN_SSID is not defined or empty in config.h"));
        hasIssues = true;
    }

    // Check MQTT configuration
    if (strlen(MQTT_BROKER) == 0)
    {
        Log.error(F("MQTT_BROKER is not defined or empty in config.h"));
        hasIssues = true;
    }

    // Check for invalid characters in configuration strings
    if (strchr(WLAN_SSID, '%'))
    {
        Log.error(F("WLAN_SSID contains invalid '%' character"));
        hasIssues = true;
    }

    if (strchr(WLAN_PASS, '%'))
    {
        Log.error(F("WLAN_PASS contains invalid '%' character"));
        hasIssues = true;
    }

    if (strchr(MQTT_BROKER, '%'))
    {
        Log.error(F("MQTT_BROKER contains invalid '%' character"));
        hasIssues = true;
    }

    // Check reader ID
    if (FABREADERID <= 0)
    {
        Log.error(F("FABREADERID must be a positive number"));
        hasIssues = true;
    }

    return hasIssues;
}

void setup()
{
    Serial.begin(115200);

    delay(3000);

    Log.begin(LOG_LEVEL_VERBOSE, &Serial); // Initialize ArduinoLog
    Log.setSuffix(printSuffix);

    // Optional: add a timestamp to each log entry.
    Log.begin(LOG_LEVEL_VERBOSE, &Serial, true);

    Log.trace(F("\n\n\n"));
    Log.trace(F("Booting ..."));

    display = new Display(PIN_SDA, PIN_SCL, FABREADERID);
    display->clearReaderInfo();

    // Check for configuration issues before proceeding
    if (hasConfigurationIssues())
    {
        Log.error(F("Configuration issues detected! Please check config.h"));
        LedControl::showError(); // Show error LED for configuration issues
        display->writeTitle("Config Error");
        display->writeInfo("Check Serial Log");

        // Flash the display a few times to alert the user
        for (int i = 0; i < 5; i++)
        {
            display->writeTitle("");
            delay(300);
            display->writeTitle("Config Error");
            delay(300);
        }
    }

    display->writeInfo("Start NFC ...");
    Log.trace(F("Connecting NFC ..."));

    nfc = new NFC(PIN_SDA, PIN_SCL);

    Log.trace(F("NFC connected"));

    display->writeInfo("Start WIFI ...");
    setup_wifi();

    // Check if WiFi connected before proceeding with MQTT
    if (WiFi.status() != WL_CONNECTED)
    {
        Log.error(F("WiFi connection failed. Cannot proceed with MQTT setup."));
        display->writeTitle("WiFi Failed");
        display->writeInfo("Restart Device");

        // Keep the device running, but don't proceed with MQTT setup
        while (1)
        {
            // Check if WiFi miraculously connects
            if (WiFi.status() == WL_CONNECTED)
            {
                Log.trace(F("WiFi connected after delay, continuing..."));
                display->writeTitle("WiFi Connected");
                display->writeInfo("Continuing...");
                delay(2000);
                break;
            }
        }
    }

    display->writeInfo("Start MQTT ...");
    mqtt = new PubSubClient(espClient);
    mqtt->setServer(MQTT_BROKER, 1883);
    mqtt->setCallback(callback);

    // Log MQTT configuration
    Log.trace(F("MQTT broker: %s"), MQTT_BROKER);
    Log.trace(F("MQTT max packet size: %d bytes"), MQTT_MAX_PACKET_SIZE);
    Log.trace(F("MQTT client ID: FabReader_%d"), FABREADERID);

    display->writeInfo("Start OTA ...");
    ota = new OTAProxy(mqtt, nfc, FABREADERID);

    LedControl::begin();
}

void loop()
{
    if (!mqtt->connected())
    {
        reconnect();
    }

    mqtt->loop();

    if (!ota->hasActiveOTA())
    {
        if (nfc->checkforCard())
        {
            Log.trace(F("Card detected"));
            if (nfc->connecttoCard())
            {
                Log.trace(F("Card connected"));
                LedControl::blinkSuccess(); // Blink success LED when card is read successfully
                lastotatime = millis();

                display->createReaderInfo("Run OTA");
                ota->startOTA();

                // Report card info to MQTT
                if (mqtt->connected())
                {
                    String cardInfo = "{\"uid\":\"" + nfc->getUID() + "\",\"type\":\"" + nfc->getCardType() + "\"}";
                    String topic = String(MQTT_TOPIC_PREFIX) + "/" + String(FABREADERID) + "/card";
                    mqtt->publish(topic.c_str(), cardInfo.c_str());
                }
            }
            else
            {
                // Card detection succeeded but connection failed
                String errorMsg = nfc->getLastErrorMessage();
                CardErrorCode errorCode = nfc->getLastErrorCode();

                LedControl::blinkError(); // Blink error LED for card connection failure

                Log.error(F("Card connection failed: %s (Code: %d)"), errorMsg.c_str(), errorCode);

                // Send error to display
                String displayError = "Card Error: " + errorMsg;
                display->createReaderInfo(displayError.c_str());

                // Send error to MQTT for enrollment API UI
                if (mqtt->connected())
                {
                    String errorInfo = "{\"error\":\"" + errorMsg +
                                       "\",\"code\":" + String(errorCode) +
                                       ",\"uid\":\"" + nfc->getUID() +
                                       "\",\"type\":\"" + nfc->getCardType() + "\"}";
                    String topic = String(MQTT_TOPIC_PREFIX) + "/" + String(FABREADERID) + "/error";
                    mqtt->publish(topic.c_str(), errorInfo.c_str());
                }

                // Brief delay so the error is visible
                delay(2000);

                display->createReaderInfo("Retry Card");
            }
        }
        else if (nfc->getLastErrorCode() != CARD_ERROR_NO_CARD)
        {
            // Log and display any error that isn't just "no card found"
            String errorMsg = nfc->getLastErrorMessage();
            CardErrorCode errorCode = nfc->getLastErrorCode();

            LedControl::blinkError(); // Blink error LED for card detection error

            Log.error(F("Card detection error: %s (Code: %d)"), errorMsg.c_str(), errorCode);

            // Only display and send significant errors, not just "no card found"
            String displayError = "Card Error: " + errorMsg;
            display->createReaderInfo(displayError.c_str());

            // Send error to MQTT for enrollment API UI
            if (mqtt->connected())
            {
                String errorInfo = "{\"error\":\"" + errorMsg +
                                   "\",\"code\":" + String(errorCode) + "\"}";
                String topic = String(MQTT_TOPIC_PREFIX) + "/" + String(FABREADERID) + "/error";
                mqtt->publish(topic.c_str(), errorInfo.c_str());
            }

            // Brief delay so the error is visible
            delay(2000);
            display->clearReaderInfo();
        }
    }
    if (ota->hasActiveOTA())
    {
        if (millis() - lastotatime > otatimeout)
        {
            ota->cancelOTA();
            display->clearReaderInfo();
        }
    }
}