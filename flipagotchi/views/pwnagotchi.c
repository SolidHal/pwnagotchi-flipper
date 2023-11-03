#include "pwnagotchi.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <furi.h>

void pwnagotchi_draw_face(PwnagotchiModel* model, Canvas* canvas) {
    FURI_LOG_I("PWN", "drawing face %d", model->face);

    if(model->face < 4 || model->face >= EndFace) {
        FURI_LOG_W("PWN", "asked to draw invalid face %d", model->face);
        return;
    }

    // subtract 4 from our PwnagotchiFace value to skip over the reserved values
    // since PwnagotchiFaceIcons is 0 indexed, while PwnagotchiFace is 4 indexed
    canvas_draw_icon(
        canvas, PWNAGOTCHI_FACE_J, PWNAGOTCHI_FACE_I, PwnagotchiFaceIcons[model->face - 4]);
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

            strlcpy(model->channel, "*", sizeof(model->channel));
            strlcpy(model->apStat, "0 (0)", sizeof(model->apStat));
            strlcpy(model->uptime, "00:00:00", sizeof(model->uptime));
            strlcpy(model->hostname, "pwn", sizeof(model->hostname));
            strlcpy(model->status, "Hack the planet!", sizeof(model->status));
            strlcpy(model->handshakes, "0 (0)", sizeof(model->handshakes));
            model->mode = PwnMode_Manual;
        },
        false);

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
