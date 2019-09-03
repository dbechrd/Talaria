#include "ta_text_entry.h"
#include "ta_editor.h"
#include "ta_primitive.h"
#include "ta_font.h"
#include "ta_game.h"
#include "dlb/dlb_vector.h"

#define DEBUG_GAP_BUFFER 0

typedef struct ta_text_entry {
    char *buf;
    u32 cursor;   // index of next character, 0 = before first char, len = after last char
    u32 gap_len;  // length of gap after cursor
    u32 selection_start;
    u32 selection_len;
    bool focused;
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

static void text_entry_generate_text(ta_text_entry *text_entry)
{
    dlb_vec_reserve(text_entry->text, dlb_vec_cap(text_entry->buf) + 1);
    dlb_vec_zero(text_entry->text);
    for (u32 i = 0; i < dlb_vec_cap(text_entry->buf) - 1; i++) {
        if (i >= text_entry->cursor && i < text_entry->cursor + text_entry->gap_len) {
#if DEBUG_GAP_BUFFER
            dlb_vec_push(text_entry->text, '_');
#endif
        } else {
            dlb_vec_push(text_entry->text, text_entry->buf[i]);
        }
    }
    dlb_vec_push(text_entry->text, 0);
    text_entry->dirty = false;
}

ta_text_entry *ta_text_entry_init()
{
    ta_text_entry *text_entry = dlb_calloc(1, sizeof(*text_entry));
    ta_text_entry_set_filter(text_entry, ta_text_entry_filter_default);
    return text_entry;
}

void ta_text_entry_free(ta_text_entry **text_entry)
{
    dlb_free(*text_entry);
    *text_entry = 0;
}

void ta_text_entry_set_filter(ta_text_entry *text_entry,
    ta_text_entry_filter *filter)
{
    text_entry->filter = filter;
}

bool ta_text_entry_active(ta_text_entry *text_entry)
{
    return ta_editor_active_text_entry() == text_entry;
}

void ta_text_entry_focus(ta_text_entry *text_entry)
{
    if (!ta_text_entry_active(text_entry)) {
        ta_editor_set_active_text_entry(text_entry);
    }
    text_entry->focused = true;
}

void ta_text_entry_unfocus(ta_text_entry *text_entry)
{
    text_entry->focused = false;
}

bool ta_text_entry_focused(ta_text_entry *text_entry)
{
    return text_entry->focused;
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
    if (ta_text_entry_active(text_entry)) {
        ta_editor_set_active_text_entry(0);
        text_entry->focused = false;
        text_entry_generate_text(text_entry);
    } else {
        DLB_ASSERT(!"How did you validate a text_entry that isn't active!?");
    }
}

bool ta_text_entry_valid(ta_text_entry *text_entry)
{
    return !text_entry->dirty;
}

char *ta_text_entry_text(ta_text_entry *text_entry, u32 *len)
{
    if (text_entry->dirty) {
        text_entry_generate_text(text_entry);
    }
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
    dlb_vec_reserve(text_entry->buf, len + 1);
    dlb_memcpy(text_entry->buf, str, len);
    text_entry->cursor = len;
    text_entry->gap_len = dlb_vec_cap(text_entry->buf) - len;
    text_entry->dirty = true;
}

ta_rectf ta_text_entry_draw(ta_text_entry *text_entry, ta_rect_uv **text_rects,
    ta_vec2 *cursor)
{
    char *text = ta_text_entry_text(text_entry, 0);
    ta_vec2 cursor_offset = { 0 };
    ta_rectf bounds = ta_font_push_text(text_rects, tg_game.font, text, 0,
        true, text_entry->cursor, cursor);
    return bounds;
}