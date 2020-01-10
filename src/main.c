/* JSMN_PARENT_LINKS is necessary to make parsing large structures linear in input size */
#define JSMN_PARENT_LINKS
/* JSMN_STRICT is necessary to reject invalid JSON documents */
#define JSMN_STRICT

#include "ta_timer.h"
#include "ta_log.h"
#include "ta_window.h"
#include "ta_audio.h"
#include "ta_render.h"
#include "ta_file.h"
#include "ta_scene.h"
#include "ta_shader.h"
#include "ta_ui_barchart.h"
#include "ta_texture.h"
#include "ta_mesh.h"
#include "ta_camera.h"
#include "ta_viewport.h"
#include "ta_event.h"
#include "ta_game.h"
#include "ta_keybind.h"
#include "ta_mouse.h"
#include "ta_light.h"
#include "ta_schema.h"
#include "ta_parse.h"
#include "ta_symbol.h"
#include "ta_font.h"
#include "ta_primitive.h"
#include "ta_editor.h"
#include "ta_rigid_body.h"
#include "ta_transform.h"
#include "ta_model.h"
#include "ta_entity.h"
#include "ta_player.h"
#include "ta_json.h"
#include "ta_gltf.h"

#include "dlb/dlb_types.h"
#define DLB_MURMUR3_IMPLEMENTATION
#include "dlb/dlb_murmur3.h"
#define DLB_VECTOR_IMPLEMENTATION
#include "dlb/dlb_vector.h"
#define DLB_HASH_IMPLEMENTATION
#define DLB_HASH_TEST
#include "dlb/dlb_hash.h"
#define DLB_BITSET_TEST
#include "dlb/dlb_bitset.h"
#define DLB_INDEX_IMPLEMENTATION
#define DLB_INDEX_TEST
#include "dlb/dlb_index.h"
#include "misc/gl3w.h"
#include "SDL/SDL.h"

DLB_ASSERT_HANDLER(handle_assert)
{
    tg_debug_log.flush = true;
    ta_log_write(&tg_debug_log, SRC_ASSERT,
        "\n---[DLB_ASSERT_HANDLER]-----------------\n"
        "Source file: %s:%d\n\n"
        "%s\n"
        "----------------------------------------\n",
        filename, line, expr
    );
#if _DEBUG
    __debugbreak();
#else
    char buf[8192] = { 0 };
    snprintf(buf, sizeof(buf),
        "\n---[DLB_ASSERT_HANDLER]-----------------\n"
        "Source file: %s:%d\n\n"
        "%s\n"
        "----------------------------------------\n",
        filename, line, expr
    );
    ta_window_msgbox(tg_window, SDL_MESSAGEBOX_ERROR, "ASSERT", buf);
#endif
    exit(-1);
}
dlb_assert_handler_def *dlb_assert_handler = handle_assert;

void debug_tests() {
#if _DEBUG
    parse_tests();
    dlb_hash_test();
    //dlb_bitset_test();
    dlb_index_test();
    ta_math_test();
#endif
}

void ndc_tests() {
    DLB_ASSERT(SCREEN_WRAP_X(0) == 0);
    DLB_ASSERT(SCREEN_WRAP_X(1) == 1);
    DLB_ASSERT(SCREEN_WRAP_X(WINDOW_W) == WINDOW_W);
    DLB_ASSERT(SCREEN_WRAP_X(-1) == WINDOW_W - 1);

    DLB_ASSERT(SCREEN_WRAP_Y(0) == 0);
    DLB_ASSERT(SCREEN_WRAP_Y(1) == 1);
    DLB_ASSERT(SCREEN_WRAP_Y(WINDOW_H) == WINDOW_H);
    DLB_ASSERT(SCREEN_WRAP_Y(-1) == WINDOW_H - 1);
}

// Random thoughts
// https://en.wikipedia.org/wiki/Accumulator_(energy)

int thread_test(void *data)
{
    UNUSED(data);
    //ta_json_test();
    return 0;
}

// NOTE: Only works in Subsystem:Console mode?
//#undef main

int main(int argc, char *argv[])
{
    UNUSED(argc);
    UNUSED(argv);
    DLB_ASSERT(SDL_NUM_SCANCODES == TA_SDL_NUM_SCANCODES);

    ta_timer_init();
    ta_log_init_file(&tg_debug_log, "log.txt", false, false, SRC_ALL,
        SRC_EVENT | SRC_GAME | SRC_EDITOR);
    srand((u32)ta_timer_only_ms());  // TODO: Better seed if it matters

    // TODO: Make delta_time specific to thread ids (hash table)
    SDL_Thread *thread_gltf = SDL_CreateThread(thread_test, "thread_test", 0);
    //SDL_WaitThread(thread, 0);
    SDL_DetachThread(thread_gltf);

    ta_log_write(&tg_debug_log, SRC_SYSTEM, "Running debug_tests...\n");
    debug_tests();
    ta_log_write(&tg_debug_log, SRC_SYSTEM, "Initializing symbols...\n");
    ta_symbol_init();
    ta_log_write(&tg_debug_log, SRC_SYSTEM, "Registering schema...\n");
    ta_schema_register();
    // TODO: Save size/position to a config file
    ta_log_write(&tg_debug_log, SRC_SYSTEM, "Initializing window...\n");
    ta_window_init(tg_window, 1600, 900, false);
    ta_log_write(&tg_debug_log, SRC_SYSTEM, "Running ndc_tests...\n");
    ndc_tests();
    ta_log_write(&tg_debug_log, SRC_SYSTEM, "Initializing audio...\n");
    ta_audio_listener_init(&tg_audio);
    ta_log_write(&tg_debug_log, SRC_SYSTEM, "Initializing mouse...\n");
    ta_mouse_init();
    ta_log_write(&tg_debug_log, SRC_SYSTEM, "Initializing renderer...\n");
    ta_render_init();
    ta_log_write(&tg_debug_log, SRC_SYSTEM, "Initializing primitives...\n");
    ta_primitive_init();
    ta_log_write(&tg_debug_log, SRC_SYSTEM, "Initializing game...\n");
    ta_game_init();
    ta_log_write(&tg_debug_log, SRC_SYSTEM, "Initializing editor...\n");
    ta_editor_init();

    ta_game_loop();

    ta_log_flush(&tg_debug_log);

    // TODO: Free *EVERYTHING* (at least in debug mode.. to check memory leaks)
    ta_window_free(tg_window);
    ta_log_write(&tg_debug_log, SRC_SYSTEM, "Goodbye.\n\n");
    return 0;
}