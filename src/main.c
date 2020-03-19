#include "glad.c"
#include "GLFW/glfw3.h"
#include "ta_audio.h"
#include "ta_editor.h"
#include "ta_game.h"
#include "ta_log.h"
#include "ta_mouse.h"
#include "ta_primitive.h"
#include "ta_schema.h"
#include "ta_symbol.h"
#include "ta_timer.h"
#include "ta_window.h"
#include "misc/glad.h"
#include "dlb/dlb_types.h"
#include "dlb/dlb_hash.h"
#include "dlb/dlb_index.h"

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
#endif
    exit(-1);
}
dlb_assert_handler_def *dlb_assert_handler = handle_assert;

// NOTE: These are just included for tests
#include "ta_math.h"
#include "ta_parse.h"

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
// https://en.wikipedia.org/wiki/Accumulator_(energy)%20

static void window_glfw_error(int code, const char* description)
{
    ta_log_write(&tg_debug_log, SRC_SYSTEM, "glfw3 error code %d: %s\n", code, description);
}

#include <stdio.h>
#include <stdlib.h>

#define RAND_ELEMENTS 8

int main(int argc, char *argv[])
{
    UNUSED(argc);
    UNUSED(argv);

    glfwSetErrorCallback(window_glfw_error);
    ta_log_write(&tg_debug_log, SRC_SYSTEM, "glfwCreateWindow...\n");
    if (!glfwInit()) {
        DLB_ASSERT(!"glfwInit failed.\n");
    }

    ta_timer_init();
    // NOTE(hack): Filters are changed again below before the main loop starts
    ta_log_init_file(&tg_debug_log, "log.txt", false, false, SRC_ALL, SRC_KEYBIND);
    srand((u32)ta_timer_only_ms());  // TODO: Better seed if it matters

    ta_log_write(&tg_debug_log, SRC_SYSTEM, "Running debug_tests...\n");
    debug_tests();
    ta_log_write(&tg_debug_log, SRC_SYSTEM, "Initializing symbols...\n");
    ta_symbol_init();
    ta_log_write(&tg_debug_log, SRC_SYSTEM, "Registering schema...\n");
    ta_schema_register();
    ta_log_write(&tg_debug_log, SRC_SYSTEM, "Initializing window...\n");
    ta_window_init(tg_window, 1600, 900, false);
    //ta_window_init(tg_window, 1920, 1080, true);
    ta_log_write(&tg_debug_log, SRC_SYSTEM, "Running ndc_tests...\n");
    ndc_tests();
    ta_log_write(&tg_debug_log, SRC_SYSTEM, "Initializing audio...\n");
    ta_audio_init();
    ta_audio_listener_init(&tg_audio_listener);
    ta_log_write(&tg_debug_log, SRC_SYSTEM, "Initializing mouse...\n");
    ta_mouse_init();
    ta_log_write(&tg_debug_log, SRC_SYSTEM, "Initializing primitives...\n");
    ta_primitive_init();
    ta_log_write(&tg_debug_log, SRC_SYSTEM, "Initializing game...\n");
    ta_game_init();
    ta_log_write(&tg_debug_log, SRC_SYSTEM, "Initializing editor...\n");
    ta_editor_init();

    // TODO: Change log_level instead once that's implemented
    // HACK: This is dumb. I just don't want debug/info.
    tg_debug_log.src_exclude |= SRC_AUDIO | SRC_CONSOLE | SRC_EVENT | SRC_GAME | SRC_EDITOR;
    ta_log_write(&tg_debug_log, SRC_SYSTEM, "Starting game loop...\n");
    ta_game_loop();

    // TODO: Free *EVERYTHING* (at least in debug mode.. to check for memory leaks)
    ta_log_write(&tg_debug_log, SRC_SYSTEM, "Cleaning up...\n");
    ta_audio_free();
    ta_window_free(tg_window);
    ta_log_write(&tg_debug_log, SRC_SYSTEM, "Goodbye.\n\n");
    ta_log_free(&tg_debug_log);
    glfwTerminate();
    return 0;
}

// Include all other .c files (i.e. "unity build"). Compilation go vroom, vroom!
#include "ta_animation.c"
#include "ta_audio.c"
#include "ta_button.c"
#include "ta_camera.c"
#include "ta_collider.c"
#include "ta_console.c"
#include "ta_editor.c"
#include "ta_event.c"
#include "ta_file.c"
#include "ta_font.c"
#include "ta_game.c"
#include "ta_gltf.c"
#include "ta_intersect.c"
#include "ta_json.c"
#include "ta_key.c"
#include "ta_keybind.c"
#include "ta_light.c"
#include "ta_log.c"
#include "ta_material.c"
#include "ta_math.c"
#include "ta_mesh.c"
#include "ta_model.c"
#include "ta_mouse.c"
#include "ta_parse.c"
#include "ta_player.c"
#include "ta_primitive.c"
#include "ta_rigid_body.c"
#include "ta_scene.c"
#include "ta_schema.c"
#include "ta_shader.c"
#include "ta_symbol.c"
#include "ta_texture.c"
#include "ta_timer.c"
#include "ta_token.c"
#include "ta_transform.c"
#include "ta_ui.c"
#include "ta_ui_barchart.c"
#include "ta_viewport.c"
#include "ta_window.c"

// Single-header implementations
#define DLB_MURMUR3_IMPLEMENTATION
#include "dlb/dlb_murmur3.h"
#undef DLB_MURMUR3_IMPLEMENTATION

#define DLB_VECTOR_IMPLEMENTATION
#include "dlb/dlb_vector.h"
#undef DLB_VECTOR_IMPLEMENTATION

#define DLB_HASH_IMPLEMENTATION
#define DLB_HASH_TEST
#include "dlb/dlb_hash.h"
#undef DLB_HASH_TEST
#undef DLB_HASH_IMPLEMENTATION

#define DLB_BITSET_TEST
#include "dlb/dlb_bitset.h"
#undef DLB_BITSET_TEST

#define DLB_INDEX_IMPLEMENTATION
#define DLB_INDEX_TEST
#include "dlb/dlb_index.h"
#undef DLB_INDEX_TEST
#undef DLB_INDEX_IMPLEMENTATION

#define CGLTF_IMPLEMENTATION
#include "misc/cgltf.h"
#undef CGLTF_IMPLEMENTATION

#undef KB
#undef MB
#undef GB
#pragma warning(push)
#pragma warning(disable: 6262)
#pragma warning(disable: 6237)
#pragma warning(disable: 6239)
#pragma warning(disable: 6240)
#pragma warning(disable: 6326)
#include "lz4.c"
#pragma warning(pop)
