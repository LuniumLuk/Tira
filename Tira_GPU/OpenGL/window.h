#pragma once

#if defined(_MSC_VER)
// 'APIENTRY': macro redefinition
#pragma warning(disable : 4005)
#endif

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <OpenGL/command.h>
#include <string>

namespace GL {

#define CHECK_ERROR() Graphics::Core::_glCheckError(__FILE__, __LINE__)

auto _glCheckError(const char* file, int line) -> void;

enum struct SamplingScheme {
    Default,
    GLFW2xMSAA,
    GLFW4xMSAA,
    GLFW8xMSAA,
};

struct WindowDesc {
    std::string name = "Graphics";
    size_t width  = 720;
    size_t height = 720;
    bool fullScreen = false;
    bool debug = true;
    bool resizable = true;
    SamplingScheme sampling = SamplingScheme::Default;
};

struct Window
{
    Window(WindowDesc const& desc);
    ~Window();

    auto preframe() noexcept -> void;
    auto update() noexcept -> bool;
    auto endframe() noexcept -> void;
    auto queryInputPressed(int i) noexcept -> bool;
    auto queryMouseButtonPressed(int i) noexcept -> bool;
    auto queryMousePosition(double* xpos, double* ypos) noexcept -> void;
    auto queryMouseScroll(double* xoffset, double* yoffset) noexcept -> void;
    auto hideCursor(bool hide) noexcept -> void;

    GLFWwindow* handle = nullptr;
    size_t width, height;
    EventList<size_t, size_t> windowResizeEvent;

    double mMouseScrollX = 0.0;
    double mMouseScrollY = 0.0;
};

}
