#include "flipagotchi_app_i.h"

#include <furi.h>
#include <furi_hal.h>


static bool flipagotchi_custom_event_callback(void* context, uint32_t event) {
  furi_assert(context);
  FlipagotchiApp* app = context;
  return scene_manager_handle_custom_event(app->scene_manager, event);
}

static bool flipagotchi_back_event_callback(void* context) {
  furi_assert(context);
  FlipagotchiApp* app = context;
  return scene_manager_handle_back_event(app->scene_manager);
}

static void flipagotchi_tick_event_callback(void* context) {
  furi_assert(context);
  FlipagotchiApp* app = context;
  scene_manager_handle_tick_event(app->scene_manager);
}

static FlipagotchiApp* flipagotchi_app_alloc() {
    FURI_LOG_I("PWN", "starting alloc");
    // Self
    FlipagotchiApp* app = malloc(sizeof(FlipagotchiApp));

    // Gui
    FURI_LOG_I("PWN", "alloc gui");
    app->gui = furi_record_open(RECORD_GUI);

    // View Dispatcher
    FURI_LOG_I("PWN", "alloc view dispatcher");
    app->view_dispatcher = view_dispatcher_alloc();

    // Scene Manager
    app->scene_manager = scene_manager_alloc(&flipagotchi_scene_handlers, app);

    // View Dispatcher CBs
    view_dispatcher_enable_queue(app->view_dispatcher);
    view_dispatcher_set_event_callback_context(app->view_dispatcher, app);

    view_dispatcher_set_custom_event_callback(
                                              app->view_dispatcher, flipagotchi_custom_event_callback);
    view_dispatcher_set_navigation_event_callback(
                                                  app->view_dispatcher, flipagotchi_back_event_callback);
    view_dispatcher_set_tick_event_callback(
                                            app->view_dispatcher, flipagotchi_tick_event_callback, 100);

    view_dispatcher_attach_to_gui(app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);

    // Notifications
    app->notifications = furi_record_open(RECORD_NOTIFICATION);

    FURI_LOG_I("PWN", "alloc views");
    // Views
    app->dialog = dialog_ex_alloc();
    view_dispatcher_add_view(
                             app->view_dispatcher, FlipagotchiAppViewExitConfirm, dialog_ex_get_view(app->dialog));

    app->pwnagotchi = pwnagotchi_alloc();
    view_dispatcher_add_view(
                             app->view_dispatcher, FlipagotchiAppViewPwnagotchi, pwnagotchi_get_view(app->pwnagotchi));

    // Start Scene Manager
    scene_manager_next_scene(app->scene_manager, FlipagotchiScenePwnagotchi);

    // Uart handler
    app->flipagotchi_uart = flipagotchi_uart_alloc();

    FURI_LOG_I("PWN", "ALLC'd");

    return app;
}

static void flipagotchi_app_free(FlipagotchiApp* app) {
    FURI_LOG_I("PWN", "freeing!");
    furi_assert(app);

    // Uart HAndler
    flipagotchi_uart_free(app->flipagotchi_uart);

    FURI_LOG_I("PWN", "free views");

    // Views
    view_dispatcher_remove_view(app->view_dispatcher, FlipagotchiAppViewPwnagotchi);
    view_dispatcher_remove_view(app->view_dispatcher, FlipagotchiAppViewExitConfirm);
    pwnagotchi_free(app->pwnagotchi);
    dialog_ex_free(app->dialog);
    // View dispatcher
    view_dispatcher_free(app->view_dispatcher);

    // Scene Manager
    scene_manager_free(app->scene_manager);

    // Gui
    furi_record_close(RECORD_GUI);

    // Notifications
    furi_record_close(RECORD_NOTIFICATION);

    // Self
    free(app);
}

int32_t flipagotchi_app(void* p) {
    UNUSED(p);
    FlipagotchiApp* app = flipagotchi_app_alloc();
    view_dispatcher_run(app->view_dispatcher);
    flipagotchi_app_free(app);
    FURI_LOG_I("PWN", "free complete");
    return 0;
}