//
// Created by Ziyi.Lu 2022/12/05
//

#ifndef PLATFORM_H
#define PLATFORM_H

#include <cassert>
#include <cstdlib>
#include <string>

#define __unused_variable(var) (void(var))

namespace LuGL
{
    typedef unsigned char byte_t;
    typedef struct APPWINDOW AppWindow;
    typedef enum {KEY_A, KEY_D, KEY_S, KEY_W, KEY_SPACE, KEY_ESCAPE, KEY_I, KEY_O, KEY_P, KEY_NUM} KEY_CODE;
    typedef enum {BUTTON_L, BUTTON_R, BUTTON_NUM} MOUSE_BUTTON;

    struct TIME {
        int year;
        int month;
        int day_of_week;
        int day;
        int hour;
        int minute;
        int second;
        int millisecond;
    };
    typedef struct TIME Time;


    /**
     *  application
     */
    void initializeApplication();
    void runApplication();
    void terminateApplication();

    /**
     *  window
     */
    AppWindow *createWindow(const char *title, long width, long height, byte_t *surface_buffer);
    void destroyWindow(AppWindow *window);
    void swapBuffer(AppWindow *window);
    bool windowShouldClose(AppWindow *window);
    void setWindowTitle(AppWindow *window, const char *title);

    /**
     *  input & callbacks
     */
    void pollEvent();
    bool isKeyDown(AppWindow *window, KEY_CODE key);
    bool isMouseButtonDown(AppWindow *window, MOUSE_BUTTON button);
    void setKeyboardCallback(AppWindow *window, void(*callback)(AppWindow*, KEY_CODE, bool));
    void setMouseButtonCallback(AppWindow *window, void(*callback)(AppWindow*, MOUSE_BUTTON, bool, float, float));
    void setMouseScrollCallback(AppWindow *window, void(*callback)(AppWindow*, float));
    void setMouseDragCallback(AppWindow *window, void(*callback)(AppWindow*, float, float));

    /**
     *  time
     */
    Time getSystemTime();
}

#endif
