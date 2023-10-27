#include <furi.h>
#include <gui/gui.h>
#include <notification/notification.h>
#include <notification/notification_messages.h>
#include <gui/elements.h>
#include <furi_hal_uart.h>
#include <furi_hal_console.h>
#include <gui/view_dispatcher.h>
#include <gui/view_port.h>
#include <gui/modules/dialog_ex.h>

#include "../include/pwnagotchi.h"
#include "../include/protocol.h"
#include "../include/protocol_queue.h"

#define LINES_ON_SCREEN 6
#define COLUMNS_ON_SCREEN 21

#define RX_BUF_SIZE 2048

typedef struct PwnDumpModel PwnDumpModel;

typedef struct {
    Gui* gui;
    ViewPort* view_port;
    NotificationApp* notification;
    FuriThread* uart_worker_thread;
    FuriThread* cmd_worker_thread;
    FuriStreamBuffer* rx_stream;
    FuriMessageQueue* event_queue;
    ProtocolQueue *queue;
    bool synack_complete;

} FlipagotchiApp;

typedef struct {
    FuriString* text;
} ListElement;

// TODO keep byte in model indicating *what* needs to be updated
struct PwnDumpModel {
    Pwnagotchi* pwn;
};

typedef enum {
    WorkerEventReserved = (1 << 0), // Reserved for StreamBuffer internal event
    WorkerEventStop = (1 << 1),
    WorkerEventRx = (1 << 2),
} WorkerEventFlags;

#define WORKER_EVENTS_MASK (WorkerEventStop | WorkerEventRx)

const NotificationSequence sequence_notification = {
    &message_display_backlight_on,
    &message_green_255,
    &message_delay_10,
    NULL,
};

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

static void flipagotchi_uart_init(FlipagotchiApp* app) {
    app->synack_complete = false;
    flipagotchi_send_syn();
}

static bool flipagotchi_exec_cmd(PwnDumpModel* model, FlipagotchiApp* app) {
    if (protocol_queue_has_message(app->queue)) {
        PwnMessage message;
        protocol_queue_pop_message(app->queue, &message);
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

              if (!app->synack_complete){
                  app->synack_complete = true;
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

                model->pwn->face = message.arguments[0];

                return true;
            }

            // Process Name
            case FLIPPER_CMD_UI_NAME: {
                // send ack before handling to avoid stalling the pwnagotchi
                flipagotchi_send_ack(message.code);

                // Write over hostname with nothing
                strncpy(model->pwn->hostname, "", PWNAGOTCHI_MAX_HOSTNAME_LEN);

                for (size_t i = 0; i < PWNAGOTCHI_MAX_HOSTNAME_LEN; i++) {
                    // Break if we hit the end of the name
                    if (message.arguments[i] == 0x00) {
                        break;
                    }

                    model->pwn->hostname[i] = message.arguments[i];
                }
                return true;
            }

            // Process channel
            case FLIPPER_CMD_UI_CHANNEL: {
                // send ack before handling to avoid stalling the pwnagotchi
                flipagotchi_send_ack(message.code);

                // Write over channel with nothing
                strncpy(model->pwn->channel, "", PWNAGOTCHI_MAX_CHANNEL_LEN);

                for (size_t i = 0; i < PWNAGOTCHI_MAX_CHANNEL_LEN; i++) {
                    // Break if we hit the end of the name
                    if (message.arguments[i] == 0x00) {
                        break;
                    }

                    model->pwn->channel[i] = message.arguments[i];
                }
                return true;
            }

            // Process APS (Access Points)
            case FLIPPER_CMD_UI_APS: {
                // send ack before handling to avoid stalling the pwnagotchi
                flipagotchi_send_ack(message.code);

                // Write over APS with nothing
                strncpy(model->pwn->apStat, "", PWNAGOTCHI_MAX_APS_LEN);

                for (size_t i = 0; i < PWNAGOTCHI_MAX_APS_LEN; i++) {
                    // Break if we hit the end of the name
                    if (message.arguments[i] == 0x00) {
                        break;
                    }
                    model->pwn->apStat[i] = message.arguments[i];
                }
                return true;
            }

            // Process uptime
            case FLIPPER_CMD_UI_UPTIME: {
                // send ack before handling to avoid stalling the pwnagotchi
                flipagotchi_send_ack(message.code);

                // Write over uptime with nothing
                strncpy(model->pwn->uptime, "", PWNAGOTCHI_MAX_UPTIME_LEN);

                for (size_t i = 0; i < PWNAGOTCHI_MAX_UPTIME_LEN; i++) {
                    // Break if we hit the end of the name
                    if (message.arguments[i] == 0x00) {
                        break;
                    }
                    model->pwn->uptime[i] = message.arguments[i];
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
                model->pwn->mode = mode;

                return true;
            }

            // Process Handshakes
            case FLIPPER_CMD_UI_HANDSHAKES: {
                // send ack before handling to avoid stalling the pwnagotchi
                flipagotchi_send_ack(message.code);

                // Write over handshakes with nothing
                strncpy(model->pwn->handshakes, "", PWNAGOTCHI_MAX_HANDSHAKES_LEN);

                for (size_t i = 0; i < PWNAGOTCHI_MAX_HANDSHAKES_LEN; i++) {
                    // Break if we hit the end of the name
                    if (message.arguments[i] == 0x00) {
                        break;
                    }
                    model->pwn->handshakes[i] = message.arguments[i];
                }
                return true;
            }

            // Process status
            case FLIPPER_CMD_UI_STATUS: {
                // send ack before handling to avoid stalling the pwnagotchi
                flipagotchi_send_ack(message.code);

                // Write over the status with nothing
                strncpy(model->pwn->status, "", PWNAGOTCHI_MAX_STATUS_LEN);

                for (size_t i = 0; i < PWNAGOTCHI_MAX_STATUS_LEN; i++) {
                    if (message.arguments[i] == 0x00) {
                        break;
                    }
                    model->pwn->status[i] = message.arguments[i];
                }
                FURI_LOG_I("PWN", "rec status: %s", model->pwn->status);
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

/* static void flipagotchi_view_draw_callback(Canvas* canvas, void* _model) { */
/*     PwnDumpModel* model = _model; */

/*     pwnagotchi_draw_all(model->pwn, canvas); */
/* } */

static void flipagotchi_input_callback(InputEvent* event, void* context) {
    FlipagotchiApp* app = context;
    furi_message_queue_put(app->event_queue, event, FuriWaitForever);
}

/* static uint32_t flipagotchi_exit(void* context) { */
/*     UNUSED(context); */
/*     return VIEW_NONE; */
/* } */

static void flipagotchi_on_irq_cb(UartIrqEvent ev, uint8_t data, void* context) {
    furi_assert(context);
    // loads the rx_stream with each byte we receive
    FlipagotchiApp* app = context;

    if(ev == UartIrqEventRXNE) {
        // put bytes onto the rx_stream, one by one as they come in


        if (data < 65){
          FURI_LOG_I("PWN", "pushing %02X to queue", data);
        }
        else{
          FURI_LOG_I("PWN", "pushing %c to queue", data);
        }

        furi_stream_buffer_send(app->rx_stream, &data, 1, 0);
        furi_thread_flags_set(furi_thread_get_id(app->uart_worker_thread), WorkerEventRx);
    }
}

static int32_t flipagotchi_uart_worker(void* context) {
    furi_assert(context);
    FlipagotchiApp* app = context;

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
    furi_hal_uart_set_irq_cb(PWNAGOTCHI_UART_CHANNEL, flipagotchi_on_irq_cb, app);

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
            size_t length = furi_stream_buffer_receive(app->rx_stream, rx_buf, RX_BUF_SIZE, 0);
            if(length > 0) {
                // TODO, pretty sure there is no reason for our queue to live in the view model
                // lets move it out into the main app
                for(size_t i = 0; i < length; i++) {
                    protocol_queue_push_byte(app->queue, rx_buf[i]);
                }
                furi_thread_flags_set(furi_thread_get_id(app->cmd_worker_thread), WorkerEventRx);
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
    FlipagotchiApp* app = context;

    FURI_LOG_I("PWN", "alloc rx stream buffert");
    // alloc incoming stream for uart thread
    app->rx_stream = furi_stream_buffer_alloc(RX_BUF_SIZE, 1);

    FURI_LOG_I("PWN", "alloc uart thread");
    // uart thread
    app->uart_worker_thread = furi_thread_alloc();
    furi_thread_set_stack_size(app->uart_worker_thread, 1024);
    furi_thread_set_context(app->uart_worker_thread, app);
    furi_thread_set_callback(app->uart_worker_thread, flipagotchi_uart_worker);
    furi_thread_start(app->uart_worker_thread);


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
            /*         app->view, */
            /*         PwnDumpModel* model, */
            /*         { */
            /*             update = flipagotchi_exec_cmd(model, app); */
            /*         }, */
            /*         update); */
            flipagotchi_exec_cmd(NULL, app);
            

            // light up the screen and blink the led
            /* notification_message(app->notification, &sequence_notification); */
            // with_view_model(
            // app->view, PwnDumpModel * model, { UNUSED(model); }, true);
        }

    }


    FURI_LOG_I("PWN", "free uart worker");
    furi_thread_flags_set(furi_thread_get_id(app->uart_worker_thread), WorkerEventStop);
    FURI_LOG_I("PWN", "free uart worker: joining");
    furi_thread_join(app->uart_worker_thread);
    FURI_LOG_I("PWN", "free uart worker: freeing");
    furi_thread_free(app->uart_worker_thread);
    FURI_LOG_I("PWN", "free uart worker: setting NULL");
    app->uart_worker_thread = NULL;

    FURI_LOG_I("PWN", "free stream buffer");
    furi_stream_buffer_free(app->rx_stream);

    return 0;
}

static FlipagotchiApp* flipagotchi_app_alloc() {
    FURI_LOG_I("PWN", "starting alloc");
    FlipagotchiApp* app = malloc(sizeof(FlipagotchiApp));

    app->synack_complete = false;

    FURI_LOG_I("PWN", "alloc gui");
    // Gui
    app->gui = furi_record_open(RECORD_GUI);
    /* app->notification = furi_record_open(RECORD_NOTIFICATION); */

    FURI_LOG_I("PWN", "alloc view dispatcher");
    // View dispatcher
    /* app->view_dispatcher = view_dispatcher_alloc(); */
    /* view_dispatcher_enable_queue(app->view_dispatcher); */
    /* view_dispatcher_attach_to_gui(app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen); */

    FURI_LOG_I("PWN", "alloc views");
    app->view_port = view_port_alloc();
    /* view_port_draw_callback_set(app->view_port, example_thermo_draw_callback, app); */
    view_port_input_callback_set(app->view_port, flipagotchi_input_callback, app);
    app->event_queue = furi_message_queue_alloc(8, sizeof(InputEvent));
    gui_add_view_port(app->gui, app->view_port, GuiLayerFullscreen);
    // Views
    /* app->view = view_alloc(); */
    /* view_set_draw_callback(app->view, flipagotchi_view_draw_callback); */
    /* view_set_input_callback(app->view, flipagotchi_view_input_callback); */
    /* view_allocate_model(app->view, ViewModelTypeLocking, sizeof(PwnDumpModel)); */
    /* with_view_model( */
    /*     app->view, */
    /*     PwnDumpModel * model, */
    /*     { */
    /*         model->pwn = pwnagotchi_alloc(); */
    /*     }, */
    /*     true); */

    //TODO uncomment, just for commented for debugging
    /* view_set_previous_callback(app->view, flipagotchi_exit); */
    /* view_dispatcher_add_view(app->view_dispatcher, 0, app->view); */
    /* view_dispatcher_switch_to_view(app->view_dispatcher, 0); */

    FURI_LOG_I("PWN", "alloc queue");
    // Queue
    app->queue = protocol_queue_alloc();

    FURI_LOG_I("PWN", "alloc threads cmd parser thread");
    // command parser thread
    app->cmd_worker_thread = furi_thread_alloc();
    furi_thread_set_stack_size(app->cmd_worker_thread, 1024);
    furi_thread_set_context(app->cmd_worker_thread, app);
    furi_thread_set_callback(app->cmd_worker_thread, flipagotchi_cmd_worker);
    furi_thread_start(app->cmd_worker_thread);

    FURI_LOG_I("PWN", "ALLC'd");

    return app;
}

static void flipagotchi_app_free(FlipagotchiApp* app) {
    FURI_LOG_I("PWN", "freeing!");
    furi_assert(app);

    FURI_LOG_I("PWN", "free cmd worker");
    // free workers
    furi_thread_flags_set(furi_thread_get_id(app->cmd_worker_thread), WorkerEventStop);
    furi_thread_join(app->cmd_worker_thread);
    furi_thread_free(app->cmd_worker_thread);
    app->cmd_worker_thread = NULL;

    FURI_LOG_I("PWN", "free queue");
    // Free Queue
    protocol_queue_free(app->queue);

    FURI_LOG_I("PWN", "free views");

    view_port_enabled_set(app->view_port, false);
    gui_remove_view_port(app->gui, app->view_port);
    furi_message_queue_free(app->event_queue);
    // Free views
    /* view_dispatcher_remove_view(app->view_dispatcher, 0); */

    /* with_view_model( */
    /*     app->view, */
    /*     PwnDumpModel * model, */
    /*     { */
    /*         pwnagotchi_free(model->pwn); */
    /*     }, */
    /*     true); */
    /* view_free(app->view); */
    /* view_dispatcher_free(app->view_dispatcher); */

    FURI_LOG_I("PWN", "free gui");
    // Close gui record
    view_port_free(app->view_port);
    furi_record_close(RECORD_GUI);
    /* furi_record_close(RECORD_NOTIFICATION); */
}

static void flipagotchi_app_run(FlipagotchiApp* app){
  UNUSED(app);
  /* An endless loop which handles the input*/
  for(bool is_running = true; is_running;) {
    InputEvent event;
    /* Wait for an input event. Input events come from the GUI thread via a callback. */
    const FuriStatus status =
      furi_message_queue_get(app->event_queue, &event, FuriWaitForever);

    /* This application is only interested in short button presses. */
    if((status != FuriStatusOk) || (event.type != InputTypeShort)) {
      continue;
    }

    /* When the user presses the "Back" button, break the loop and exit the application. */
    if(event.key == InputKeyBack) {
      is_running = false;
    }
  }
}

int32_t flipagotchi_app(void* p) {
    UNUSED(p);
    FlipagotchiApp* app = flipagotchi_app_alloc();
    flipagotchi_uart_init(app);
    flipagotchi_app_run(app);
    flipagotchi_app_free(app);
    FURI_LOG_I("PWN", "free complete");
    return 0;
}