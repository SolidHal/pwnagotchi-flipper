#pragma once

#include "flipagotchi_app.h"
#include "scenes/flipagotchi_scene.h"
#include "flipagotchi_uart.h"

#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/scene_manager.h>
#include <gui/modules/submenu.h>
#include <notification/notification_messages.h>
#include <gui/modules/variable_item_list.h>
#include <gui/modules/widget.h>
#include <gui/modules/dialog_ex.h>
#include "views/pwnagotchi.h"
#include <assets_icons.h>

struct FlipagotchiApp {
    Gui* gui;
    NotificationApp* notifications;
    ViewDispatcher* view_dispatcher;
    SceneManager* scene_manager;
    Widget* widget;
    DialogEx* dialog;
    FlipagotchiUart* flipagotchi_uart;
    Pwnagotchi* pwnagotchi;
};

typedef enum {
    FlipagotchiAppViewPwnagotchi,
    FlipagotchiAppViewExitConfirm,
} FlipagotchiAppView;
