#pragma once

#include <furi.h>
#include <furi_hal_uart.h>
#include <notification/notification.h>
#include <notification/notification_messages.h>
#include <furi_hal_uart.h>
#include <furi_hal_console.h>

#include "views/pwnagotchi.h"
#include "protocol.h"
#include "protocol_queue.h"

/// Defines the channel that the pwnagotchi uses
// TX pin 15, RX pin 16
#define PWNAGOTCHI_UART_CHANNEL FuriHalUartIdLPUART1
// TX pin 13, RX pin 14
/* #define PWNAGOTCHI_UART_CHANNEL FuriHalUartIdUSART1 */

/// Defines the baudrate that the pwnagotchi will use
#define PWNAGOTCHI_UART_BAUD 115200

#define RX_BUF_SIZE 2048

typedef enum {
    WorkerEventReserved = (1 << 0), // Reserved for StreamBuffer internal event
    WorkerEventStop = (1 << 1),
    WorkerEventRx = (1 << 2),
} WorkerEventFlags;

#define WORKER_EVENTS_MASK (WorkerEventStop | WorkerEventRx)

typedef struct FlipagotchiUart FlipagotchiUart;

FlipagotchiUart* flipagotchi_uart_alloc();

void flipagotchi_uart_free(FlipagotchiUart* flip_uart);

void flipagotchi_uart_init(FlipagotchiUart* app);

