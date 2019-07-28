#include "ta_ui_scrollview.h"
#include "ta_primitive.h"
#include "ta_window.h"
#include "ta_shader.h"
#include "ta_symbol.h"
#include "ta_mouse.h"
#include "ta_scene.h"
#include "ta_game.h"
#include "dlb_vector.h"
#include "misc/gl3w.h"

#define ROW_PAD_LEFT 4
#define ROW_PAD_RIGHT 0
#define ROW_PAD_TOP 4
#define WIDGET_PAD 1
#define SCROLL_SPEED 20

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
static control_state last_control_state;

typedef enum ui_frame_type {
    UI_IMAGE,
    UI_WINDOW,
    UI_PANEL,
    UI_COUNT
} ui_frame_type;

typedef struct ui_frame {
    int index;
    ui_frame_type type;

    ta_vec2i pos;
    ta_vec2i offset;
    ta_size size;

    int row_height;
    bool row_continue;
} ui_frame;

static ui_frame *ui_frames;

typedef enum ui_color_type {
    COLOR_NONE,
    COLOR_HOVER,
    COLOR_DOWN,
    COLOR_COUNT
} ui_color_type;

static ta_rgba ui_colors[][COLOR_COUNT] = {
    [UI_IMAGE] = {
        [COLOR_NONE]  = { 0.5f, 0.5f, 0.5f, 1.0f }, //TA_COLOR_GRAY2
        [COLOR_HOVER] = { 1.0f, 1.0f, 0.0f, 1.0f }, //TA_COLOR_YELLOW
        [COLOR_DOWN]  = { 1.0f, 0.0f, 1.0f, 1.0f }, //TA_COLOR_MAGENTA
    },
    [UI_WINDOW] = {
        [COLOR_NONE]  = { 1.0f, 0.0f, 0.0f, 1.0f }, //TA_COLOR_RED
        [COLOR_HOVER] = { 0.0f, 1.0f, 0.0f, 1.0f }, //TA_COLOR_GREEN
        [COLOR_DOWN]  = { 0.0f, 0.0f, 1.0f, 1.0f }, //TA_COLOR_BLUE
    },
    [UI_PANEL] = {
        [COLOR_NONE]  = { 0.5f, 0.0f, 0.0f, 1.0f }, //TA_COLOR_RED
        [COLOR_HOVER] = { 0.0f, 0.5f, 0.0f, 1.0f }, //TA_COLOR_GREEN
        [COLOR_DOWN]  = { 0.0f, 0.0f, 0.5f, 1.0f }, //TA_COLOR_BLUE
    },
};

static bool type_is_container(ui_frame_type type)
{
    return (type == UI_WINDOW || type == UI_PANEL);
}

static ui_frame *ui_container()
{
    DLB_ASSERT(dlb_vec_len(ui_frames));
    ui_frame *frame = dlb_vec_last(ui_frames);
    while (frame != ui_frames && !type_is_container(frame->type)) {
        frame--;
    }
    DLB_ASSERT(type_is_container(frame->type));
    return frame;
}

static void ui_pop(int index)
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

static control_state *ui_before(ui_frame *control)
{
    glDisable(GL_SCISSOR_TEST);

    // Background
    ta_rect rect = { 0 };
    rect.x = control->offset.x;
    rect.y = control->offset.y;
    rect.w = control->size.w;
    rect.h = control->size.h;

    last_control_state.hover = false;
    last_control_state.down = false;
    last_control_state.pressed = false;
    last_control_state.released = false;

    ta_rgba bg_color = ui_colors[control->type][COLOR_NONE];
    if (tg_mouse.x >= rect.x && tg_mouse.x < rect.x + rect.w &&
        tg_mouse.y >= rect.y && tg_mouse.y < rect.y + rect.h)
    {
        bg_color = ui_colors[control->type][COLOR_HOVER];
        last_control_state.hover = true;
        if (ta_key_state_down(&tg_mouse.left)) {
            bg_color = ui_colors[control->type][COLOR_DOWN];
            last_control_state.down = true;
            last_control_state.pressed = ta_key_state_pressed(&tg_mouse.left);
        } else {
            last_control_state.released = ta_key_state_released(&tg_mouse.left);
        }
    }

    if (!type_is_container(control->type)) {
        ta_primitive_push_rect(rect, bg_color);
        ta_primitive_render();
        ta_primitive_clear(true);
    }

    // Content
    ta_rect clip_rect = { 0 };
    clip_rect.x = control->offset.x + ROW_PAD_LEFT;
    clip_rect.y = control->offset.y + ROW_PAD_TOP - 0; //sv->scrollbar_y.widget.offset;
    clip_rect.w = rect.w - ROW_PAD_LEFT * 2;
    clip_rect.h = rect.h - ROW_PAD_TOP * 2;
    glEnable(GL_SCISSOR_TEST);
    int inv_y = tg_window.rect.h - (clip_rect.y + clip_rect.h);
    glScissor(clip_rect.x, inv_y, clip_rect.w, clip_rect.h);

    return &last_control_state;
}

static void ui_after(ui_frame *control)
{
    ui_frame *container = ui_container();
    container->offset.x += control->size.w;
    container->row_height = MAX(container->row_height, control->size.h);
    if (!container->row_continue) {
        container->offset.x = container->pos.x;
        container->offset.y += container->row_height;
        container->row_height = 0;
    }
}

void ta_ui_window_begin(ta_vec2i *pos, ta_size *size, int *scroll_v)
{
    ui_frame *frame = dlb_vec_alloc(ui_frames);
    frame->index = dlb_vec_len(ui_frames) - 1;
    frame->type = UI_WINDOW;
    frame->pos = *pos;
    frame->offset = frame->pos;
    frame->size = *size;
    //frame->row_start = TA_VEC2I_ZERO;
    //frame->row_height = 0;
    //frame->row_continue = false;

    ui_before(frame);
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

void ta_ui_panel_begin(ta_size *size, int *index)
{
    ui_frame *container = ui_container();
    ui_frame *frame = dlb_vec_alloc(ui_frames);
    frame->index = dlb_vec_len(ui_frames) - 1;
    frame->type = UI_PANEL;
    frame->pos = container->offset;
    frame->offset = frame->pos;
    frame->size = *size;
    //frame->row_start = TA_VEC2I_ZERO;
    //frame->row_height = 0;
    //frame->row_continue = false;
    ui_before(frame);

    if (index) *index = frame->index;
}

void ta_ui_panel_end(int index)
{
    ui_pop(index);
}

void ta_ui_row_end();
void ta_ui_row_start()
{
    ui_frame *container = ui_container();
    ta_ui_row_end();
    container->row_continue = true;
}

void ta_ui_row_end()
{
    ui_frame *container = ui_container();
    if (container->row_continue) {
        container->offset.x = container->pos.x;
        container->offset.y += container->row_height;
        container->row_height = 0;
        container->row_continue = false;
    }
}

void ta_ui_pad(ta_size *size)
{
    ui_frame *container = ui_container();
    container->offset.x += size->w;
    container->offset.y += size->h;
}

control_state *ta_ui_image(ta_size *size, ta_texture *tex)
{
    DLB_ASSERT(dlb_vec_len(ui_frames));

    ui_frame *container = ui_container();
    ui_frame *frame = dlb_vec_alloc(ui_frames);
    frame->index = dlb_vec_len(ui_frames) - 1;
    frame->type = UI_IMAGE;
    frame->pos = container->offset;
    frame->offset = frame->pos;
    frame->size = *size;
    //frame->row_start = TA_VEC2I_ZERO;
    //frame->row_height = 0;
    //frame->row_continue = false;

    control_state *state = ui_before(frame);
    ta_shader_set_mat4(tg_shader_quads, SYM_U_PROJ, &MAT4_IDENT);
    ta_shader_set_mat4(tg_shader_quads, SYM_U_VIEW, &MAT4_IDENT);
    ta_shader_set_mat4(tg_shader_quads, SYM_U_MODEL, &MAT4_IDENT);
    ta_shader_set_sampler2d(tg_shader_quads, SYM_U_TEX, tex->gl_id);
    ta_rect img_rect;
    img_rect.x = frame->pos.x;
    img_rect.y = frame->pos.y;
    img_rect.w = tex->width;
    img_rect.h = tex->height;
    ta_primitive_push_rect(img_rect, TA_COLOR_INVIS);
    ta_primitive_render();
    ta_shader_set_sampler2d(tg_shader_quads, SYM_U_TEX, 0);
    ta_primitive_clear(true);
    ui_after(frame);
    return state;
}

void ta_ui_window_end()
{
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
    glDisable(GL_DEPTH_TEST);

    static ta_vec2i ui_window_pos = { 10, 10 };
    static ta_size ui_window_size = { 200, 40 };

    static ta_texture *tex_orange = 0;
    if (!tex_orange) {
        tex_orange = ta_scene_find(tg_game.scene, TA_TEXTURE, INTERN("tex_genesis_albedo"));
        DLB_ASSERT(tex_orange && tex_orange->gl_id);
    }

    static ta_texture *tex_gray = 0;
    if (!tex_gray) {
        tex_gray = ta_scene_find(tg_game.scene, TA_TEXTURE, INTERN("tex_genesis_metallic"));
        DLB_ASSERT(tex_gray && tex_gray->gl_id);
    }

    // TODO: Remove x,y coords from init() methods and only store size. Pass x,y
    //       at render time (make sure to update viewport correctly).
    ta_ui_window_begin(&ui_window_pos, &ui_window_size, 0);
    ta_ui_row_start();
    for (int i = 0; i < tg_game.player_ammo_max; i++) {
        if (i < tg_game.player_ammo) {
            ta_ui_image(&TA_SIZE(20, 20), tex_orange);
        } else {
            ta_ui_image(&TA_SIZE(20, 20), tex_gray);
        }
    }
    ta_ui_pad(&TA_SIZE(0, 4));
    ta_ui_row_start();
    for (int i = 0; i < tg_game.player_clip_max; i++) {
        if (i < tg_game.player_clip) {
            ta_ui_image(&TA_SIZE(20, 20), tex_orange);
        } else {
            ta_ui_image(&TA_SIZE(20, 20), tex_gray);
        }
    }
    ta_ui_window_end();

    glDisable(GL_SCISSOR_TEST);
    glEnable(GL_DEPTH_TEST);
}

void ui_4x4_grid(ta_texture *tex)
{
    int panel_id = -1;
    ta_ui_panel_begin(&TA_SIZE(240, 240), &panel_id);
    DLB_ASSERT(panel_id >= 0);

    for (int r = 0; r < 4; r++) {
        ta_ui_row_start();
        for (int c = 0; c < 4; c++) {
            ta_ui_pad(&TA_SIZE(4, 0));
            ta_ui_image(&TA_SIZE(80, 80), tex);
        }
        //ta_ui_row_end();
        ta_ui_pad(&TA_SIZE(0, 4));
    }
    ta_ui_pad(&TA_SIZE(0, 4));
    ta_ui_panel_end(panel_id);
}

void ta_ui_test()
{
    glDisable(GL_DEPTH_TEST);

    static ta_vec2i ui_window_pos = { 10, 10 };
    static ta_size ui_window_size = { 300, 400 };

    static ta_texture *tex_test = 0;
    if (!tex_test) {
        tex_test = ta_scene_find(tg_game.scene, TA_TEXTURE, INTERN("tex_genesis_albedo"));
        DLB_ASSERT(tex_test && tex_test->gl_id);
    }

    // TODO: Remove x,y coords from init() methods and only store size. Pass x,y
    //       at render time (make sure to update viewport correctly).
    ta_ui_window_begin(&ui_window_pos, &ui_window_size, 0);

#if 0
    ta_ui_row_start();
    ta_ui_image(&TA_SIZE(50, 50), tex_test);
    ta_ui_image(&TA_SIZE(50, 50), tex_test);
    ui_4x4_grid(tex_test);
    //ta_ui_row_end();
#else
    static int selected_index = -1;
    for (int i = 0; i < 4; i++) {
        ta_ui_row_start();
        ta_ui_pad(&TA_SIZE(4, 4));
        if (ta_ui_image(&TA_SIZE(50, 50), tex_test)->pressed) {
            selected_index = i == selected_index ? -1 : i;
        }
        if (i == selected_index) {
            ui_4x4_grid(tex_test);
        }
    }
#endif

    //ta_ui_next_size(50, 50);  // TODO: Implement this? Auto-size otherwise
    //static bool subimage1 = false;
    //static bool subimage2 = false;
    //if (ta_ui_image(&TA_SIZE(100, 20), tex_test)->pressed) {
    //    subimage1 = !subimage1;
    //}
    //if (subimage1) {
    //    //ta_ui_row_start();
    //    ta_ui_pad(&TA_SIZE(10, 0));
    //    if (ta_ui_image(&TA_SIZE(100, 20), tex_test)->pressed) {
    //        subimage2 = !subimage2;
    //    }
    //    //if (subimage2) {
    //    //    ta_ui_pad(&TA_SIZE(20, 0));
    //    //    ta_ui_sameline();
    //    //    ta_ui_image(&TA_SIZE(100, 20), tex_test);
    //    //    ta_ui_pad(&TA_SIZE(20, 0));
    //    //    ta_ui_sameline();
    //    //    ta_ui_image(&TA_SIZE(100, 20), tex_test);
    //    //}
    //    //ta_ui_row_end();
    //}
    ta_ui_window_end();

    glDisable(GL_SCISSOR_TEST);
    glEnable(GL_DEPTH_TEST);

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