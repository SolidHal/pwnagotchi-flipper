#include "../flipagotchi_app_i.h"

void flipagotchi_scene_pwnagotchi_on_enter(void* context) {
    FlipagotchiApp* app = context;

    view_dispatcher_switch_to_view(app->view_dispatcher, FlipagotchiAppViewPwnagotchi);
}

bool flipagotchi_scene_pwnagotchi_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    /* FlipagotchiApp* app = context; */
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        // TODO we can hop into other scenes using this
        // which will be useful for menus/configs
        consumed = true;
    }
    return consumed;
}

void flipagotchi_scene_pwnagotchi_on_exit(void* context) {
    UNUSED(context);
}