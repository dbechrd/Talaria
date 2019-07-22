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

#define CONTENT_PAD 4
#define WIDGET_PAD 1
#define SCROLL_SPEED 20

static ta_vec2i cur_pos;

#define UI_COLOR_CONTROL_NONE  (ta_rgba){ 0.5f, 0.5f, 0.5f, 1.0f }; //TA_COLOR_GRAY2
#define UI_COLOR_CONTROL_HOVER (ta_rgba){ 1.0f, 1.0f, 0.0f, 1.0f }; //TA_COLOR_YELLOW
#define UI_COLOR_CONTROL_CLICK (ta_rgba){ 1.0f, 0.0f, 0.0f, 1.0f }; //TA_COLOR_RED

static void ui_before(ta_size *size)
{
    ta_rect rect = { 0 };
    rect.x = cur_pos.x;
    rect.y = cur_pos.y;
    rect.w = size->w;
    rect.h = size->h;

    ta_rgba bg_color = UI_COLOR_CONTROL_NONE;

    if (tg_mouse.x >= rect.x && tg_mouse.x < rect.x + rect.w &&
        tg_mouse.y >= rect.y && tg_mouse.y < rect.y + rect.w)
    {
        bg_color = UI_COLOR_CONTROL_HOVER;
        if (ta_button_state_down(&tg_mouse.left)) {
            bg_color = UI_COLOR_CONTROL_CLICK;
        }
    }

    // Background color
    ta_primitive_push_rect(rect, bg_color);
    ta_primitive_render();
    ta_primitive_clear(true);

    cur_pos.x += CONTENT_PAD;
    cur_pos.y += CONTENT_PAD;

    ta_rect content_rect = { 0 };
    content_rect.x = cur_pos.x;
    content_rect.y = cur_pos.y - 0; //sv->scrollbar_y.widget.offset;
    content_rect.w = rect.w - CONTENT_PAD * 2;
    content_rect.h = rect.h - CONTENT_PAD * 2;

    glEnable(GL_SCISSOR_TEST);
    int inv_y = tg_window.rect.h - (content_rect.y + content_rect.h);
    glScissor(content_rect.x, inv_y, content_rect.w, content_rect.h);
    //glClear(GL_DEPTH_BUFFER_BIT);
}

static void ui_after()
{
    //glClear(GL_DEPTH_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);
}

void ta_ui_window_begin(ta_vec2i *pos, ta_size *size, int *scroll_v)
{
    cur_pos = *pos;
    ui_before(size);

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

    ui_after();
}

void ta_ui_image(int w, int h, ta_texture *tex)
{
    ta_vec2i old_pos = cur_pos;
    ta_size size;
    size.w = w;
    size.h = h;
    ui_before(&size);

    ta_shader_set_mat4(tg_shader_quads, SYM_U_PROJ, &MAT4_IDENT);
    ta_shader_set_mat4(tg_shader_quads, SYM_U_VIEW, &MAT4_IDENT);
    ta_shader_set_mat4(tg_shader_quads, SYM_U_MODEL, &MAT4_IDENT);
    ta_shader_set_sampler2d(tg_shader_quads, SYM_U_TEX, tex->gl_id);
    ta_rect img_rect;
    img_rect.x = cur_pos.x;
    img_rect.y = cur_pos.y;
    img_rect.w = tex->width;
    img_rect.h = tex->height;
    ta_primitive_push_rect(img_rect, TA_COLOR_INVIS);
    ta_primitive_render();
    ta_shader_set_sampler2d(tg_shader_quads, SYM_U_TEX, 0);
    ta_primitive_clear(true);

    cur_pos.x = old_pos.x;
    cur_pos.y += size.h;

    ui_after();
}

void ta_ui_window_end()
{
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

    cur_pos.x = 0;
    cur_pos.y = 0;
}

void ta_ui_test()
{
    glDisable(GL_DEPTH_TEST);

    static ta_vec2i ui_window_pos = { 20, 20 };
    static ta_size ui_window_size = { 400, 400 };

    static ta_texture *tex_test = 0;
    if (!tex_test) {
        tex_test = ta_scene_find(tg_game.scene, TA_TEXTURE, INTERN("tex_genesis_albedo"));
        DLB_ASSERT(tex_test && tex_test->gl_id);
    }

    // TODO: Remove x,y coords from init() methods and only store size. Pass x,y
    //       at render time (make sure to update viewport correctly).
    ta_ui_window_begin(&ui_window_pos, &ui_window_size, 0);
    //ta_ui_next_size(50, 50);  // TODO: Implement this? Auto-size otherwise
    ta_ui_image(50, 50, tex_test);
    ta_ui_image(50, 50, tex_test);
    ta_ui_window_end();

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