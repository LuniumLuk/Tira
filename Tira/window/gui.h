//
// Created by Ziyi.Lu 2022/12/05
//

#ifndef GUI_H
#define GUI_H

#include <vector>
#include <string>
#include <misc/utils.h>
#include <misc/image.h>
#include <window/platform.h>

namespace tira {

    colorf const __text_color = { .9f, .9f, .9f };
    colorf const __base_color = { .3f, .3f, .3f };
    colorf const __active_color = { .9f, .9f, .9f };
    int const __tp = 1;

    void fill_rect(Image& image, int x, int y, int w, int h, colorf const& color);

    void draw_line_X(Image& image, int x, int y, int w, colorf const& color, int scale = 1);
    void draw_line_Y(Image& image, int x, int y, int h, colorf const& color, int scale = 1);
    void draw_rect(Image& image, int x, int y, int w, int h, colorf const& color, int scale = 1);

    void draw_font(Image& image, char c, int x, int y, colorf const& color, int scale = 1);
    void draw_text(Image& image, char const* text, int x, int y, colorf const& color, int scale = 1);

    struct GUI {
        GUI(int _w, int _h, int _x, int _y, int _scale = 1, bool _flip = false)
            : width(_w)
            , height(_h)
            , x(_x)
            , scale(_scale)
            , flip(_flip) {
            y = _y - 13 * scale * (flip ? -1 : 1);
            oy = y;
        }

        void process_mouse_button_event(LuGL::MOUSE_BUTTON button, bool pressed, float x, float y);
        void process_mouse_drag_event(float x, float y);
        // Call after drawing every elements in each render loop.
        void tick();

        // GUI elements.
        void text(Image& image, char const* text);
        bool button(Image& image, char const* text);
        bool radio_button(Image& image, char const* text, bool active);
        void slider_float(Image& image, float* value, float min, float max, int length = 100);

        int width, height;
        int x, y, oy;
        int scale;
        bool flip;

    private:
        float2 mouse_last_pos;
        float2 mouse_pos;
        float2 mouse_delta;
        float2 mouse_click;
        bool  mouse_button_pressed = false;
        bool  mouse_drag_start = false;
        bool  mouse_clicked = false;
    };

} // namespace tira

#endif
