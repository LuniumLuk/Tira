//
// Created by Ziyi.Lu 2022/12/05
//

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <utility>
#include <platform.h>

// reference : http://www.winprog.org/tutorial/simple_window.html

const char g_szClassName[] = "viewer_window_class";

struct LuGL::APPWINDOW
{
    HWND        handle;
    byte_t      *surface;
    int         width;
    int         height;
    bool        keys[KEY_NUM];
    bool        buttons[BUTTON_NUM];
    bool        should_close;
    void        (*keyboardCallback)(AppWindow *window, KEY_CODE key, bool pressed);
    void        (*mouseButtonCallback)(AppWindow *window, MOUSE_BUTTON button, bool pressed, float x, float y);
    void        (*mouseScrollCallback)(AppWindow *window, float offset);
    void        (*mouseDragCallback)(AppWindow *window, float x, float y);
};

LuGL::APPWINDOW * g_window;
LuGL::byte_t * g_paint_surface;

HINSTANCE   g_hInstance;
POINTS      g_mouse_pts;
BITMAPINFO  g_bitmapinfo;
bool        g_update_paint = false;

static void handleKeyPress(WPARAM wParam, bool pressed)
{
    static LuGL::KEY_CODE key;
    // reference : https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
    switch(wParam)
    {
        case 0x41: key = LuGL::KEY_A;      break;
        case 0x44: key = LuGL::KEY_D;      break;
        case 0x53: key = LuGL::KEY_S;      break;
        case 0x57: key = LuGL::KEY_W;      break;
        case 0x20: key = LuGL::KEY_SPACE;  break;
        case 0x1B: key = LuGL::KEY_ESCAPE; break;
        case 0x49: key = LuGL::KEY_I;      break;
        case 0x4F: key = LuGL::KEY_O;      break;
        case 0x50: key = LuGL::KEY_P;      break;
        default:   key = LuGL::KEY_NUM;    break;
    }

    if (key < LuGL::KEY_NUM)
    {
        g_window->keys[key] = pressed;
        if (g_window->keyboardCallback)
        {
            g_window->keyboardCallback(g_window, key, pressed);
        }
    }
}

static void handleMouseDrag(float x, float y)
{
    if (g_window->mouseDragCallback)
    {
        // inverse Y
        g_window->mouseDragCallback(g_window, x, g_window->height - y);
    }
}

static void handleMouseButton(LuGL::MOUSE_BUTTON button, bool pressed, float x, float y)
{
    g_window->buttons[button] = pressed;
    if (g_window->mouseButtonCallback)
    {
        g_window->mouseButtonCallback(g_window, button, pressed, x, g_window->height - y);
    }
}

static void handleMouseScroll(float delta)
{
    if (g_window->mouseScrollCallback)
    {
        g_window->mouseScrollCallback(g_window, delta);
    }
}

static void blitRGB2BGR(const LuGL::byte_t * src, LuGL::byte_t * tar, const size_t & width, const size_t & height)
{
    size_t image_size = width * height * 3;
    for (size_t i = 0; i < image_size; i += 3)
    {
        tar[i] = src[i + 2];
        tar[i + 1] = src[i + 1];
        tar[i + 2] = src[i];
    }
}

// handling windows procedures
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // reference : https://docs.microsoft.com/en-us/windows/win32/inputdev

    // TODO: need higher swap speed
    // note: if draws here, input will not block paint
    if (g_update_paint)
    {
        HDC hdc = GetDC(hwnd);

        blitRGB2BGR(g_window->surface, g_paint_surface, g_window->width, g_window->height);
        SetDIBitsToDevice(
            hdc,
            0, 0,
            g_window->width,
            g_window->height,
            0, 0, 0, g_window->height,
            (void*)(g_paint_surface),
            &g_bitmapinfo,
            DIB_RGB_COLORS
        );

        ReleaseDC(hwnd, hdc);
        g_update_paint = false;
    }

    switch(msg)
    {
        // handle keyboard input
        case WM_KEYDOWN:
            handleKeyPress(wParam, true);
            break;
        case WM_KEYUP:
            handleKeyPress(wParam, false);
            break;
        // handle mouse input
        case WM_LBUTTONDOWN:
            g_mouse_pts = MAKEPOINTS(lParam); 
            handleMouseButton(LuGL::BUTTON_L, true, g_mouse_pts.x, g_mouse_pts.y);
            break;
        case WM_LBUTTONUP:
            g_mouse_pts = MAKEPOINTS(lParam); 
            handleMouseButton(LuGL::BUTTON_L, false, g_mouse_pts.x, g_mouse_pts.y);
            break;
        case WM_RBUTTONDOWN:
            g_mouse_pts = MAKEPOINTS(lParam); 
            handleMouseButton(LuGL::BUTTON_R, true, g_mouse_pts.x, g_mouse_pts.y);
            break;
        case WM_RBUTTONUP:
            g_mouse_pts = MAKEPOINTS(lParam); 
            handleMouseButton(LuGL::BUTTON_R, false, g_mouse_pts.x, g_mouse_pts.y);
            break;
        case WM_MOUSEMOVE:
            if (wParam & MK_LBUTTON) // left mouse drag
            {
                g_mouse_pts = MAKEPOINTS(lParam); 
                handleMouseDrag(g_mouse_pts.x, g_mouse_pts.y);
            }
            break;
        case WM_MOUSEWHEEL:
            handleMouseScroll(GET_WHEEL_DELTA_WPARAM(wParam) > 0 ? 1.0f : -1.0f);
            break;
        case WM_PAINT:
            // note: if not draw here, pollEvent will wait until receiving inputs
            // therfore fps will drop when there's no input
            {
                HDC hdc = GetDC(hwnd);

                blitRGB2BGR(g_window->surface, g_paint_surface, g_window->width, g_window->height);
                SetDIBitsToDevice(
                    hdc,
                    0, 0,
                    g_window->width,
                    g_window->height,
                    0, 0, 0, g_window->height,
                    (void*)(g_paint_surface),
                    &g_bitmapinfo,
                    DIB_RGB_COLORS
                );

                ReleaseDC(hwnd, hdc);
                g_update_paint = false;
            }
            break;
        case WM_CLOSE:
            g_window->should_close = true;
            DestroyWindow(hwnd);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

void LuGL::initializeApplication()
{
    WNDCLASSEX wc;
    g_hInstance = GetModuleHandle(NULL);

    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.style         = 0;
    wc.lpfnWndProc   = WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = g_hInstance;
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = g_szClassName;
    wc.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassEx(&wc))
    {
        MessageBox(NULL, "Window Registration Failed!", "Error!",
            MB_ICONEXCLAMATION | MB_OK);
    }
}

// need no implementation
void LuGL::runApplication() {};

void LuGL::terminateApplication()
{
    delete[] g_paint_surface;
};

void LuGL::setWindowTitle(AppWindow *window, const char *title)
{
    SetWindowText(window->handle, title);
}


LuGL::AppWindow* LuGL::createWindow(const char *title, long width, long height, byte_t *surface_buffer)
{
    HWND hwnd = CreateWindowEx(
        0,
        g_szClassName,
        title,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height,
        NULL, NULL, g_hInstance, NULL);

    if (hwnd == NULL)
    {
        MessageBox(NULL, "Window Creation Failed!", "Error!",
            MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    // fix incorrect window size due to titlebar and border
    RECT winRect, cliRect;
    GetWindowRect(hwnd, &winRect);
    GetClientRect(hwnd, &cliRect);

    long wDiff = width - cliRect.right;
    long hDiff = height - cliRect.bottom;

    MoveWindow(hwnd, winRect.left, winRect.top, width + wDiff, height + hDiff, TRUE);

    g_bitmapinfo.bmiHeader.biSize           = sizeof(BITMAPINFOHEADER);
    g_bitmapinfo.bmiHeader.biBitCount       = 24;
    g_bitmapinfo.bmiHeader.biWidth          = width;
    g_bitmapinfo.bmiHeader.biHeight         = -height;
    g_bitmapinfo.bmiHeader.biCompression    = BI_RGB;
    g_bitmapinfo.bmiHeader.biClrUsed        = 0;
    g_bitmapinfo.bmiHeader.biClrImportant   = 0;
    g_bitmapinfo.bmiHeader.biPlanes         = 1;
    g_bitmapinfo.bmiHeader.biSizeImage      = 0;

    g_window = new LuGL::AppWindow();
    g_window->handle = hwnd;
    g_window->surface = surface_buffer;
    g_window->width = width;
    g_window->height = height;
    g_paint_surface = new LuGL::byte_t[width * height * 3];
    memset(g_paint_surface, 0, sizeof(LuGL::byte_t) * width * height * 3);

    ShowWindow(hwnd, SW_SHOWNORMAL);
    UpdateWindow(hwnd);

    return g_window;
}

void LuGL::destroyWindow(AppWindow *window)
{
    window->should_close = true;
}

void LuGL::swapBuffer(AppWindow *window)
{
    __unused_variable(window);
    g_update_paint = true;
}

bool LuGL::windowShouldClose(AppWindow *window)
{
    return window->should_close;
}

void LuGL::pollEvent()
{
    static MSG msg;
    if (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

/**
 * input & callback registrations
 */
void LuGL::setKeyboardCallback(AppWindow *window, void(*callback)(AppWindow*, KEY_CODE, bool))
{
    window->keyboardCallback = callback;
}

void LuGL::setMouseButtonCallback(AppWindow *window, void(*callback)(AppWindow*, MOUSE_BUTTON, bool, float, float))
{
    window->mouseButtonCallback = callback;
}

void LuGL::setMouseScrollCallback(AppWindow *window, void(*callback)(AppWindow*, float))
{
    window->mouseScrollCallback = callback;
}

void LuGL::setMouseDragCallback(AppWindow *window, void(*callback)(AppWindow*, float, float))
{
    window->mouseDragCallback = callback;
}

bool LuGL::isKeyDown(AppWindow *window, KEY_CODE key)
{
    return window->keys[key];
}

bool LuGL::isMouseButtonDown(AppWindow *window, MOUSE_BUTTON button)
{
    return window->buttons[button];
}

LuGL::Time LuGL::getSystemTime()
{
    SYSTEMTIME lt;
    GetLocalTime(&lt);

    LuGL::Time time;
    time.year = lt.wYear;
    time.month = lt.wMonth;
    time.day_of_week = lt.wDayOfWeek;
    time.day = lt.wDay;
    time.hour = lt.wHour;
    time.minute = lt.wMinute;
    time.second = lt.wSecond;
    time.millisecond = lt.wMilliseconds;

    return time;
}
#endif
