#include "protocol_queue.h"

ProtocolQueue* protocol_queue_alloc() {
    ProtocolQueue* instance = malloc(sizeof(ProtocolQueue));
    instance->message_queue = furi_message_queue_alloc(PWNAGOTCHI_PROTOCOL_MESSAGE_QUEUE_SIZE, sizeof(PwnMessage));

    instance->cur_message = malloc(sizeof(PwnMessage));

    instance->cur_message_len = 0;
    instance->cur_message_valid=false;

    return instance;
}

void protocol_queue_free(ProtocolQueue* instance) {
    FURI_LOG_W("PWN", "freeing furi message queue");
    furi_message_queue_free(instance->message_queue);
    FURI_LOG_W("PWN", "cur message");
    free(instance->cur_message);
    FURI_LOG_W("PWN", "our instance");
    free(instance);

    FURI_LOG_W("PWN", "protocol_queue_free setting our instance NULL");
    instance = NULL;
    FURI_LOG_W("PWN", "protocol_queue_free done");
}

bool protocol_queue_has_message(ProtocolQueue* instance) {
    if (furi_message_queue_get_count(instance->message_queue) > 0) {
        return true;
    }
    else {
        return false;
    }
}

void protocol_queue_push_byte(ProtocolQueue* instance, uint8_t byte) {
    if (PACKET_START == byte){
        // we have a new message
        instance->cur_message_len=0;
        instance->cur_message_valid=true;
        // don't copy packet control characters into the cur_message
        return;
    }

    if (!instance->cur_message_valid){
        // if we haven't seen a PACKET_START since the last PACKET_END
        // we are not currently receiving a valid packet, so we can
        // short circuit parsing here
        FURI_LOG_W("PWN", "cur_message is not valid! dropping byte");
        return;
    }

    if (PACKET_END == byte){
        // we have completed a message, put it on the message_queue
        // don't copy packet control characters into the cur_message

        if (furi_message_queue_get_space(instance->message_queue) <= 0){
            // no space left, just drop the message
            FURI_LOG_W("PWN", "message_queue is full! dropping message");
            return;
        }
        furi_message_queue_put(instance->message_queue, instance->cur_message, FuriWaitForever);
        // now that its copied into the queue, clear cur_message
        instance->cur_message_valid = false;
        memset(instance->cur_message, 0, instance->cur_message_len);
        return;
    }

    if (instance->cur_message_len + 1 > sizeof(PwnMessage)){
        FURI_LOG_W("PWN", "cur_message is full! dropping byte");
        return;
    }
    else {
        // we good to append the byte to the current message
        instance->cur_message[instance->cur_message_len] = byte;
        instance->cur_message_len++;
        return;
    }
}

void protocol_queue_wipe(ProtocolQueue* instance) {
    // Set everything to 0
    memset(instance->cur_message, 0, sizeof(PwnMessage));
    instance->cur_message_len = 0;
    furi_message_queue_reset(instance->message_queue);
}


bool protocol_queue_pop_message(ProtocolQueue* instance, PwnMessage* dest) {
    FURI_LOG_I("PWN", "trying to pop message");
    if (!protocol_queue_has_message(instance)) {
        return false;
    }

    FURI_LOG_I("PWN", "grabbing the message!");
    furi_message_queue_get(instance->message_queue, dest, FuriWaitForever);
    return true;
}