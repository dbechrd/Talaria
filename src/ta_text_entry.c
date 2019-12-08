#include "ta_text_entry.h"
#include "ta_editor.h"
#include "ta_primitive.h"
#include "ta_font.h"
#include "ta_game.h"
#include "ta_event.h"
#include "ta_scene.h"
#include "dlb/dlb_vector.h"
#include "SDL/SDL.h"

#define DEBUG_GAP_BUFFER 0

typedef struct ta_text_entry {
    char *buf;
    u32 cursor;   // index of next character, 0 = before first char, len = after last char
    u32 gap_len;  // length of gap after cursor
    u32 selection_start;
    u32 selection_len;
    bool multiline;
    bool submit;  // user requested save
    bool cancel;  // user requested discard
    bool dirty;   // true if text buffer is dirty
    char *text;   // active text (without gap buffer)
    ta_text_entry_filter *filter;
} ta_text_entry;

static bool text_entry_filter_default(char c)
{
    ta_font *font = ta_scene_find_by_name(tg_game.scene, RES_FONT, tg_game.font);
    if ((c >= font->first_char && c <= font->last_char) ||
        c == '\n')
    {
        return true;
    }
    return false;
}

ta_text_entry_filter *ta_text_entry_filter_default = &text_entry_filter_default;

static void text_entry_cursor_bof(ta_text_entry *text_entry)
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

static void text_entry_cursor_bol(ta_text_entry *text_entry)
{
    while (text_entry->cursor &&
        text_entry->buf[text_entry->cursor - 1] != '\n')
    {
        text_entry->cursor--;
        text_entry->buf[text_entry->cursor + text_entry->gap_len] = text_entry->buf[text_entry->cursor];
        text_entry->buf[text_entry->cursor] = 0;
    }
}

static void text_entry_cursor_eof(ta_text_entry *text_entry)
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

static void text_entry_cursor_eol(ta_text_entry *text_entry)
{
    while (text_entry->cursor &&
        text_entry->buf[text_entry->cursor - 1] != '\n')
    {
        text_entry->cursor--;
        text_entry->buf[text_entry->cursor + text_entry->gap_len] = text_entry->buf[text_entry->cursor];
        text_entry->buf[text_entry->cursor] = 0;
    }
}

static void text_entry_cursor_right(ta_text_entry *text_entry)
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

static void text_entry_cursor_left(ta_text_entry *text_entry)
{
    if (text_entry->cursor) {
        text_entry->cursor--;
        if (text_entry->gap_len) {
            text_entry->buf[text_entry->cursor + text_entry->gap_len] = text_entry->buf[text_entry->cursor];
            text_entry->buf[text_entry->cursor] = 0;
        }
    }
}

static void text_entry_cursor_down(ta_text_entry *text_entry)
{
    //TODO: Move cursor up
    UNUSED(text_entry);
}

static void text_entry_cursor_up(ta_text_entry *text_entry)
{
    //TODO: Move cursor down
    UNUSED(text_entry);
}

static void text_entry_backspace(ta_text_entry *text_entry)
{
    if (text_entry->cursor) {
        text_entry->cursor--;
        text_entry->gap_len++;
    }
    text_entry->dirty = true;
}

static void text_entry_delete(ta_text_entry *text_entry)
{
    u32 cap = dlb_vec_cap(text_entry->buf);
    if (text_entry->cursor + text_entry->gap_len < cap) {
        text_entry->gap_len++;
    }
    text_entry->dirty = true;
}

ta_text_entry *ta_text_entry_init()
{
    ta_text_entry *text_entry = dlb_calloc(1, sizeof(*text_entry));
    ta_text_entry_set_filter(text_entry, ta_text_entry_filter_default);
    return text_entry;
}

void ta_text_entry_free(ta_text_entry **text_entry)
{
    ta_text_entry *entry = *text_entry;
    ta_text_entry_unfocus(entry);
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

bool ta_text_entry_multiline(ta_text_entry *text_entry)
{
    return text_entry->multiline;
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

bool text_entry_insert(ta_text_entry *text_entry, char c)
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

// Save changes
void ta_text_entry_submit(ta_text_entry *text_entry)
{
    DLB_ASSERT(!text_entry->submit);  // Duplicate submit?
    DLB_ASSERT(!text_entry->cancel);  // Submit after cancel?
    ta_text_entry_validate(text_entry);
    text_entry->submit = true;
}

bool ta_text_entry_submitted(ta_text_entry *text_entry)
{
    return text_entry->submit;
}

// TODO: This should probably be a validation callback or something.. could also
//       set background color to red and maybe display a tooltip with advice how
//       to fix the error.
// Reject changes (e.g. reject empty UIDs)
void ta_text_entry_reject(ta_text_entry *text_entry)
{
    text_entry->submit = false;
}

// Discard changes
void ta_text_entry_cancel(ta_text_entry *text_entry)
{
    DLB_ASSERT(!text_entry->cancel);  // Duplicate cancel?
    DLB_ASSERT(!text_entry->submit);  // Cancel after submit? Perhaps you meant reject?
    text_entry->cancel = true;
}

bool ta_text_entry_canceled(ta_text_entry *text_entry)
{
    return text_entry->cancel;
}

char *ta_text_entry_text(ta_text_entry *text_entry, u32 *len)
{
    ta_text_entry_validate(text_entry);
    if (len) {
        *len = dlb_vec_len(text_entry->text);
    }
    return text_entry->text;
}

// TODO: Run filter on input string.. maybe?
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
    ta_font *font = ta_scene_find_by_name(tg_game.scene, RES_FONT, tg_game.font);
    u32 text_len;
    char *text = ta_text_entry_text(text_entry, &text_len);
    ta_rectf bounds = ta_font_push_text(text_rects, font, text, text_len, true,
        &text_entry->cursor, cursor, 0, 0);
    return bounds;
}

void ta_text_entry_event(ta_text_entry *text_entry, ta_event *event)
{
    bool handled = true;

    switch (event->type) {
        case TA_EVENT_EDITOR_TXT_NEWLINE: {
            if (ta_text_entry_multiline(text_entry)) {
                text_entry_insert(text_entry, '\n');
            }
            break;
        } case TA_EVENT_EDITOR_TXT_SUBMIT: {
            ta_text_entry_submit(text_entry);
            break;
        } case TA_EVENT_EDITOR_TXT_CANCEL: {
            ta_text_entry_cancel(text_entry);
            break;
        } case TA_EVENT_EDITOR_TXT_BACKSPACE: {
            text_entry_backspace(text_entry);
            break;
        } case TA_EVENT_EDITOR_TXT_CURSOR_BOF: {
            text_entry_cursor_bof(text_entry);
            break;
        } case TA_EVENT_EDITOR_TXT_CURSOR_BOL: {
            text_entry_cursor_bol(text_entry);
            break;
        } case TA_EVENT_EDITOR_TXT_CURSOR_EOF: {
            text_entry_cursor_eof(text_entry);
            break;
        } case TA_EVENT_EDITOR_TXT_CURSOR_EOL: {
            text_entry_cursor_eol(text_entry);
            break;
        } case TA_EVENT_EDITOR_TXT_DELETE: {
            text_entry_delete(text_entry);
            break;
        } case TA_EVENT_EDITOR_TXT_CURSOR_RIGHT: {
            text_entry_cursor_right(text_entry);
            break;
        } case TA_EVENT_EDITOR_TXT_CURSOR_LEFT: {
            text_entry_cursor_left(text_entry);
            break;
        } case TA_EVENT_EDITOR_TXT_CURSOR_DOWN: {
            text_entry_cursor_down(text_entry);
            break;
        } case TA_EVENT_EDITOR_TXT_CURSOR_UP: {
            text_entry_cursor_up(text_entry);
            break;
        } case TA_EVENT_TEXT_INPUT: {
            text_entry_insert(text_entry, event->data.text_input.chr);
            break;
        } case TA_EVENT_KEY_PRESS: {
            // Consume all unhandled keystrokes when text editor is active
            break;
        } case TA_EVENT_KEY_RELEASE: {
            // Consume all unhandled keystrokes when text editor is active
            break;
        } default: {
            handled = false;
        }
    }

    event->handled = handled;
}
