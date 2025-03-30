#include "OTAProxy.h"
#include "NFC.h"
#include <PubSubClient.h>
#include <ArduinoLog.h>
#include "helpers.h"

// Maximum size of each chunk (leaving room for MQTT overhead)
#define MAX_CHUNK_SIZE 128

OTAProxy::OTAProxy(PubSubClient *mqttClient, NFC *nfc, int id)
{
    this->mqtt = mqttClient;
    this->nfc = nfc;
    this->id = id;
}

bool OTAProxy::hasActiveOTA()
{
    return activeOTA;
}

void OTAProxy::startOTA()
{
    if (!(nfc->hasCardSelected()))
    {
        return;
    }
    activeOTA = true;

    char topic[] = "fabreader/00000/startOTA";
    sprintf(topic, "fabreader/%05d/startOTA", id);

    String uid = nfc->getUID();
    mqtt->publish(topic, uid.c_str(), uid.length());

    Log.info(F("Start OTA"));
}

// Function to publish large payloads in chunks
bool OTAProxy::publishInChunks(const char *topic, const byte *payload, unsigned int length)
{
    // If payload is small enough, publish directly
    if (length <= MAX_CHUNK_SIZE)
    {
        return mqtt->publish(topic, payload, length);
    }

    // For larger payloads, we need to split into chunks
    Log.trace(F("Splitting large payload (%d bytes) into chunks"), length);

    // Calculate number of chunks needed
    int totalChunks = (length + MAX_CHUNK_SIZE - 1) / MAX_CHUNK_SIZE;

    // Prepare chunk topics
    char chunkTopic[64];

    // Send each chunk
    for (int chunkNum = 0; chunkNum < totalChunks; chunkNum++)
    {
        // Calculate this chunk's size and starting position
        int chunkStart = chunkNum * MAX_CHUNK_SIZE;
        int chunkSize = min(MAX_CHUNK_SIZE, (int)(length - chunkStart));

        // Prepare chunk topic with chunk information
        sprintf(chunkTopic, "fabreader/%05d/responseOTA/chunk/%d/%d", id, chunkNum + 1, totalChunks);

        Log.trace(F("Sending chunk %d/%d: %d bytes, topic: %s"),
                  chunkNum + 1, totalChunks, chunkSize, chunkTopic);

        // Small delay between chunks to prevent overwhelming the broker
        if (chunkNum > 0)
        {
            delay(20);
        }

        // Ensure we're still connected
        if (!mqtt->connected())
        {
            Log.error(F("MQTT disconnected during chunked publish!"));
            return false;
        }

        // Publish this chunk
        bool result = mqtt->publish(chunkTopic, &payload[chunkStart], chunkSize);

        if (!result)
        {
            Log.error(F("Failed to publish chunk %d/%d"), chunkNum + 1, totalChunks);
            return false;
        }
    }

    // All chunks sent successfully
    Log.trace(F("All %d chunks sent successfully"), totalChunks);

    // Send a final message to indicate completion
    sprintf(chunkTopic, "fabreader/%05d/responseOTA/complete", id);
    return mqtt->publish(chunkTopic, (byte *)&length, sizeof(length));
}

void OTAProxy::continueOTA(char *topic, byte *payload, unsigned int length)
{
    // Create topic strings with sufficient buffer sizes
    char topic_requestOTA[] = "fabreader/00000/requestOTA";
    sprintf(topic_requestOTA, "fabreader/%05d/requestOTA", id);

    char topic_responseOTA[] = "fabreader/00000/responseOTA";
    sprintf(topic_responseOTA, "fabreader/%05d/responseOTA", id);

    char topic_stopOTA[] = "fabreader/00000/stopOTA";
    sprintf(topic_stopOTA, "fabreader/%05d/stopOTA", id);

    if (!strcmp(topic, topic_requestOTA))
    {
        Log.trace("Request OTA");
        byte response[APDU_BUFFER_SIZE] = {0};
        byte response_len = APDU_BUFFER_SIZE;

        uint8_t status;

        Log.trace("Run Transceive");

        Log.trace("Payload length: %d", length);
        Log.trace("Payload: %X", payload);

        status = nfc->Transceive(payload, length, response, &response_len);
        Serial.printf("PICC_Tranceive: 0x%02x\n", status);

        if (status != 0x00)
        {
            cancelOTA();
            return;
        }

        Log.trace("Response length: %d", response_len);
        Log.trace("Response: %X", response);

        printbytes(response, response_len);

        // Get MQTT state before publishing
        int mqttStateBeforePublish = mqtt->state();
        Log.trace(F("MQTT state before publish: %d"), mqttStateBeforePublish);

        // Check if we're still connected to MQTT
        if (!mqtt->connected())
        {
            Log.error(F("MQTT disconnected before publish! Attempting to reconnect..."));
            // Attempt to reconnect once
            String clientId = "FabReader_";
            clientId += String(id);
            bool reconnected = mqtt->connect(clientId.c_str());
            if (!reconnected)
            {
                Log.error(F("MQTT reconnection failed with state: %d"), mqtt->state());
                cancelOTA();
                return;
            }
            else
            {
                Log.trace(F("MQTT reconnected successfully"));
            }
        }

        // Use the chunked publish method
        bool mqttResult = publishInChunks(topic_responseOTA, response, response_len);

        if (!mqttResult)
        {
            // Get current MQTT state for diagnostics
            int mqttStateAfterPublish = mqtt->state();
            Log.error(F("MQTT chunked publish failed! State: %d, Topic: %s, Payload size: %d bytes"),
                      mqttStateAfterPublish, topic_responseOTA, response_len);

            cancelOTA();
            return;
        }

        Log.trace("Response OTA successfully published in chunks");
    }
    else if (!strcmp(topic, topic_stopOTA))
    {
        Log.trace("Stop OTA");
        nfc->disconnectCard();
        activeOTA = false;
    }
}

void OTAProxy::cancelOTA()
{
    char topic_cancelOTA[] = "fabreader/00000/cancelOTA";
    sprintf(topic_cancelOTA, "fabreader/%05d/cancelOTA", id);

    String uid = nfc->getUID();
    mqtt->publish(topic_cancelOTA, uid.c_str(), uid.length());

    while (!(nfc->disconnectCard()))
    {
        yield();
    }
    activeOTA = false;

    Log.info("Cancel OTA");
}