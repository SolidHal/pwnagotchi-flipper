#include "pwnagotchi.h"

/*
Icons from RogueMaster at:
https://github.com/RogueMaster/flipperzero-firmware-wPlugins/commit/8c45f8e9a921f61cda78ecdb2e58a244041d3e05
*/
#include "flipagotchi_icons.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <furi.h>

void pwnagotchi_draw_face(PwnagotchiModel* model, Canvas* canvas) {
    const Icon* currentFace;
    bool draw = true;

    switch(model->face) {
    case NoFace:
        // Draw nothing
        draw = false;
        break;
    case DefaultFace:
        currentFace = &I_awake_flipagotchi;
        break;
    case Look_r:
        currentFace = &I_look_r_flipagotchi;
        break;
    case Look_l:
        currentFace = &I_look_l_flipagotchi;
        break;
    case Look_r_happy:
        currentFace = &I_look_r_happy_flipagotchi;
        break;
    case Look_l_happy:
        currentFace = &I_look_l_happy_flipagotchi;
        break;
    case Sleep:
        currentFace = &I_sleep_flipagotchi;
        break;
    case Sleep2:
        currentFace = &I_sleep2_flipagotchi;
        break;
    case Awake:
        currentFace = &I_awake_flipagotchi;
        break;
    case Bored:
        currentFace = &I_bored_flipagotchi;
        break;
    case Intense:
        currentFace = &I_intense_flipagotchi;
        break;
    case Cool:
        currentFace = &I_cool_flipagotchi;
        break;
    case Happy:
        currentFace = &I_happy_flipagotchi;
        break;
    case Grateful:
        currentFace = &I_grateful_flipagotchi;
        break;
    case Excited:
        currentFace = &I_excited_flipagotchi;
        break;
    case Motivated:
        currentFace = &I_motivated_flipagotchi;
        break;
    case Demotivated:
        currentFace = &I_demotivated_flipagotchi;
        break;
    case Smart:
        currentFace = &I_smart_flipagotchi;
        break;
    case Lonely:
        currentFace = &I_lonely_flipagotchi;
        break;
    case Sad:
        currentFace = &I_sad_flipagotchi;
        break;
    case Angry:
        currentFace = &I_angry_flipagotchi;
        break;
    case Friend:
        currentFace = &I_friend_flipagotchi;
        break;
    case Broken:
        currentFace = &I_broken_flipagotchi;
        break;
    case Debug:
        currentFace = &I_debug_flipagotchi;
        break;
    case Upload:
        currentFace = &I_upload_flipagotchi;
        break;
    case Upload1:
        currentFace = &I_upload1_flipagotchi;
        break;
    case Upload2:
        currentFace = &I_upload2_flipagotchi;
        break;
    default:
        draw = false;
    }

    if(draw) {
        canvas_draw_icon(canvas, PWNAGOTCHI_FACE_J, PWNAGOTCHI_FACE_I, currentFace);
    }
}

void pwnagotchi_draw_name(PwnagotchiModel* model, Canvas* canvas) {
    char* formatName = malloc(sizeof(char) * (PWNAGOTCHI_MAX_HOSTNAME_LEN + 2));
    strncpy(formatName, model->hostname, PWNAGOTCHI_MAX_HOSTNAME_LEN);
    strncat(formatName, ">", 2);
    canvas_set_font(canvas, PWNAGOTCHI_FONT);
    canvas_draw_str(canvas, PWNAGOTCHI_NAME_J, PWNAGOTCHI_NAME_I, formatName);
    free(formatName);
}

void pwnagotchi_draw_channel(PwnagotchiModel* model, Canvas* canvas) {
    char* formatChannel = malloc(sizeof(char) * (PWNAGOTCHI_MAX_CHANNEL_LEN + 4));
    strncpy(formatChannel, "CH", 3);
    strcat(formatChannel, model->channel);
    canvas_set_font(canvas, PWNAGOTCHI_FONT);
    canvas_draw_str(canvas, PWNAGOTCHI_CHANNEL_J, PWNAGOTCHI_CHANNEL_I, formatChannel);
    free(formatChannel);
}

void pwnagotchi_draw_aps(PwnagotchiModel* model, Canvas* canvas) {
    char* formatAP = malloc(sizeof(char) * (PWNAGOTCHI_MAX_APS_LEN + 5));
    strncpy(formatAP, "APS", 4);
    strcat(formatAP, model->apStat);
    canvas_set_font(canvas, PWNAGOTCHI_FONT);
    canvas_draw_str(canvas, PWNAGOTCHI_APS_J, PWNAGOTCHI_APS_I, formatAP);
    free(formatAP);
}

void pwnagotchi_draw_uptime(PwnagotchiModel* model, Canvas* canvas) {
    char* formatUp = malloc(sizeof(char) * (PWNAGOTCHI_MAX_UPTIME_LEN + 4));
    strncpy(formatUp, "UP", 3);
    strcat(formatUp, model->uptime);
    canvas_set_font(canvas, PWNAGOTCHI_FONT);
    canvas_draw_str(canvas, PWNAGOTCHI_UPTIME_J, PWNAGOTCHI_UPTIME_I, formatUp);
    free(formatUp);
}

void pwnagotchi_draw_lines(PwnagotchiModel* model, Canvas* canvas) {
    UNUSED(model);
    // Line 1
    canvas_draw_line(
        canvas,
        PWNAGOTCHI_LINE1_START_J,
        PWNAGOTCHI_LINE1_START_I,
        PWNAGOTCHI_LINE1_END_J,
        PWNAGOTCHI_LINE1_END_I);

    // Line 2
    canvas_draw_line(
        canvas,
        PWNAGOTCHI_LINE2_START_J,
        PWNAGOTCHI_LINE2_START_I,
        PWNAGOTCHI_LINE2_END_J,
        PWNAGOTCHI_LINE2_END_I);
}

void pwnagotchi_draw_handshakes(PwnagotchiModel* model, Canvas* canvas) {
    char* formatShakes = malloc(sizeof(char) * (PWNAGOTCHI_MAX_HANDSHAKES_LEN + 5));
    strncpy(formatShakes, "PWND ", 6);
    strcat(formatShakes, model->handshakes);
    canvas_set_font(canvas, PWNAGOTCHI_FONT);
    canvas_draw_str(canvas, PWNAGOTCHI_HANDSHAKES_J, PWNAGOTCHI_HANDSHAKES_I, formatShakes);
    free(formatShakes);
}

void pwnagotchi_draw_mode(PwnagotchiModel* model, Canvas* canvas) {
    canvas_set_font(canvas, PWNAGOTCHI_FONT);
    switch(model->mode) {
    case PwnMode_Manual:
        canvas_draw_str(canvas, PWNAGOTCHI_MODE_MANU_J, PWNAGOTCHI_MODE_MANU_I, "MANU");
        break;
    case PwnMode_Auto:
        canvas_draw_str(canvas, PWNAGOTCHI_MODE_AUTO_J, PWNAGOTCHI_MODE_AUTO_I, "AUTO");
        break;
    case PwnMode_Ai:
        canvas_draw_str(canvas, PWNAGOTCHI_MODE_AI_J, PWNAGOTCHI_MODE_AI_I, "AI");
        break;
    }
}

void pwnagotchi_draw_status(PwnagotchiModel* model, Canvas* canvas) {
    // we don't use a monospace font like FontKeyboard because we can fit a lot more characters on average
    // using FontSecondary. This is a bummer for figuring out how many characters we can fit on screen

    // TODO figure out how to make the multi-line status fit properly

    canvas_set_font(canvas, FontSecondary);
    int fontHeight = canvas_current_font_height(canvas);

    // Apparently W is the widest character (USING a for a more average approach)
    size_t charLength = canvas_string_width(canvas, "a");

    size_t horizSpace = FLIPPER_SCREEN_WIDTH - PWNAGOTCHI_STATUS_J;
    size_t charSpaces = floor(((double)horizSpace) / charLength);
    size_t statusPixLen = canvas_string_width(canvas, model->status);
    size_t maxLines = floor((PWNAGOTCHI_STATUS_I - PWNAGOTCHI_LINE2_END_I) / ((double)fontHeight));

    size_t requiredLines = ceil(((double)statusPixLen) / horizSpace);

    size_t charIndex = 0;
    for(size_t i = 0; i < requiredLines && i < maxLines - 1; i++) {
        // Allocate the line with room for two more characters (a space and then another char)
        size_t allocSize = charSpaces + 2;
        char* line = malloc(sizeof(char) * allocSize);

        // Copy the allotted characters into line
        memcpy(line, (model->status + charIndex), allocSize);

        // Now loop backwards and cut it off at a space if we end with a letter
        size_t backspaceCount = 0;
        if(line[allocSize - 1] != ' ' && line[allocSize - 1] != '\0') {
            for(int j = allocSize - 1; j >= 0; j--) {
                if(line[j] == ' ') {
                    line[j] = '\0';
                    break;
                }
                backspaceCount++;
            }
        }

        // Lets make sure if backspaceCount is too large that we cut the word instead of drawing off the screen
        if(backspaceCount >= charSpaces) {
            backspaceCount = 0;
        }

        canvas_draw_str(canvas, PWNAGOTCHI_STATUS_J, PWNAGOTCHI_STATUS_I + (i * fontHeight), line);

        charIndex += (charSpaces - backspaceCount + 1);
        free(line);
    }
}

static void pwnagotchi_draw_callback(Canvas* canvas, void* _model) {
    PwnagotchiModel* model = _model;

    pwnagotchi_draw_face(model, canvas);
    pwnagotchi_draw_name(model, canvas);
    pwnagotchi_draw_channel(model, canvas);
    pwnagotchi_draw_aps(model, canvas);
    pwnagotchi_draw_uptime(model, canvas);
    pwnagotchi_draw_lines(model, canvas);
    pwnagotchi_draw_mode(model, canvas);
    pwnagotchi_draw_handshakes(model, canvas);
    pwnagotchi_draw_status(model, canvas);
}

static bool pwnagotchi_input_callback(InputEvent* event, void* context) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

Pwnagotchi* pwnagotchi_alloc() {
    Pwnagotchi* pwn = malloc(sizeof(Pwnagotchi));

    pwn->view = view_alloc();
    view_allocate_model(pwn->view, ViewModelTypeLocking, sizeof(PwnagotchiModel));

    with_view_model(
        pwn->view,
        PwnagotchiModel * model,
        {
            model->face = Cool;

            strncpy(model->channel, "*", 2);
            strncpy(model->apStat, "0 (0)", 6);
            strncpy(model->uptime, "00:00:00", 9);
            strncpy(model->hostname, "pwn", 4);
            strncpy(model->status, "Hack the planet!", 17);
            strncpy(model->handshakes, "0 (0)", 6);
            model->mode = PwnMode_Manual;
        },
        false
    );

    view_set_context(pwn->view, pwn);
    view_set_draw_callback(pwn->view, pwnagotchi_draw_callback);
    view_set_input_callback(pwn->view, pwnagotchi_input_callback);

    return pwn;
}

void pwnagotchi_free(Pwnagotchi* pwn) {
    furi_assert(pwn);
    view_free(pwn->view);
    free(pwn);
}

View* pwnagotchi_get_view(Pwnagotchi* pwn) {
    furi_assert(pwn);
    return pwn->view;
}
