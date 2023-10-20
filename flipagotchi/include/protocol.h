#pragma once

#include <furi.h>

/// Number of bytes that can be stored in the queue at one time
#define PWNAGOTCHI_PROTOCOL_QUEUE_SIZE 5000

/// Max bytes of argument data
#define PWNAGOTCHI_PROTOCOL_ARGS_MAX 100

/// Start byte at beginning of transmission
#define PACKET_START 0x02
/// End byte at the end of transmission
#define PACKET_END 0x03

// Shared Commands
// Used for basic communication
#define CMD_SYN  0x16
#define CMD_ACK  0x06

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


typedef struct {
    /// Command code to operate on
    uint8_t code;

    /// Holds arguments sent folowing command code
    uint8_t arguments[PWNAGOTCHI_PROTOCOL_ARGS_MAX];
} PwnCommand;
