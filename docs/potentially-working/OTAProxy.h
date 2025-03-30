#ifndef OTAProxy_H
#define OTAProxy_H

#include <PubSubClient.h>
#include "NFC.h"
#define MSG_BUFFER_SIZE 50

class OTAProxy
{
private:
    bool activeOTA = false;
    PubSubClient *mqtt;
    NFC *nfc;
    int id;
    char msg[MSG_BUFFER_SIZE];

    // Helper method to publish large payloads in chunks
    bool publishInChunks(const char *topic, const byte *payload, unsigned int length);

public:
    OTAProxy(PubSubClient *mqtt, NFC *nfc, int id);
    bool hasActiveOTA();

    void startOTA();
    void continueOTA(char *topic, byte *payload, unsigned int length);
    void cancelOTA();
};

#endif
