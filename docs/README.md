# MIFARE NFC Communication Protocol Documentation

This documentation provides a comprehensive guide to the MIFARE DESFire and MIFARE Ultralight C communication protocols as implemented in the nfcjlib library. The goal is to enable developers to create similar libraries in different languages, particularly Arduino/C++ and TypeScript.

## Contents

1. [Overview](overview.md) - Introduction to MIFARE NFC protocols
2. [DESFire EV1 Protocol](desfire.md) - Detailed documentation of the DESFire EV1 protocol
3. [Ultralight C Protocol](ultralight.md) - Detailed documentation of the Ultralight C protocol
4. [Authentication Protocols](authentication.md) - Mutual authentication protocols used by both card types
5. [Command Reference](commands.md) - Full reference of all commands and their payloads
6. [Implementation Examples](examples.md) - Code examples in Java, C++, and TypeScript
7. [Cryptography](crypto.md) - Cryptographic operations used in the protocols

This documentation is based on the nfcjlib Java library, which provides implementations for communicating with MIFARE DESFire EV1 and MIFARE Ultralight C smart cards using standard card readers via the javax.smartcardio API.
