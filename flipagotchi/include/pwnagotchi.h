#pragma once

#include <furi.h>
#include <gui/canvas_i.h>
#include <stdbool.h>
#include <furi_hal_uart.h>

/// Defines the channel that the pwnagotchi uses
// TX pin 15, RX pin 16
#define PWNAGOTCHI_UART_CHANNEL FuriHalUartIdLPUART1
// TX pin 13, RX pin 14
/* #define PWNAGOTCHI_UART_CHANNEL FuriHalUartIdUSART1 */

/// Defines the baudrate that the pwnagotchi will use
#define PWNAGOTCHI_UART_BAUD 115200

/// Max length of channel data at top left
#define PWNAGOTCHI_MAX_CHANNEL_LEN 4

/// Max length of APS captured at top left
#define PWNAGOTCHI_MAX_APS_LEN 11

/// Max length for uptime
#define PWNAGOTCHI_MAX_UPTIME_LEN 11

/// Maximum length of pwnagotchi hostname
#define PWNAGOTCHI_MAX_HOSTNAME_LEN 11

/// Maximum length of pwnagotchi message
#define PWNAGOTCHI_MAX_STATUS_LEN 101

/// Maximum length of handshakes info at the bottom
#define PWNAGOTCHI_MAX_HANDSHAKES_LEN 21

/// Maximum length of a pwnagotchi SSID info displayed at the bottom
#define PWNAGOTCHI_MAX_SSID_LEN 26

/// Height of flipper screen
#define FLIPPER_SCREEN_HEIGHT 64

/// Width of flipper screen
#define FLIPPER_SCREEN_WIDTH 128


#define PWNAGOTCHI_HEIGHT FLIPPER_SCREEN_HEIGHT
#define PWNAGOTCHI_WIDTH FLIPPER_SCREEN_WIDTH
#define PWNAGOTCHI_FACE_I           25
#define PWNAGOTCHI_FACE_J           0
#define PWNAGOTCHI_NAME_I           17
#define PWNAGOTCHI_NAME_J           0
#define PWNAGOTCHI_CHANNEL_I        7
#define PWNAGOTCHI_CHANNEL_J        0
#define PWNAGOTCHI_APS_I            7
#define PWNAGOTCHI_APS_J            25
#define PWNAGOTCHI_UPTIME_I         7
#define PWNAGOTCHI_UPTIME_J         77
#define PWNAGOTCHI_LINE1_START_I    8
#define PWNAGOTCHI_LINE1_START_J    0
#define PWNAGOTCHI_LINE1_END_I      8
#define PWNAGOTCHI_LINE1_END_J      127
#define PWNAGOTCHI_LINE2_START_I    54
#define PWNAGOTCHI_LINE2_START_J    0
#define PWNAGOTCHI_LINE2_END_I      54
#define PWNAGOTCHI_LINE2_END_J      127
#define PWNAGOTCHI_HANDSHAKES_I     63
#define PWNAGOTCHI_HANDSHAKES_J     0
#define PWNAGOTCHI_MODE_AI_I        63
#define PWNAGOTCHI_MODE_AI_J        121
#define PWNAGOTCHI_MODE_AUTO_I      63
#define PWNAGOTCHI_MODE_AUTO_J      105
#define PWNAGOTCHI_MODE_MANU_I      63
#define PWNAGOTCHI_MODE_MANU_J      103
#define PWNAGOTCHI_STATUS_I        17
#define PWNAGOTCHI_STATUS_J        60

#define PWNAGOTCHI_FONT             FontSecondary


/**
 * Enum to represent possible faces to save them locally rather than transmit every time
 */
enum PwnagotchiFace {
    NoFace = 4, // 0, 1, 2, and 3 are reserved values
    DefaultFace,
    Look_r,
    Look_l,
    Look_r_happy,
    Look_l_happy,
    Sleep,
    Sleep2,
    Awake,
    Bored,
    Intense,
    Cool,
    Happy,
    Grateful,
    Excited,
    Motivated,
    Demotivated,
    Smart,
    Lonely,
    Sad,
    Angry,
    Friend,
    Broken,
    Debug,
    Upload,
    Upload1,
    Upload2
};

/** All of the faces as macros so we don't have to worry about size */
#define LOOK_R          "( ⚆_⚆)"
#define LOOK_L          "(☉_☉ )"
#define LOOK_R_HAPPY    "( ◕‿◕)"
#define LOOK_L_HAPPY    "(◕‿◕ )"
#define SLEEP           "(⇀‿‿↼)"
#define SLEEP2          "(≖‿‿≖)"
#define AWAKE           "(◕‿‿◕)"
#define BORED           "(-__-)"
#define INTENSE         "(°▃▃°)"
#define COOL            "(⌐■_■)"
#define HAPPY           "(•‿‿•)"
#define GRATEFUL        "(^‿‿^)"
#define EXCITED         "(ᵔ◡◡ᵔ)"
#define MOTIVATED       "(☼‿‿☼)"
#define DEMOTIVATED     "(≖__≖)"
#define SMART           "(✜‿‿✜)"
#define LONELY          "(ب__ب)"
#define SAD             "(╥☁╥ )"
#define ANGRY           "(-_-')"
#define FRIEND          "(♥‿‿♥)"
#define BROKEN          "(☓‿‿☓)"
#define DEBUG           "(#__#)"
#define UPLOAD          "(1__0)"
#define UPLOAD1         "(1__1)"
#define UPLOAD2         "(0__1)"

/**
 * Enum for current mode of the pwnagotchi
 */
enum PwnagotchiMode {
    PwnMode_Manual,
    PwnMode_Auto,
    PwnMode_Ai
};

typedef struct {
    /// Current face
    enum PwnagotchiFace face;
    // char* faceStr;
    /// CH channel display at top left
    char channel[PWNAGOTCHI_MAX_CHANNEL_LEN];
    /// AP text shown at the top
    char apStat[PWNAGOTCHI_MAX_APS_LEN];
    /// Uptime as text
    char uptime[PWNAGOTCHI_MAX_UPTIME_LEN];
    /// Hostname of the unit
    char hostname[PWNAGOTCHI_MAX_HOSTNAME_LEN];
    /// Status that is displayed
    char status[PWNAGOTCHI_MAX_STATUS_LEN];
    /// LAST SSID and other handshake information for the bottom
    char handshakes[PWNAGOTCHI_MAX_SSID_LEN];
    /// Current mode the pwnagotchi is in
    enum PwnagotchiMode mode;

} Pwnagotchi;

/**
 * @brief Allocates and constructs a pwnagotchi struct
 * 
 * @return Pwnagotchi* Constructed pwnagotchi pointer
 */
Pwnagotchi* pwnagotchi_alloc();

/**
 * @brief Destruct and free pwnagotchi
 * 
 * @param pwn Pwnagotchi to destruct
 */
void pwnagotchi_free(Pwnagotchi* pwn);

/**
 * Draw the default display with no additional information provided
 * 
 * @param pwn Pwnagotchi device to draw
 * @param canvas Canvas to draw on
 */
void pwnagotchi_draw_blank(Pwnagotchi* pwn, Canvas* canvas);

/**
 * Draw the stored pwnagotchi's face on the device
 * 
 * @param pwn Pwnagotchi to draw
 * @param canvas Canvas to draw on
 */
void pwnagotchi_draw_face(Pwnagotchi* pwn, Canvas* canvas);

/**
 * Draw the name of the pwnagotchi
 * 
 * @param pwn Pwnagotchi to draw
 * @param canvas Canvas to draw on
 */
void pwnagotchi_draw_name(Pwnagotchi* pwn, Canvas* canvas);

/**
 * Draw channel on pwnagotchi
 * 
 * @param pwn Pwnagotchi to draw
 * @param canvas Canvas to draw on
 */
void pwnagotchi_draw_channel(Pwnagotchi* pwn, Canvas* canvas);

/**
 * Draw aps on pwnagotchi
 * 
 * @param pwn Pwnagotchi to draw
 * @param canvas Canvas to draw on
 */
void pwnagotchi_draw_aps(Pwnagotchi* pwn, Canvas* canvas);

/**
 * Draw uptime on pwnagotchi
 * 
 * @param pwn Pwnagotchi to draw
 * @param canvas Canvas to draw on
 */
void pwnagotchi_draw_uptime(Pwnagotchi* pwn, Canvas* canvas);

/**
 * Draw lines on pwnagotchi
 * 
 * @param pwn Pwnagotchi to draw
 * @param canvas Canvas to draw on
 */
void pwnagotchi_draw_lines(Pwnagotchi* pwn, Canvas* canvas);

/**
 * Draw current mode of pwnagotchi
 * 
 * @param pwn Pwnagotchi to draw
 * @param canvas Canvas to draw on
 */
void pwnagotchi_draw_mode(Pwnagotchi* pwn, Canvas* canvas);

/**
 * Draw the number of handshakes in the PWND portion as well as the last handshake
 * 
 * @param pwn Pwnagotchi to draw
 * @param canvas Canvas to draw on
 */
void pwnagotchi_draw_handshakes(Pwnagotchi* pwn, Canvas* canvas);

/**
 * Draw the status that the pwnagotchi is showing on the screen
 * 
 * @param pwn Pwnagotchi to draw
 * @param canvas Canvas to draw on
 */
void pwnagotchi_draw_status(Pwnagotchi* pwn, Canvas* canvas);

/**
 * Runs all drawing functions to update the screen completely
 * 
 * @param pwn Pwnagotchi to draw
 * @param canvas Canvas to draw on
 */
void pwnagotchi_draw_all(Pwnagotchi* pwn, Canvas* canvas);

/**
 * Clears the screen buffer of the pwnagotchi
 * 
 * @param pwn Pwn to clear
 */
void pwnagotchi_screen_clear(Pwnagotchi* pwn);

