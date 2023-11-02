#include "flipagotchi_uart.h"

struct FlipagotchiUart {
    FuriThread* uart_worker_thread;
    FuriThread* cmd_worker_thread;
    FuriStreamBuffer* rx_stream;
    ProtocolQueue* queue;
    bool synack_complete;
};

const NotificationSequence sequence_notification = {
    &message_display_backlight_on,
    &message_green_255,
    &message_delay_10,
    NULL,
};

static void flipagotchi_send_syn() {
    uint8_t msg[] = {PACKET_START, CMD_SYN, PACKET_END};
    furi_hal_uart_tx(PWNAGOTCHI_UART_CHANNEL, msg, sizeof(msg));
}

static void flipagotchi_send_ack(const uint8_t received_cmd) {
    uint8_t ack_msg[] = {PACKET_START, CMD_ACK, PACKET_END};
    FURI_LOG_I("PWN", "valid command %02X received, replying with ACK", received_cmd);
    furi_hal_uart_tx(PWNAGOTCHI_UART_CHANNEL, ack_msg, sizeof(ack_msg));
}

static void flipagotchi_send_nak(const uint8_t received_cmd) {
    uint8_t nak_msg[] = {PACKET_START, CMD_NAK, PACKET_END};
    FURI_LOG_I("PWN", "invalid command %02X received, replying with NAK", received_cmd);
    furi_hal_uart_tx(PWNAGOTCHI_UART_CHANNEL, nak_msg, sizeof(nak_msg));
}

static void flipagotchi_send_ui_refresh() {
    uint8_t msg[] = {PACKET_START, PWN_CMD_UI_REFRESH, PACKET_END};
    FURI_LOG_I("PWN", "sending ui refresh cmd");
    furi_hal_uart_tx(PWNAGOTCHI_UART_CHANNEL, msg, sizeof(msg));
}

void flipagotchi_uart_init(FlipagotchiUart* ctx) {
    ctx->synack_complete = false;
    flipagotchi_send_syn();
}


static bool flipagotchi_exec_cmd(PwnagotchiModel* pwn_model, FlipagotchiUart* flipagotchi_uart) {
    if (protocol_queue_has_message(flipagotchi_uart->queue)) {
        PwnMessage message;
        protocol_queue_pop_message(flipagotchi_uart->queue, &message);
        FURI_LOG_I("PWN", "Has message (code: %02X), processing...", message.code);
        //TODO REMOVE, only here to debug crashes
        flipagotchi_send_ack(message.code);
        return false;

        // See what the message wants
        switch (message.code) {

            // Process ACK
            case CMD_ACK: {
              FURI_LOG_I("PWN", "received ACK");
              //TODO NOT IMPLEMENTED
              //TODO, add logic to ensure every message we send receives an ACK

              if (!flipagotchi_uart->synack_complete){
                  flipagotchi_uart->synack_complete = true;
                  // this ack is likely an ack to our last syn
                  // assume that is true, and mark synack complete

                  // either we start first, or the pwnagotchi does
                  // if the pwn does, then we won't have up to date ui elements
                  // send a ui refresh.
                  // if we get a reply, pwn started first
                  // if we don't get a reply, just move on. we probably started first
                  // pwn will update us when it gets going
                  FURI_LOG_I("PWN", "sending ui refresh");
                  flipagotchi_send_ui_refresh();
              }
              break;
            }

            // Process SYN
            case CMD_SYN: {
              flipagotchi_send_ack(message.code);
              break;
            }

            // Process Face
            case FLIPPER_CMD_UI_FACE: {
                // send ack before handling to avoid stalling the pwnagotchi
                flipagotchi_send_ack(message.code);

                pwn_model->face = message.arguments[0];

                return true;
            }

            // Process Name
            case FLIPPER_CMD_UI_NAME: {
                // send ack before handling to avoid stalling the pwnagotchi
                flipagotchi_send_ack(message.code);

                // Write over hostname with nothing
                strncpy(pwn_model->hostname, "", PWNAGOTCHI_MAX_HOSTNAME_LEN);

                for (size_t i = 0; i < PWNAGOTCHI_MAX_HOSTNAME_LEN; i++) {
                    // Break if we hit the end of the name
                    if (message.arguments[i] == 0x00) {
                        break;
                    }

                    pwn_model->hostname[i] = message.arguments[i];
                }
                return true;
            }

            // Process channel
            case FLIPPER_CMD_UI_CHANNEL: {
                // send ack before handling to avoid stalling the pwnagotchi
                flipagotchi_send_ack(message.code);

                // Write over channel with nothing
                strncpy(pwn_model->channel, "", PWNAGOTCHI_MAX_CHANNEL_LEN);

                for (size_t i = 0; i < PWNAGOTCHI_MAX_CHANNEL_LEN; i++) {
                    // Break if we hit the end of the name
                    if (message.arguments[i] == 0x00) {
                        break;
                    }

                    pwn_model->channel[i] = message.arguments[i];
                }
                return true;
            }

            // Process APS (Access Points)
            case FLIPPER_CMD_UI_APS: {
                // send ack before handling to avoid stalling the pwnagotchi
                flipagotchi_send_ack(message.code);

                // Write over APS with nothing
                strncpy(pwn_model->apStat, "", PWNAGOTCHI_MAX_APS_LEN);

                for (size_t i = 0; i < PWNAGOTCHI_MAX_APS_LEN; i++) {
                    // Break if we hit the end of the name
                    if (message.arguments[i] == 0x00) {
                        break;
                    }
                    pwn_model->apStat[i] = message.arguments[i];
                }
                return true;
            }

            // Process uptime
            case FLIPPER_CMD_UI_UPTIME: {
                // send ack before handling to avoid stalling the pwnagotchi
                flipagotchi_send_ack(message.code);

                // Write over uptime with nothing
                strncpy(pwn_model->uptime, "", PWNAGOTCHI_MAX_UPTIME_LEN);

                for (size_t i = 0; i < PWNAGOTCHI_MAX_UPTIME_LEN; i++) {
                    // Break if we hit the end of the name
                    if (message.arguments[i] == 0x00) {
                        break;
                    }
                    pwn_model->uptime[i] = message.arguments[i];
                }
                return true;
            }

            // Process friend
            case FLIPPER_CMD_UI_FRIEND: {
                // send ack before handling to avoid stalling the pwnagotchi
                flipagotchi_send_ack(message.code);

                // Friend not implemented yet,
                // nothing to update
                return false;
            }

            // Process mode
            case FLIPPER_CMD_UI_MODE: {
                // send ack before handling to avoid stalling the pwnagotchi
                flipagotchi_send_ack(message.code);

                enum PwnagotchiMode mode;

                switch (message.arguments[0]) {
                    case 0x04:
                        mode = PwnMode_Manual;
                        break;
                    case 0x05:
                        mode = PwnMode_Auto;
                        break;
                    case 0x06:
                        mode = PwnMode_Ai;
                        break;
                    default:
                        mode = PwnMode_Manual;
                        break;
                }
                pwn_model->mode = mode;

                return true;
            }

            // Process Handshakes
            case FLIPPER_CMD_UI_HANDSHAKES: {
                // send ack before handling to avoid stalling the pwnagotchi
                flipagotchi_send_ack(message.code);

                // Write over handshakes with nothing
                strncpy(pwn_model->handshakes, "", PWNAGOTCHI_MAX_HANDSHAKES_LEN);

                for (size_t i = 0; i < PWNAGOTCHI_MAX_HANDSHAKES_LEN; i++) {
                    // Break if we hit the end of the name
                    if (message.arguments[i] == 0x00) {
                        break;
                    }
                    pwn_model->handshakes[i] = message.arguments[i];
                }
                return true;
            }

            // Process status
            case FLIPPER_CMD_UI_STATUS: {
                // send ack before handling to avoid stalling the pwnagotchi
                flipagotchi_send_ack(message.code);

                // Write over the status with nothing
                strncpy(pwn_model->status, "", PWNAGOTCHI_MAX_STATUS_LEN);

                for (size_t i = 0; i < PWNAGOTCHI_MAX_STATUS_LEN; i++) {
                    if (message.arguments[i] == 0x00) {
                        break;
                    }
                    pwn_model->status[i] = message.arguments[i];
                }
                FURI_LOG_I("PWN", "rec status: %s", pwn_model->status);
                return true;
            }
            default: {
                // didn't match any of the known FLIPPER_CMDs
                // reply with a NAK
                flipagotchi_send_nak(message.code);
                return false;
            }
        }
    }

    return false;
}

static void flipagotchi_on_irq_cb(UartIrqEvent ev, uint8_t data, void* context) {
    furi_assert(context);
    // loads the rx_stream with each byte we receive
    FlipagotchiUart* flipagotchi_uart = context;

    if(ev == UartIrqEventRXNE) {
        // put bytes onto the rx_stream, one by one as they come in


        if (data < 65){
          FURI_LOG_I("PWN", "pushing %02X to queue", data);
        }
        else{
          FURI_LOG_I("PWN", "pushing %c to queue", data);
        }

        furi_stream_buffer_send(flipagotchi_uart->rx_stream, &data, 1, 0);
        furi_thread_flags_set(furi_thread_get_id(flipagotchi_uart->uart_worker_thread), WorkerEventRx);
    }
}

static int32_t flipagotchi_uart_worker(void* context) {
    furi_assert(context);
    FlipagotchiUart* flipagotchi_uart = context;

    FURI_LOG_I("PWN", "setup uart");
    // setup uart
    if(PWNAGOTCHI_UART_CHANNEL == FuriHalUartIdUSART1) {
      // when using the main uart, aka the ones labeled on the flippper, we
      // MUST DISABLE CONSOLE OR ELSE IT DIRTIES OUR UART!
      // this is annoying for debugging :(
      furi_hal_console_disable();
    } else if(PWNAGOTCHI_UART_CHANNEL == FuriHalUartIdLPUART1) {
      furi_hal_uart_init(PWNAGOTCHI_UART_CHANNEL, PWNAGOTCHI_UART_BAUD);
    }
    furi_hal_uart_set_br(PWNAGOTCHI_UART_CHANNEL, PWNAGOTCHI_UART_BAUD);
    furi_hal_uart_set_irq_cb(PWNAGOTCHI_UART_CHANNEL, flipagotchi_on_irq_cb, flipagotchi_uart);

    uint8_t rx_buf[RX_BUF_SIZE + 1];
    FURI_LOG_I("PWN", "uart worker, staring loop");
    while(true) {
        uint32_t events =
            furi_thread_flags_wait(WORKER_EVENTS_MASK, FuriFlagWaitAny, FuriWaitForever);
        furi_check((events & FuriFlagError) == 0);

        if(events & WorkerEventStop) {
            FURI_LOG_I("PWN", "uart_worker received stop");
            break;
        }
        else if(events & WorkerEventRx) {
            size_t length = furi_stream_buffer_receive(flipagotchi_uart->rx_stream, rx_buf, RX_BUF_SIZE, 0);
            if(length > 0) {
                for(size_t i = 0; i < length; i++) {
                    protocol_queue_push_byte(flipagotchi_uart->queue, rx_buf[i]);
                }
                furi_thread_flags_set(furi_thread_get_id(flipagotchi_uart->cmd_worker_thread), WorkerEventRx);
            }
        }
    }


    FURI_LOG_I("PWN", "free uart");
    // free uart
    if(PWNAGOTCHI_UART_CHANNEL == FuriHalUartIdUSART1){
      furi_hal_console_enable();
    }
    else if(PWNAGOTCHI_UART_CHANNEL == FuriHalUartIdLPUART1){
      furi_hal_uart_deinit(PWNAGOTCHI_UART_CHANNEL);
    }
    furi_hal_uart_set_irq_cb(PWNAGOTCHI_UART_CHANNEL, NULL, NULL);

    return 0;
}

static int32_t flipagotchi_cmd_worker(void* context){
    furi_assert(context);
    FlipagotchiUart* flipagotchi_uart = context;

    FURI_LOG_I("PWN", "alloc rx stream buffert");
    // alloc incoming stream for uart thread
    flipagotchi_uart->rx_stream = furi_stream_buffer_alloc(RX_BUF_SIZE, 1);

    FURI_LOG_I("PWN", "alloc uart thread");
    // uart thread
    flipagotchi_uart->uart_worker_thread = furi_thread_alloc();
    furi_thread_set_stack_size(flipagotchi_uart->uart_worker_thread, 1024);
    furi_thread_set_context(flipagotchi_uart->uart_worker_thread, flipagotchi_uart);
    furi_thread_set_callback(flipagotchi_uart->uart_worker_thread, flipagotchi_uart_worker);
    furi_thread_start(flipagotchi_uart->uart_worker_thread);

    flipagotchi_uart_init(flipagotchi_uart);

    FURI_LOG_I("PWN", "cmd_worker, starting loop");
    while(true) {
        uint32_t events =
            furi_thread_flags_wait(WORKER_EVENTS_MASK, FuriFlagWaitAny, FuriWaitForever);
        furi_check((events & FuriFlagError) == 0);

        if(events & WorkerEventStop) {
          FURI_LOG_I("PWN", "cmd_worker received stop");
          break;
        }
        else if(events & WorkerEventRx) {
            /* with_view_model( */
            /*         flipagotchi_uart->view, */
            /*         PwnDumpModel* model, */
            /*         { */
            /*             update = flipagotchi_exec_cmd(model, flipagotchi_uart); */
            /*         }, */
            /*         update); */
            flipagotchi_exec_cmd(NULL, flipagotchi_uart);

            // light up the screen and blink the led
            /* notification_message(flipagotchi_uart->notification, &sequence_notification); */
            // with_view_model(
            // flipagotchi_uart->view, PwnDumpModel * model, { UNUSED(model); }, true);
        }

    }


    FURI_LOG_I("PWN", "free uart worker");
    furi_thread_flags_set(furi_thread_get_id(flipagotchi_uart->uart_worker_thread), WorkerEventStop);
    FURI_LOG_I("PWN", "free uart worker: joining");
    furi_thread_join(flipagotchi_uart->uart_worker_thread);
    FURI_LOG_I("PWN", "free uart worker: freeing");
    furi_thread_free(flipagotchi_uart->uart_worker_thread);
    FURI_LOG_I("PWN", "free uart worker: setting NULL");
    flipagotchi_uart->uart_worker_thread = NULL;

    FURI_LOG_I("PWN", "free stream buffer");
    furi_stream_buffer_free(flipagotchi_uart->rx_stream);

    return 0;
}

/* static void flipagotchi_enable_power() { */
/*   uint8_t attempts = 0; */
/*   while(!furi_hal_power_is_otg_enabled() && attempts++ < 5) { */
/*     furi_hal_power_enable_otg(); */
/*     furi_delay_ms(10); */
/*     } */
/* } */

/* static void flipagotchi_disable_power() { */
/*   if(furi_hal_power_is_otg_enabled()) { */
/*     furi_hal_power_disable_otg(); */
/*   } */
/* } */

FlipagotchiUart* flipagotchi_uart_alloc(){
    FlipagotchiUart* flipagotchi_uart = malloc(sizeof(FlipagotchiUart));

    flipagotchi_uart->synack_complete = false;
    FURI_LOG_I("PWN", "alloc queue");
    // Queue
    flipagotchi_uart->queue = protocol_queue_alloc();

    FURI_LOG_I("PWN", "alloc threads cmd parser thread");
    // command parser thread
    flipagotchi_uart->cmd_worker_thread = furi_thread_alloc();
    furi_thread_set_stack_size(flipagotchi_uart->cmd_worker_thread, 1024);
    furi_thread_set_context(flipagotchi_uart->cmd_worker_thread, flipagotchi_uart);
    furi_thread_set_callback(flipagotchi_uart->cmd_worker_thread, flipagotchi_cmd_worker);
    furi_thread_start(flipagotchi_uart->cmd_worker_thread);

    return flipagotchi_uart;
}

void flipagotchi_uart_free(FlipagotchiUart* flipagotchi_uart){
    FURI_LOG_I("PWN", "free cmd worker");
    // free workers
    furi_thread_flags_set(
        furi_thread_get_id(flipagotchi_uart->cmd_worker_thread), WorkerEventStop);
    furi_thread_join(flipagotchi_uart->cmd_worker_thread);
    furi_thread_free(flipagotchi_uart->cmd_worker_thread);
    flipagotchi_uart->cmd_worker_thread = NULL;

    FURI_LOG_I("PWN", "free queue");
    // Free Queue
    protocol_queue_free(flipagotchi_uart->queue);
}
