#include "ta_ui_scrollview.h"
#include "ta_primitive.h"
#include "ta_window.h"
#include "ta_shader.h"
#include "ta_symbol.h"
#include "ta_mouse.h"
#include "ta_scene.h"
#include "ta_game.h"
#include "ta_buffer.h"
#include "dlb_vector.h"
#include "misc/gl3w.h"

#define UI_DEBUG_CONTAINERS 0

#if 0
    #define ROW_PAD_LEFT        1 //4
    #define ROW_PAD_RIGHT       0
    #define ROW_PAD_TOP         1 //4
    #define TEXTBOX_PAD_LEFT    4
    #define TEXTBOX_PAD_TOP     4
    #define TEXTBOX_PAD_CURSOR  4
    #define WIDGET_PAD          1
    #define SCROLL_SPEED        20
#else
    #define ROW_PAD_LEFT        0
    #define ROW_PAD_RIGHT       0
    #define ROW_PAD_TOP         0
    #define GRID_PAD            0
    #define TEXTBOX_PAD_LEFT    0
    #define TEXTBOX_PAD_TOP     0
    #define TEXTBOX_PAD_CURSOR  0
    #define WIDGET_PAD          0
    #define SCROLL_SPEED        20
#endif

typedef struct control_state {
    bool hover;
    bool down;
    bool pressed;
    bool released;
} control_state;

//static ta_vec2i row_start;
//static ta_vec2i row_current;
//static int row_max_height;
//static bool row_continue;
static control_state last_frame_state;
static const char *status_msg;

typedef enum ui_frame_type {
    UI_ROOT,
    UI_WINDOW,
    UI_PANEL,
    UI_TEXTBOX,
    UI_BUTTON,
    UI_COUNT
} ui_frame_type;

typedef struct ui_frame {
    u32 index;
    ui_frame_type type;

    ta_vec2i pos;
    ta_vec2i offset;
    ta_size size;
    ta_rect margin;
    ta_rect pad;

    int row_height;
    bool row_continue;
    ta_size content_size;
} ui_frame;

static ui_frame ui_root = {
    .index = (u32)-1,
    .type = UI_ROOT
};
static ui_frame *ui_frames;

typedef struct textbox_settings {
    ta_buffer buffer;
    u32 text_len;
    u32 cursor;  // index of next character, 0 = before first char, len = after last char
    u32 selection_start;
    u32 selection_len;
} textbox_settings;

typedef enum ui_color_type {
    COLOR_NONE,
    COLOR_HOVER,
    COLOR_DOWN,
    COLOR_COUNT
} ui_color_type;

static ta_rgba ui_colors[][COLOR_COUNT] = {
    [UI_ROOT] = {
        [COLOR_NONE]  = { 1.0f, 0.0f, 0.0f, 0.5f }, //TA_COLOR_RED1
        [COLOR_HOVER] = { 1.0f, 0.0f, 0.0f, 0.5f }, //TA_COLOR_RED2
        [COLOR_DOWN]  = { 1.0f, 0.0f, 0.0f, 0.5f }, //TA_COLOR_RED3
    },
    [UI_WINDOW] = {
        [COLOR_NONE]  = { 0.0f, 1.0f, 0.0f, 0.5f }, //TA_COLOR_GREEN1
        [COLOR_HOVER] = { 0.0f, 1.0f, 0.0f, 0.5f }, //TA_COLOR_GREEN2
        [COLOR_DOWN]  = { 0.0f, 1.0f, 0.0f, 0.5f }, //TA_COLOR_GREEN3
    },
    [UI_PANEL] = {
        [COLOR_NONE]  = { 0.0f, 0.0f, 1.0f, 0.5f }, //TA_COLOR_RED
        [COLOR_HOVER] = { 0.0f, 0.0f, 1.0f, 0.5f }, //TA_COLOR_GREEN
        [COLOR_DOWN]  = { 0.0f, 0.0f, 1.0f, 0.5f }, //TA_COLOR_BLUE
    },
    [UI_TEXTBOX] = {
        [COLOR_NONE]  = { 0.5f, 0.5f, 0.5f, 0.5f }, //TA_COLOR_GRAY2
        [COLOR_HOVER] = { 1.0f, 1.0f, 0.0f, 0.5f }, //TA_COLOR_YELLOW
        [COLOR_DOWN]  = { 1.0f, 0.0f, 1.0f, 0.5f }, //TA_COLOR_MAGENTA
    },
    [UI_BUTTON] = {
        [COLOR_NONE]  = { 0.5f, 0.5f, 0.5f, 1.0f }, //TA_COLOR_GRAY2
        [COLOR_HOVER] = { 1.0f, 1.0f, 0.0f, 1.0f }, //TA_COLOR_YELLOW
        [COLOR_DOWN]  = { 1.0f, 0.0f, 1.0f, 1.0f }, //TA_COLOR_MAGENTA
    },
};

static bool type_is_container(ui_frame_type type)
{
    return (type == UI_WINDOW || type == UI_PANEL);
}

// returns closest parent container index
static ui_frame *ui_container(u32 frame_idx)
{
    DLB_ASSERT(frame_idx <= dlb_vec_len(ui_frames));

    // TODO: Could just keep track of most recent container? Idk.. this most
    // likely only runs for 1-2 iterations worst case right now.
    int i = frame_idx - 1;
    for (; i >= 0; i--) {
        if (type_is_container(ui_frames[i].type)) {
            break;
        }
    }

    ui_frame *frame = &ui_root;
    if (i >= 0) {
        DLB_ASSERT(type_is_container(ui_frames[i].type));
        frame = &ui_frames[i];
    }
    return frame;
}

static ui_frame *ui_container_last()
{
    return ui_container(dlb_vec_len(ui_frames));
}

void ta_ui_pad(int x, int y)
{
    ui_frame *container = ui_container_last();
    container->offset.x += x;
    container->offset.y += y;
}

static void ui_pop(u32 index)
{
    DLB_ASSERT(dlb_vec_len(ui_frames));
    ui_frame *frame = dlb_vec_last(ui_frames);
    while (frame != ui_frames && frame->index != index) {
        dlb_vec_popz(ui_frames);
        frame--;
    }
    DLB_ASSERT(frame->index == index);
    dlb_vec_popz(ui_frames);
}

// returns frame index
static u32 ui_frame_start(ui_frame_type type, const char *name,
    const ta_size *size, const ta_rect *margin, const ta_rect *pad)
{
    glDisable(GL_SCISSOR_TEST);

    // Allocate frame
    ui_frame *frame = dlb_vec_alloc(ui_frames);
    frame->index = dlb_vec_len(ui_frames) - 1;
    frame->type = type;
    frame->size = *size;
    if (margin) frame->margin = *margin;
    if (pad)    frame->pad = *pad;
    frame->content_size.w = frame->pad.x + frame->pad.w;
    frame->content_size.h = frame->pad.y + frame->pad.h;
    ui_frame *container = ui_container(frame->index);
    frame->pos = container->pos;
    frame->pos.x += container->offset.x;
    frame->pos.y += container->offset.y;

    // Margin
    //ta_ui_pad(frame->margin.x, frame->margin.y);

    // Background
    ta_rect rect = { 0 };
    rect.x = frame->pos.x + frame->margin.x;
    rect.y = frame->pos.y + frame->margin.y;
    rect.w = frame->size.w;
    rect.h = frame->size.h;

    last_frame_state.hover = false;
    last_frame_state.down = false;
    last_frame_state.pressed = false;
    last_frame_state.released = false;

    ta_rgba bg_color = ui_colors[frame->type][COLOR_NONE];
    if (tg_mouse.x >= rect.x && tg_mouse.x < rect.x + rect.w &&
        tg_mouse.y >= rect.y && tg_mouse.y < rect.y + rect.h)
    {
        bg_color = ui_colors[frame->type][COLOR_HOVER];
        last_frame_state.hover = true;
        if (last_frame_state.hover) {
            status_msg = name;
        }
        if (ta_key_state_down(&tg_mouse.left)) {
            bg_color = ui_colors[frame->type][COLOR_DOWN];
            last_frame_state.down = true;
            last_frame_state.pressed = ta_key_state_pressed(&tg_mouse.left);
        } else {
            last_frame_state.released = ta_key_state_released(&tg_mouse.left);
        }
    }

    // TODO: If we're going to render containers we need to defer *all*
    //       rendering to e.g. container->queue until the container pops, then
    //       render everything starting at the container for proper ordering.
    if (UI_DEBUG_CONTAINERS || !type_is_container(frame->type)) {
        ta_primitive_push_rect(rect, bg_color, UI_LAYER_EDIT_1_BG);
        ta_primitive_render(true, true);
    }

    // Container padding
    if (type_is_container(frame->type)) {
        ta_ui_pad(frame->pad.x, frame->pad.y);
    }

    // Content
    ta_rect clip_rect = { 0 };
    clip_rect.x = rect.x + frame->pad.x;
    clip_rect.y = rect.y + frame->pad.y;
    clip_rect.w = rect.w - (frame->pad.x + frame->pad.w);
    clip_rect.h = rect.h - (frame->pad.w + frame->pad.h);
    glEnable(GL_SCISSOR_TEST);
    int inv_y = tg_window.rect.h - (clip_rect.y + clip_rect.h);
    glScissor(clip_rect.x, inv_y, clip_rect.w, clip_rect.h);

    return frame->index;
}

static void ui_frame_end(u32 frame_idx)
{
    ui_frame *container = ui_container(frame_idx);
    ui_frame *frame = &ui_frames[frame_idx];

    int frame_w = 0;
    int frame_h = 0;
    if (type_is_container(frame->type)) {
        // assume all containers auto-expand for now
        frame_w = frame->margin.x + frame->content_size.w + frame->margin.w;
        frame_h = frame->margin.y + frame->content_size.h + frame->margin.h;
    } else {
        frame_w = frame->margin.x + frame->size.w + frame->margin.w;
        frame_h = frame->margin.y + frame->size.h + frame->margin.h;
    }

    container->offset.x += frame_w;
    container->row_height = MAX(container->row_height, frame_h);
    container->content_size.w = MAX(container->content_size.w, container->offset.x);
    container->content_size.h += container->row_height;

    if (!container->row_continue) {
        container->offset.x = container->pad.x;
        container->offset.y += container->row_height;
        container->row_height = 0;
    }

    // Pop container (currently required for container search to work)
    if (type_is_container(frame->type)) {
        ui_pop(frame->index);
    }
}

void ta_ui_window_begin(const char *name, const ta_size *size,
    const ta_rect *pad, int *scroll_v)
{
    glClear(GL_DEPTH_BUFFER_BIT);

    u32 frame_idx = ui_frame_start(UI_WINDOW, name, size, 0, pad);
    UNUSED(scroll_v);

#if 0
    if (sv->scrollbar_y.visible) {
        ta_ui_scrollbar *bar = &sv->scrollbar_y;
        int offset = bar->widget.offset + scroll * SCROLL_SPEED;

        if (offset < 0) {
            offset = 0;
        } else if (offset > sv->content->rect.h - bar->rect.h) {
            offset = sv->content->rect.h - bar->rect.h;
        }

        bar->widget.offset = offset;
        bar->widget.rect.y = bar->widget.offset + WIDGET_PAD;
    }
#endif

#if 0
    int content_w = content->rect.x + content->rect.w + CONTENT_PAD * 2;
    int content_h = content->rect.y + content->rect.h + CONTENT_PAD * 2;

    ta_ui_scrollbar *bar = &view->scrollbar_y;
    if (content_h > view->rect.h) {
        bar->rect.w = 10;
        bar->rect.h = view->rect.h;
        bar->rect.x = (view->rect.x + view->rect.w) - bar->rect.w;
        bar->rect.y = view->rect.y;
        bar->widget.offset = 0;
        bar->widget.rect.x = WIDGET_PAD;
        bar->widget.rect.y = bar->widget.offset + WIDGET_PAD;
        bar->widget.rect.w = bar->rect.w - WIDGET_PAD*2;
        bar->widget.rect.h = bar->rect.h*2 - view->content->rect.h - WIDGET_PAD*2;
        bar->visible = true;
    }
#endif
}

void ta_ui_panel_begin(const char *name, const ta_size *size,
    const ta_rect *margin, const ta_rect *pad, u32 *index)
{
    u32 frame_idx = ui_frame_start(UI_PANEL, name, size, margin, pad);
    if (index) *index = frame_idx;
}

void ta_ui_panel_end(u32 index)
{
    ui_frame_end(index);
}

void ta_ui_row_end()
{
    ui_frame *container = ui_container_last();
    if (container->row_continue) {
        container->offset.x = container->pad.x;
        container->offset.y += container->row_height;
        container->row_height = 0;
        container->row_continue = false;
    }
}

void ta_ui_row_begin()
{
    ui_frame *container = ui_container_last();
    ta_ui_row_end();
    container->row_continue = true;
}

void ta_ui_tooltip(const char *text, u32 text_len)
{
    float x = tg_mouse.x + 10.0f;
    float y = tg_mouse.y + 20.0f;
    ta_rectf text_rect = ta_font_push_text(&tooltip_fg_queue, tg_game.font, x, y,
        UI_LAYER_TIP, text, text_len, true, 0, 0);

    ta_rect_uv tooltip_bg = { 0 };
    tooltip_bg.rect.x = x - 10.0f;
    tooltip_bg.rect.y = y;
    tooltip_bg.rect.w = text_rect.w + 20.0f;
    tooltip_bg.rect.h = text_rect.h;
    ta_primitive_push_rect_uv(&tooltip_bg_queue, tooltip_bg, TA_COLOR_GRAY3A,
        UI_LAYER_TIP_BG, true);
}

void ta_ui_statusbar()
//void ta_ui_statusbar(int x, int y)
{
    const float statusbar_pad = 4;
    ta_rect_uv status_bg = { 0 };
    status_bg.rect.x = statusbar_pad;
    status_bg.rect.y = -(float)(tg_game.font->line_height + statusbar_pad);
    status_bg.rect.w = (float)(tg_window.rect.w - statusbar_pad * 2);
    status_bg.rect.h = (float)tg_game.font->line_height;
    ta_primitive_push_rect_uv(&tooltip_bg_queue, status_bg, TA_COLOR_GRAY3A,
        UI_LAYER_TIP_BG, true);
}

bool ta_ui_button(const char *name, const ta_size *size, const ta_rect *margin,
    const ta_rect *pad, const ta_texture *tex)
{
    u32 frame_idx = ui_frame_start(UI_BUTTON, name, size, margin, pad);
    if (tex) {
        ta_shader_set_mat4(tg_shader_quads, SYM_U_PROJ, &MAT4_IDENT);
        ta_shader_set_mat4(tg_shader_quads, SYM_U_VIEW, &MAT4_IDENT);
        ta_shader_set_mat4(tg_shader_quads, SYM_U_MODEL, &MAT4_IDENT);
        ta_shader_set_sampler2d(tg_shader_quads, SYM_U_TEX, tex->gl_id);
        ta_rect img_rect;
        img_rect.x = ui_frames[frame_idx].pos.x;
        img_rect.y = ui_frames[frame_idx].pos.y;
        img_rect.w = tex->width;
        img_rect.h = tex->height;
        ta_primitive_push_rect(img_rect, TA_COLOR_INVIS, UI_LAYER_EDIT_1);
        ta_primitive_render(true, true);
        ta_shader_set_sampler2d(tg_shader_quads, SYM_U_TEX, 0);
    }
    ui_frame_end(frame_idx);

    return last_frame_state.pressed;
}

void ta_ui_textbox(const char *name, const ta_size *size, const ta_rect *margin,
    const ta_rect *pad, textbox_settings *settings)
{
    DLB_ASSERT(settings);

    u32 frame_idx = ui_frame_start(UI_TEXTBOX, name, size, margin, pad);
    ui_frame *frame = &ui_frames[frame_idx];

    // Calculate text rect
    static ta_vert_quad *text_queue;
    float cursor_x = 0.0f;
    float text_left = (float)frame->pos.x + TEXTBOX_PAD_LEFT;
    float text_top = (float)frame->pos.y + TEXTBOX_PAD_TOP;
    ta_rectf text_rect = ta_font_push_text(&text_queue, tg_game.font, text_left,
        text_top, UI_LAYER_EDIT_1, (char *)(settings->buffer.data),
        settings->buffer.length, true, settings->cursor, &cursor_x);

    // Render background
    ta_rect bg_rect;
    bg_rect.x = frame->pos.x;
    bg_rect.y = frame->pos.y;
    bg_rect.w = (int)(text_rect.w + 0.5f);
    bg_rect.h = (int)(text_rect.h + 0.5f);
    ta_primitive_push_rect(bg_rect, TA_COLOR_GRAY2, UI_LAYER_EDIT_1_BG);
    ta_primitive_render(true, false);

    // Render text
    ta_font_render(text_queue, tg_game.font, 0, true, true);
    dlb_vec_clear(text_queue);

    ui_frame_end(frame_idx);

    // TODO: Remove scissors and move back up before frame_end, was just debugging
    // Render cursor
    glDisable(GL_SCISSOR_TEST);
    ta_rect cursor_rect = bg_rect;
    cursor_rect.x = (int)cursor_x;
    cursor_rect.y += TEXTBOX_PAD_CURSOR;
    cursor_rect.w = 1;
    cursor_rect.h -= TEXTBOX_PAD_CURSOR * 2;
    ta_primitive_push_rect(cursor_rect, TA_COLOR_RED, UI_LAYER_EDIT_2);
    ta_primitive_render(true, false);
    glEnable(GL_SCISSOR_TEST);
}

void ta_ui_window_end()
{
    glDisable(GL_SCISSOR_TEST);

    // TODO: Render scrollbar on top of content, don't try to pad beforehand
    //       since we don't yet know the size.
#if 0
    if (sv->scrollbar_y.visible) {
        // Scrollbar background
        static ta_rgba scrollbar_color = { 0.5f, 0.5f, 0.5f, 0.5f };
        ta_primitive_push_rect(parent, sv->scrollbar_y.rect, scrollbar_color);

        // Scrollbar widget
        ta_rect widget_parent = parent;
        widget_parent.x += sv->scrollbar_y.rect.x;
        widget_parent.y += sv->scrollbar_y.rect.y;
        static ta_rgba widget_color = { 0.2f, 0.2f, 0.2f, 0.5f };
        ta_primitive_push_rect(widget_parent, sv->scrollbar_y.widget.rect,
            widget_color);
    }
#endif
    dlb_vec_clearz(ui_frames);
}

void ta_ui_hud()
{
    //ta_ui_pad(&TA_SIZE(10, 10));

    // TODO: Remove x,y coords from init() methods and only store size. Pass x,y
    //       at render time (make sure to update viewport correctly).
    ta_ui_window_begin(INTERN("hud"), &TA_SIZE(200, 40), 0, 0);
    ta_ui_row_begin();
    for (int i = 0; i < tg_game.player_ammo_max; i++) {
        if (i < tg_game.player_ammo) {
            ta_ui_button(INTERN("clip_slot_full"), &TA_SIZE(20, 20), 0, 0, tg_game.tex_orange);
        } else {
            ta_ui_button(INTERN("clip_slot_empty"), &TA_SIZE(20, 20), 0, 0, tg_game.tex_red);
        }
    }
    ta_ui_pad(0, 4);
    ta_ui_row_begin();
    for (int i = 0; i < tg_game.player_clip_max; i++) {
        if (i < tg_game.player_clip) {
            ta_ui_button(INTERN("ammo_slot_full"), &TA_SIZE(20, 20), 0, 0, tg_game.tex_orange);
        } else {
            ta_ui_button(INTERN("ammo_slot_empty"), &TA_SIZE(20, 20), 0, 0, tg_game.tex_red);
        }
    }
    ta_ui_window_end();

    glDisable(GL_SCISSOR_TEST);
}

static const ta_rect grid_pad = { 1, 1, 1, 1 };

void ui_4x4_grid(int rows, ta_texture *tex)
{
    for (int r = 0; r < rows; r++) {
        ta_ui_row_begin();
        for (int c = 0; c < 4; c++) {
            ta_ui_button(INTERN("4x4_cell"), &TA_SIZE(20, 20), &grid_pad, &grid_pad, tex);
        }
    }
}

void ta_ui_test()
{
    // TODO: Remove x,y coords from init() methods and only store size. Pass x,y
    //       at render time (make sure to update viewport correctly).
    ta_ui_window_begin(INTERN("test_window"), &TA_SIZE(300, 400), &TA_RECT1(2), 0);

    ta_ui_row_begin();
    static textbox_settings text_settings;
    if (!text_settings.buffer.length) {
        ta_buffer_init(&text_settings.buffer, 32);
        dlb_memcpy(text_settings.buffer.data, CSTR("|`_,_`|"));
    }
    ta_ui_textbox(INTERN("test_textbox"), &TA_SIZE(200, 50), 0, 0, &text_settings);

    enum {
        CATEGORY_AUDIO,
        CATEGORY_NOT_USED_1,
        CATEGORY_NOT_USED_2,
        CATEGORY_NOT_USED_3,
        CATEGORY_COUNT
    };
    const char *category_names[CATEGORY_COUNT] = {
        [CATEGORY_AUDIO]      = INTERN(STRING(CATEGORY_AUDIO)),
        [CATEGORY_NOT_USED_1] = INTERN(STRING(CATEGORY_NOT_USED_1)),
        [CATEGORY_NOT_USED_2] = INTERN(STRING(CATEGORY_NOT_USED_2)),
        [CATEGORY_NOT_USED_3] = INTERN(STRING(CATEGORY_NOT_USED_3)),
    };
    static int category_selected = -1;

    ta_ui_row_begin();
    u32 category_panel_id = (u32)-1;
    ta_ui_panel_begin(INTERN("category_panel"), &TA_SIZE(50, 50), 0, &TA_RECT1(2), &category_panel_id);
    for (int i = 0; i < CATEGORY_COUNT; i++) {
        ta_ui_row_begin();
        if (ta_ui_button(INTERN("category_button"), &TA_SIZE(50, 50), 0, &TA_RECT1(2), tg_game.tex_orange)) {
            category_selected = (i == category_selected ? -1 : i);
        }
        if (last_frame_state.hover) {
            ta_ui_tooltip(SYM(category_names[i]));
        }
    }
    ta_ui_panel_end(category_panel_id);

    switch (category_selected) {
        case -1: {
            break;
        } case CATEGORY_AUDIO: {
            // Audio buffers
            static const char *audio_playing_uid = 0;
            ta_audio_buffer *audio_buffers = tg_game.scene->pools[TA_AUDIO_BUFFER];
            u32 buf_count = dlb_vec_len(audio_buffers);

            int audio_playing_idx = -1;
            int audio_request_idx = -1;

            u32 audio_panel_id = (u32)-1;
            ta_ui_panel_begin(INTERN("sound_panel"), &TA_SIZE(50, 50), 0, &TA_RECT1(2), &audio_panel_id);

            ta_ui_row_begin();
            for (u32 i = 0; i < buf_count; i++) {
#if 0
                int panel_id = -1;
                ta_ui_panel_begin(&TA_SIZE(60 * buf_count, 60), &panel_id);
                DLB_ASSERT(panel_id >= 0);

                ta_ui_label(audio_buffers[i].uid.uid);
                ta_ui_row_begin();
                ta_ui_button("Play");
                ta_ui_button("Loop");

                ta_ui_pad(&TA_SIZE(0, 4));
                ta_ui_panel_end(panel_id);
#endif
                if (audio_buffers[i].uid.uid == audio_playing_uid) {
                    audio_playing_idx = i;
                }
                if (ta_ui_button(INTERN("sound_button"), &TA_SIZE(50, 50), 0, &TA_RECT1(2), 0)) { //tg_game.tex_orange)) {
                    audio_request_idx = i;
                }
                if (last_frame_state.hover) {
                    ta_ui_tooltip(SYM(audio_buffers[i].uid.uid));
                }
            }

            if (audio_request_idx >= 0) {
                ta_audio_source_stop(tg_game.background_music);
                audio_playing_uid = 0;
                if (audio_request_idx != audio_playing_idx) {
                    ta_audio_source_set_buffer(tg_game.background_music, &audio_buffers[audio_request_idx]);
                    ta_audio_source_play_loop(tg_game.background_music);
                    audio_playing_uid = audio_buffers[audio_request_idx].uid.uid;
                }
            }

            ta_ui_panel_end(audio_panel_id);
            break;
        } default: {
            u32 category_details_id = (u32)-1;
            ta_ui_panel_begin(INTERN("category_details_panel"), &TA_SIZE(240, 240), 0, &grid_pad, &category_details_id);
            ui_4x4_grid(category_selected + 1, tg_game.tex_orange);
            ta_ui_panel_end(category_details_id);
            break;
        }
    }

    ta_ui_window_end();

    if (status_msg) {
        ta_ui_statusbar();

        ta_rectf status_rect = ta_font_push_text(&quads_queue, tg_game.font, 0,
            0, UI_LAYER_TIP, SYM(status_msg), true, 0, 0);
        int status_halfw = tg_window.rect.w / 2 - (int)status_rect.w / 2;

        const int status_pad_bottom = 20;
        ta_vec3 status_pos = { 0 };
        status_pos.x = (float)status_halfw;
        status_pos.y = (float)(tg_window.rect.h - (tg_game.font->ascent + status_pad_bottom));
        ta_font_render(quads_queue, tg_game.font, &status_pos, true, true);

        status_msg = 0;
    }

    // Render tooltips
    ta_primitive_render_quads(tooltip_bg_queue, tg_shader_quads, true, true);
    ta_font_render(tooltip_fg_queue, tg_game.font, 0, true, true);

#if 0
    // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
    if (show_demo_window)
        ImGui::ShowDemoWindow(&show_demo_window);

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
    {
        static float f = 0.0f;
        static int counter = 0;

        ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

        ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
        ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
        ImGui::Checkbox("Another Window", &show_another_window);

        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
        ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

        if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
            counter++;
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();
    }

    // 3. Show another simple window.
    if (show_another_window)
    {
        ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
        ImGui::Text("Hello from another window!");
        if (ImGui::Button("Close Me"))
            show_another_window = false;
        ImGui::End();
    }

#endif
}