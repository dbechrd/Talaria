#include "ta_text_entry.h"
#include "ta_editor.h"
#include "ta_primitive.h"
#include "ta_font.h"
#include "ta_game.h"
#include "ta_event.h"
#include "dlb/dlb_vector.h"

#define DEBUG_GAP_BUFFER 0

typedef struct ta_text_entry {
    char *buf;
    u32 cursor;   // index of next character, 0 = before first char, len = after last char
    u32 gap_len;  // length of gap after cursor
    u32 selection_start;
    u32 selection_len;
    bool multiline;
    bool submitted;
    bool dirty;   // true if text buffer is dirty
    char *text;   // active text (without gap buffer)
    ta_text_entry_filter *filter;
} ta_text_entry;

static bool text_entry_filter_default(char c)
{
    if ((c >= tg_game.font->first_char && c <= tg_game.font->last_char) ||
        c == '\n')
    {
        return true;
    }
    return false;
}

ta_text_entry_filter *ta_text_entry_filter_default = &text_entry_filter_default;

ta_text_entry *ta_text_entry_init()
{
    ta_text_entry *text_entry = dlb_calloc(1, sizeof(*text_entry));
    ta_text_entry_set_filter(text_entry, ta_text_entry_filter_default);
    return text_entry;
}

void ta_text_entry_free(ta_text_entry **text_entry)
{
    ta_text_entry *entry = *text_entry;
    dlb_vec_free(entry->buf);
    dlb_vec_free(entry->text);
    dlb_free(entry);
    *text_entry = 0;
}

void ta_text_entry_set_filter(ta_text_entry *text_entry,
    ta_text_entry_filter *filter)
{
    text_entry->filter = filter;
}

void ta_text_entry_focus(ta_text_entry *text_entry)
{
    ta_editor_set_active_text_entry(text_entry);
}

void ta_text_entry_unfocus(ta_text_entry *text_entry)
{
    if (ta_text_entry_focused(text_entry)) {
        ta_editor_set_active_text_entry(0);
    }
}

bool ta_text_entry_focused(ta_text_entry *text_entry)
{
    return ta_editor_active_text_entry() == text_entry;
}

void ta_text_entry_cursor_bof(ta_text_entry *text_entry)
{
    if (text_entry->gap_len) {
        while (text_entry->cursor) {
            text_entry->cursor--;
            text_entry->buf[text_entry->cursor + text_entry->gap_len] = text_entry->buf[text_entry->cursor];
            text_entry->buf[text_entry->cursor] = 0;
        }
    } else {
        text_entry->cursor = 0;
    }
}

void ta_text_entry_cursor_eof(ta_text_entry *text_entry)
{
    u32 cap = dlb_vec_cap(text_entry->buf);
    if (text_entry->gap_len) {
        while (text_entry->cursor + text_entry->gap_len < cap) {
            text_entry->buf[text_entry->cursor] = text_entry->buf[text_entry->cursor + text_entry->gap_len];
            text_entry->buf[text_entry->cursor + text_entry->gap_len] = 0;
            text_entry->cursor++;
        }
    } else {
        text_entry->cursor = cap - text_entry->gap_len;
    }
}

void ta_text_entry_cursor_right(ta_text_entry *text_entry)
{
    u32 cap = dlb_vec_cap(text_entry->buf);
    if (text_entry->cursor + text_entry->gap_len < cap) {
        if (text_entry->gap_len) {
            text_entry->buf[text_entry->cursor] = text_entry->buf[text_entry->cursor + text_entry->gap_len];
            text_entry->buf[text_entry->cursor + text_entry->gap_len] = 0;
        }
        text_entry->cursor++;
    }
}

void ta_text_entry_cursor_left(ta_text_entry *text_entry)
{
    if (text_entry->cursor) {
        text_entry->cursor--;
        if (text_entry->gap_len) {
            text_entry->buf[text_entry->cursor + text_entry->gap_len] = text_entry->buf[text_entry->cursor];
            text_entry->buf[text_entry->cursor] = 0;
        }
    }
}

void ta_text_entry_backspace(ta_text_entry *text_entry)
{
    if (text_entry->cursor) {
        text_entry->cursor--;
        text_entry->gap_len++;
    }
    text_entry->dirty = true;
}

void ta_text_entry_delete(ta_text_entry *text_entry)
{
    u32 cap = dlb_vec_cap(text_entry->buf);
    if (text_entry->cursor + text_entry->gap_len < cap) {
        text_entry->gap_len++;
    }
    text_entry->dirty = true;
}

bool ta_text_entry_insert(ta_text_entry *text_entry, char c)
{
    if (c == '\n' && !text_entry->multiline) {
        ta_text_entry_submit(text_entry);
        return false;
    }
    if (text_entry->filter && !text_entry->filter(c)) {
        return false;
    }

    if (!text_entry->gap_len) {
        u32 cap = dlb_vec_cap(text_entry->buf);
        u32 start = text_entry->cursor + text_entry->gap_len;
        dlb_vec_reserve(text_entry->buf, dlb_vec_cap(text_entry->buf) + 1);
        u32 new_cap = dlb_vec_cap(text_entry->buf);
        text_entry->gap_len = new_cap - cap;
        u32 len = cap - start;
        dlb_memcpy(
            text_entry->buf + text_entry->cursor + text_entry->gap_len,
            text_entry->buf + text_entry->cursor,
            len
        );
    }
    text_entry->buf[text_entry->cursor] = c;
    text_entry->cursor++;
    text_entry->gap_len--;
    text_entry->dirty = true;
    return true;
}

void ta_text_entry_validate(ta_text_entry *text_entry)
{
    if (!text_entry->dirty) return;

    dlb_vec_zero(text_entry->text);
    dlb_vec_reserve(text_entry->text, dlb_vec_cap(text_entry->buf));
    for (u32 i = 0; i < dlb_vec_cap(text_entry->buf); i++) {
        if (i < text_entry->cursor || i >= text_entry->cursor + text_entry->gap_len) {
            dlb_vec_push(text_entry->text, text_entry->buf[i]);
        }
#if DEBUG_GAP_BUFFER
        else {
            dlb_vec_push(text_entry->text, '_');
        }
#endif
    }
    text_entry->dirty = false;
}

bool ta_text_entry_valid(ta_text_entry *text_entry)
{
    return !text_entry->dirty;
}

void ta_text_entry_submit(ta_text_entry *text_entry)
{
    ta_text_entry_validate(text_entry);
    text_entry->submitted = true;
}

void ta_text_entry_reject(ta_text_entry *text_entry)
{
    text_entry->submitted = false;
}

bool ta_text_entry_submitted(ta_text_entry *text_entry)
{
    return text_entry->submitted;
}

char *ta_text_entry_text(ta_text_entry *text_entry, u32 *len)
{
    ta_text_entry_validate(text_entry);
    if (len) {
        *len = dlb_vec_len(text_entry->text);
    }
    return text_entry->text;
}

void ta_text_entry_set_text(ta_text_entry *text_entry, const char *str, u32 len)
{
    DLB_ASSERT(str);
    DLB_ASSERT(len);

    if (text_entry->buf) {
        dlb_vec_zero(text_entry->buf);
    }
    dlb_vec_reserve(text_entry->buf, len);
    dlb_memcpy(text_entry->buf, str, len);
    text_entry->cursor = len;
    text_entry->gap_len = dlb_vec_cap(text_entry->buf) - len;
    text_entry->dirty = true;
}

ta_rectf ta_text_entry_draw(ta_text_entry *text_entry, ta_rect_uv **text_rects,
    ta_vec2 *cursor)
{
    u32 text_len;
    char *text = ta_text_entry_text(text_entry, &text_len);
    ta_rectf bounds = ta_font_push_text(text_rects, tg_game.font, text, text_len,
        true, text_entry->cursor, cursor);
    return bounds;
}

void ta_text_entry_event(ta_event *event)
{
    ta_text_entry *text_entry = ta_editor_active_text_entry();
    if (!text_entry) return;

    switch (sdl_event.key.keysym.scancode) {
#if 0
        // NOTE: This doesn't work because it gets double processed
        //       and the entire application exits.
        case SDL_SCANCODE_ESCAPE: {
            ta_game_state_set(text_entry->prev_state);
            text_entry = 0;
            tg_game.text_entry.filter = 0;
            break;
        }
#endif
        case SDL_SCANCODE_HOME: {
            ta_text_entry_cursor_bof(text_entry);
            break;
        } case SDL_SCANCODE_END: {
            ta_text_entry_cursor_eof(text_entry);
            break;
        } case SDL_SCANCODE_BACKSPACE: {
            ta_text_entry_backspace(text_entry);
            break;
        } case SDL_SCANCODE_DELETE: {
            ta_text_entry_delete(text_entry);
            break;
        } case SDL_SCANCODE_RIGHT: {
            ta_text_entry_cursor_right(text_entry);
            break;
        } case SDL_SCANCODE_LEFT: {
            ta_text_entry_cursor_left(text_entry);
            break;
        } case SDL_SCANCODE_RETURN: {
            ta_text_entry_insert(text_entry, '\n');
            break;
        }
#if 0
    } case SDL_SCANCODE_DOWN: {
        break;
    } case SDL_SCANCODE_UP: {
        break;
    } case SDL_SCANCODE_PAGEUP: {
        SDL_StartTextInput();
        break;
    } case SDL_SCANCODE_PAGEDOWN: {
        SDL_StopTextInput();
        break;
    }
#endif
}

    switch (event->type) {
        case TA_EVENT_WINDOW_RESIZE: {
            // Update all cameras to new aspect ratio
            dlb_vec_each(ta_camera *, cam, tg_game.scene->pools[TYP_CAMERA]) {
                if (!cam->ortho) {
                    ta_camera_recalc_projection(cam);
                }
            }
            break;
        } case TA_EVENT_CAMERA_MOVE_FORWARD: {
            camera->move_buffer = vec3_add(camera->move_buffer, camera->front);
            break;
        } case TA_EVENT_CAMERA_MOVE_BACKWARD: {
            camera->move_buffer = vec3_sub(camera->move_buffer, camera->front);
            break;
        } case TA_EVENT_CAMERA_MOVE_RIGHT: {
            camera->move_buffer = vec3_add(camera->move_buffer, camera->right);
            break;
        } case TA_EVENT_CAMERA_MOVE_LEFT: {
            camera->move_buffer = vec3_sub(camera->move_buffer, camera->right);
            break;
        } case TA_EVENT_CAMERA_MOVE_UP: {
            camera->move_buffer = vec3_add(camera->move_buffer, camera->up);
            break;
        } case TA_EVENT_CAMERA_MOVE_DOWN: {
            camera->move_buffer = vec3_sub(camera->move_buffer, camera->up);
            break;
        } case TA_EVENT_CAMERA_ROTATE: {
            if (event->data.camera_rotate.delta_yaw) {
                ta_camera_yaw(camera, event->data.camera_rotate.delta_yaw);
            }
            if (event->data.camera_rotate.delta_pitch) {
                ta_camera_pitch(camera, event->data.camera_rotate.delta_pitch);
            }
            break;
        }
    }
}