#pragma once

#include <furi.h>
#include <core/message_queue.h>

#include "protocol.h"

typedef struct {

    FuriMessageQueue* message_queue;
    uint8_t* cur_message;
    size_t cur_message_len;
    bool cur_message_valid;

} ProtocolQueue;

/**
 * Allocates memory to store the queue
 *
 * @return Pointer to the newly created queue
 */
ProtocolQueue* protocol_queue_alloc();

/**
 * Destructs all memory that is stored at the pointer and sets pointer to null
 *
 * @param instance ProtocolQueue to operate on
 */
void protocol_queue_free(ProtocolQueue* instance);

/**
 * Decides if the queue has a full message available
 *
 * @param instance ProtocolQueue to check
 * @return If a message is available
 */
bool protocol_queue_has_message(ProtocolQueue* instance);

/**
 * Add a byte to the message queue
 *
 * @param instance ProtocolQueue to operate on
 * @param data Byte to add to the queue
 */
void protocol_queue_push_byte(ProtocolQueue* instance, uint8_t data);

/**
 * Wipes the entire message queue
 *
 * @param instance ProtocolQueue to wipe
 */
void protocol_queue_wipe(ProtocolQueue* instance);

/**
 * Loops through queue and ensures that all present bytes are valid according to the protocol,
 * if they are not it will wipe the queue
 *
 * @param instance ProtocolQueue to validate
 * @return If it was validated successfully. If it was wiped it will return false
 */
bool protocol_queue_validate(ProtocolQueue* instance);

/**
 * Pops a command off of the queue and saves into dest
 *
 * @note This will call validate_wipe and clear the queue if it detects bad instructions
 *
 * @param instance ProtocolQueue to pop from
 * @param dest Where to save the command to
 * @return If there was a command to pop and a valid command entered the dest
 */
bool protocol_queue_pop_message(ProtocolQueue* instance, PwnMessage* dest);