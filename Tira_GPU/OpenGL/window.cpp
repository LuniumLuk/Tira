#include <OpenGL/window.h>
#include <OpenGL/root.h>
#include <OpenGL/gui.h>
#ifndef _WINDOWS_
#include <Windows.h>
#endif
#include <iostream>

namespace GL {

auto _glCheckError(const char* file, int line) -> void {
    GLenum errorCode;
    while ((errorCode = glGetError()) != GL_NO_ERROR) {
        std::string error;
        switch (errorCode) {
        case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
        case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
        case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
        case GL_STACK_OVERFLOW:                error = "STACK_OVERFLOW"; break;
        case GL_STACK_UNDERFLOW:               error = "STACK_UNDERFLOW"; break;
        case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
        case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
        }
        std::cout << error << " | " << file << " (" << line << ")" << std::endl;
    }
}

auto resizeCallback(GLFWwindow* window, int width, int height) -> void
{
    Root::get()->window->windowResizeEvent.emit((size_t)width, (size_t)height);

    Root::get()->window->width = width;
    Root::get()->window->height = height;
}

auto APIENTRY glDebugOutput(GLenum source,
    GLenum type,
    unsigned int id,
    GLenum severity,
    GLsizei length,
    const char* message,
    const void* userParam) -> void {
    // ignore non-significant error/warning codes
    if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

    std::cout << "---------------" << std::endl;
    std::cout << "Debug message (" << id << "): " << message << std::endl;

    switch (source) {
    case GL_DEBUG_SOURCE_API:             std::cout << "Source: API"; break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   std::cout << "Source: Window System"; break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cout << "Source: Shader Compiler"; break;
    case GL_DEBUG_SOURCE_THIRD_PARTY:     std::cout << "Source: Third Party"; break;
    case GL_DEBUG_SOURCE_APPLICATION:     std::cout << "Source: Application"; break;
    case GL_DEBUG_SOURCE_OTHER:           std::cout << "Source: Other"; break;
    } std::cout << std::endl;

    switch (type) {
    case GL_DEBUG_TYPE_ERROR:               std::cout << "Type: Error"; break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cout << "Type: Deprecated Behaviour"; break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  std::cout << "Type: Undefined Behaviour"; break;
    case GL_DEBUG_TYPE_PORTABILITY:         std::cout << "Type: Portability"; break;
    case GL_DEBUG_TYPE_PERFORMANCE:         std::cout << "Type: Performance"; break;
    case GL_DEBUG_TYPE_MARKER:              std::cout << "Type: Marker"; break;
    case GL_DEBUG_TYPE_PUSH_GROUP:          std::cout << "Type: Push Group"; break;
    case GL_DEBUG_TYPE_POP_GROUP:           std::cout << "Type: Pop Group"; break;
    case GL_DEBUG_TYPE_OTHER:               std::cout << "Type: Other"; break;
    } std::cout << std::endl;

    switch (severity) {
    case GL_DEBUG_SEVERITY_HIGH:         std::cout << "Severity: high"; break;
    case GL_DEBUG_SEVERITY_MEDIUM:       std::cout << "Severity: medium"; break;
    case GL_DEBUG_SEVERITY_LOW:          std::cout << "Severity: low"; break;
    case GL_DEBUG_SEVERITY_NOTIFICATION: std::cout << "Severity: notification"; break;
    } std::cout << std::endl;
    std::cout << std::endl;
}

auto scrollCallback(GLFWwindow* window, double xoffset, double yoffset) -> void
{
    Root::get()->window->mMouseScrollX = xoffset;
    Root::get()->window->mMouseScrollY = yoffset;
}

auto processInput(GLFWwindow* window) -> void
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
        return;
    }
}

Window::Window(WindowDesc const& desc) {
    WindowDesc descriptor = desc;
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, desc.resizable);

    switch (desc.sampling) {
    case SamplingScheme::Default:
        glfwWindowHint(GLFW_SAMPLES, 1); break;
    case SamplingScheme::GLFW2xMSAA:
        glfwWindowHint(GLFW_SAMPLES, 2); break;
    case SamplingScheme::GLFW4xMSAA:
        glfwWindowHint(GLFW_SAMPLES, 4); break;
    case SamplingScheme::GLFW8xMSAA:
        glfwWindowHint(GLFW_SAMPLES, 8); break;
    }

    if (descriptor.fullScreen) {
        glfwWindowHint(GLFW_DECORATED, false);
        descriptor.width = GetSystemMetrics(SM_CXSCREEN);
        descriptor.height = GetSystemMetrics(SM_CYSCREEN);
    }
    handle = glfwCreateWindow(descriptor.width, descriptor.height, descriptor.name.c_str(), NULL, NULL);
    if (descriptor.fullScreen) {
        glfwSetWindowPos(handle, 0, 0);
    }
    else {
        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);
        glfwSetWindowPos(handle, screenWidth / 2 - desc.width / 2, screenHeight / 2 - desc.height / 2);
    }
    width = descriptor.width;
    height = descriptor.height;
    if (handle == NULL) {
        glfwTerminate();
    }
    glfwMakeContextCurrent(handle);
    gladLoadGL();
    glfwSetFramebufferSizeCallback(handle, resizeCallback);
    glfwSetScrollCallback(handle, scrollCallback);

    glfwSwapInterval(0);

    ImGuiInit(handle);

    if (desc.debug) {
        int flags;
        glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
        if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
            std::cout << "GLFW debug output successfully initialized!" << std::endl;
            glEnable(GL_DEBUG_OUTPUT);
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
            glDebugMessageCallback(glDebugOutput, nullptr);
            glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
            glDebugMessageControl(GL_DEBUG_SOURCE_API,
                GL_DEBUG_TYPE_PERFORMANCE,
                GL_DONT_CARE,
                0, nullptr, GL_FALSE);
        }
    }

    windowResizeEvent.connect([](size_t x, size_t y)->void {
        CommandList::SetVieportSize(x, y)();
    });
}

Window::~Window() {
    ImGuiShutdown();
    glfwTerminate();
}

auto Window::preframe() noexcept -> void {
    ImGuiPreFrame();
}

auto Window::update() noexcept -> bool {
    if (glfwWindowShouldClose(handle)) return false;
    processInput(handle);
    return true;
}

auto Window::endframe() noexcept -> void {
    ImGuiEndFrame();
    glfwSwapBuffers(handle);

    mMouseScrollX = 0.0;
    mMouseScrollY = 0.0;

    glfwPollEvents();
}

auto Window::queryInputPressed(int i) noexcept -> bool {
    return glfwGetKey(handle, i) == GLFW_PRESS;
}

auto Window::queryMouseButtonPressed(int i) noexcept -> bool {
    return glfwGetMouseButton(handle, i) == GLFW_PRESS;
}

auto Window::queryMousePosition(double* xpos, double* ypos) noexcept -> void {
    glfwGetCursorPos(handle, xpos, ypos);
}

auto Window::queryMouseScroll(double* xoffset, double* yoffset) noexcept -> void {
    *xoffset = mMouseScrollX;
    *yoffset = mMouseScrollY;
}

auto Window::hideCursor(bool hide) noexcept -> void {
    if (hide) {
        glfwSetInputMode(handle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
    else {
        glfwSetInputMode(handle, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}

}
