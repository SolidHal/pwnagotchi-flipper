#pragma once

#include <furi.h>

/// Number of messages that can be stored in the queue at once
/// When the message queue fills up, we wrap around to the front
#define PWNAGOTCHI_PROTOCOL_MESSAGE_QUEUE_SIZE 15

/// Number of bytes reserved to protocol overhead, 1 for the PACKET_START and 1 for the PACKET_END
#define PWNAGOTCHI_PROTOCOL_OVERHEAD_SIZE 2

/// Max number of bytes in a single message, not including the PWNAGOTCHI_PROTOCOL_OVERHEAD_SIZE
#define PWNAGOTCHI_PROTOCOL_MAX_MESSAGE_SIZE 200

/// Start byte at beginning of transmission
#define PACKET_START 0x02
/// End byte at the end of transmission
#define PACKET_END 0x03

// Shared Commands
// Used for basic communication
#define CMD_SYN  0x16
#define CMD_ACK  0x06
#define CMD_NAK  0x15

// Flipper Zero Commands
// These commands can be sent from the pwnagotchi to the flipper
#define FLIPPER_CMD_UI_FACE        0x04
#define FLIPPER_CMD_UI_NAME        0x05
#define FLIPPER_CMD_UI_APS         0x07
#define FLIPPER_CMD_UI_UPTIME      0x08
#define FLIPPER_CMD_UI_FRIEND      0x09
#define FLIPPER_CMD_UI_MODE        0x0a
#define FLIPPER_CMD_UI_HANDSHAKES  0x0b
#define FLIPPER_CMD_UI_STATUS      0x0c
#define FLIPPER_CMD_UI_CHANNEL     0x0d

// Pwnagotchi commands
// These commands can be sent from the Flipper to the pwnagotchi
#define PWN_CMD_REBOOT      0x04
#define PWN_CMD_SHUTDOWN    0x05
#define PWN_CMD_MODE        0x07
#define PWN_CMD_UI_REFRESH  0x08
#define PWN_CMD_CLOCK_SET   0x09



typedef struct {
    /// Command code to operate on
    uint8_t code;

    /// Holds arguments sent folowing command code
    uint8_t arguments[PWNAGOTCHI_PROTOCOL_MAX_MESSAGE_SIZE - 1];
} PwnMessage;
