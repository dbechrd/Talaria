#include "ta_console.h"
#include "ta_log.h"
#include "ta_window.h"
#include "ta_ui.h"
#include "dlb/dlb_vector.h"

typedef enum console_cmd_type {
    CONSOLE_CMD_EXIT,
    CONSOLE_CMD_CLEAR,
    CONSOLE_CMD_MOTD,
    CONSOLE_CMD_LIFE,
    CONSOLE_CMD_PING,
    CONSOLE_CMD_CAT,
    CONSOLE_CMD_LOG,

    // NOTE: Unknown must be the last command
    CONSOLE_CMD_UNKNOWN,
    CONSOLE_CMD_COUNT
} console_cmd_type;

static struct {
    const char *prompt;
    size_t prompt_len;
    char *buffer;
} console;

void ta_console_init()
{
    console.prompt = "root@talaria.dev:~# ";
    console.prompt_len = strlen(console.prompt);
}

// len is without nil
static void console_history_push(char **history, const char *str, size_t len)
{
    size_t old_len = dlb_vec_len(*history);
    dlb_vec_reserve(*history, old_len + len + 1);  // one extra for nil
    dlb_memcpy(*history + old_len, str, len);
    dlb_vec_hdr(*history)->len += len;
}
void ta_console_print(const char *str, size_t len)
{
    console_history_push(&console.buffer, str, len);
}

static void console_prompt_push(char **history)
{
    console_history_push(history, console.prompt, console.prompt_len);
}

static void console_cmd_clear(char **history)
{
    dlb_vec_free(*history);
}
static void console_cmd_motd(char **history)
{
    console_history_push(history, CSTR(
        "MOTD MOTD MOTD MOTD MOTD MOTD MOTD MOTD MOTD MOTD MOTD MOTD MOTD MOTD MOTD MOTD\n"
        "MOTD                                                                       MOTD\n"
        "MOTD                            Talaria OS v0.1                            MOTD\n"
        "MOTD                                                                       MOTD\n"
        "MOTD MOTD MOTD MOTD MOTD MOTD MOTD MOTD MOTD MOTD MOTD MOTD MOTD MOTD MOTD MOTD\n"
    ));
}
static void console_cmd_42(char **history)
{
    console_history_push(history, CSTR("The answer to life, the universe, and everything."));
}
static void console_cmd_ping(char **history)
{
    console_history_push(history, CSTR("pong"));
}
static void console_cmd_cat(char **history)
{
    console_history_push(history, CSTR("=^_^=  *meow*"));
}
static void console_cmd_log(char **history)
{
    UNUSED(history);
    ta_log_write(&tg_debug_log, SRC_SYSTEM, "Testing log writes '%s'.", "foo");
}
static void console_cmd_unknown(char **history)
{
    console_history_push(history, CSTR("Unknown command"));
}

static console_cmd_type console_exec(char **history, char *command)
{
    static struct {
        const char *cmd;
        size_t len;
        void (*handler)(char **history);
        bool newline;
    } commands[CONSOLE_CMD_COUNT] = {
        [CONSOLE_CMD_EXIT]    = { CSTR0("exit"),  console_cmd_clear,   false },
        [CONSOLE_CMD_CLEAR]   = { CSTR0("clear"), console_cmd_clear,   false },
        [CONSOLE_CMD_MOTD]    = { CSTR0("motd"),  console_cmd_motd,    true },
        [CONSOLE_CMD_LIFE]    = { CSTR0("42"),    console_cmd_42,      true },
        [CONSOLE_CMD_PING]    = { CSTR0("ping"),  console_cmd_ping,    true },
        [CONSOLE_CMD_CAT]     = { CSTR0("cat"),   console_cmd_cat,     true },
        [CONSOLE_CMD_LOG]     = { CSTR0("log"),   console_cmd_log,     true },
        [CONSOLE_CMD_UNKNOWN] = { 0, 0,           console_cmd_unknown, true }
    };

    console_history_push(history, CSTR("\n"));
    console_prompt_push(history);
    console_history_push(history, command, dlb_vec_len(command));

    console_cmd_type cmd_type = CONSOLE_CMD_UNKNOWN;
    if (!dlb_vec_len(command)) {
        return cmd_type;
    }

    for (cmd_type = 0; cmd_type < CONSOLE_CMD_COUNT; ++cmd_type) {
        // NOTE: This will many any command that doesn't have a cmd string. You
        // should ensure there's always a valid command string for all commands
        // except CONSOLE_CMD_UNKNOWN.
        if (!strncmp(command, commands[cmd_type].cmd, commands[cmd_type].len)) {
            if (commands[cmd_type].newline) {
                console_history_push(history, CSTR("\n"));
            }
            if (commands[cmd_type].handler) {
                commands[cmd_type].handler(history);
            }
            break;
        }
    }

    return cmd_type;
}

void ta_console_draw_screen()
{
    ta_log_write(&tg_debug_log, SRC_CONSOLE, "UI layout end\n");

    int offset = 20;
    int window_w = 1300;
    ta_ui_next_offset(offset, 20);
    ta_ui_next_size(window_w, WINDOW_H - 60);
    ta_ui_next_bg_color(UI_STATE_ALL, 0, 0, 0, 1.0f);
    static ta_ui_window_state console_window = { 0 };
    ta_ui_window_begin(&console_window, 0);

#if 1
    static bool auto_scroll_init = false;
#else
    static bool auto_scroll = true;
    bool auto_scroll_clicked = ta_ui_toggle_button(CSTR("Auto scroll"), &auto_scroll);
#endif

    static ta_ui_panel_state scroll_panel = { 0 };
    ta_ui_next_size(window_w - 18, WINDOW_H - 102);
    ta_ui_panel_begin(&scroll_panel, 0);

#if 1
    if (!auto_scroll_init) {
        scroll_panel.scroll.percent.y = 1.0f;
        auto_scroll_init = true;
    }
#else
    // TODO: We're currently auto-scrolling everything by saving scroll state as percentage. Need to save as pixel
    // offset in order to allow auto-scrolling to be disabled.
    if (auto_scroll) {
        if (!auto_scroll_init) {
            scroll_panel.scroll.percent.y = 1.0f;
            auto_scroll_init = true;
        } else if (scroll_panel.scroll.percent.y < 1.0f) {
            auto_scroll = false;
        } else {
            scroll_panel.scroll.percent.y = 1.0f;
        }
    }
#endif

    if (!console.buffer) {
        console_cmd_motd(&console.buffer);
    }
    ta_ui_next_margin(0, 0, 0, 0);
    ta_ui_next_pad(0, 0, 0, 0);
    ta_ui_label(console.buffer, dlb_vec_len(console.buffer));

    ta_ui_row_begin();
    ta_ui_next_margin(0, 0, 0, 0);
    ta_ui_next_pad(0, 0, 0, 0);
    ta_ui_label(console.prompt, console.prompt_len);

    ta_ui_next_size(window_w - 40, 15);
    ta_ui_next_margin(0, 0, 0, 0);
    ta_ui_next_pad(0, 0, 0, 0);
    ta_ui_next_bg_color(UI_STATE_ALL, 0, 0, 0, 1.0f);
    static ta_ui_textbox_state console_textbox = { 0 };
    if (ta_ui_textbox(0, 0, &console_textbox, TA_UI_AUTOSIZE)) {
        console_cmd_type cmd_type = console_exec(&console.buffer, console_textbox.buffer);
        if (cmd_type == CONSOLE_CMD_EXIT) {
            ta_ui_textbox_cancel(&console_textbox);
            // TODO: Hide console window
        } else {
            ta_ui_textbox_clear(&console_textbox);
        }
        console_window.scroll.percent.y = 1.0f;
    }

    ta_ui_panel_end();
    ta_ui_window_end();
    ta_log_write(&tg_debug_log, SRC_CONSOLE, "UI layout end\n");

    ta_log_write(&tg_debug_log, SRC_CONSOLE, "UI render begin\n");
    ta_ui_render();
    ta_log_write(&tg_debug_log, SRC_CONSOLE, "UI render end\n");
}