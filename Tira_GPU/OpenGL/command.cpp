#include <glad/glad.h>
#include <OpenGL/command.h>
#include <iostream>

namespace GL {

auto CommandList::invoke() const noexcept -> void {
    for (auto& cmd : commands) {
        cmd();
    }
}

auto CommandList::clear() noexcept -> void {
    commands.clear();
}

auto CommandList::emplace(CommandType&& cmd) noexcept -> void {
    commands.emplace_back(std::move(cmd));
}

auto CommandList::EnableDepthTest(bool value) noexcept -> CommandType {
    return [=]() {
        if (value) {
            glEnable(GL_DEPTH_TEST);
        }
        else {
            glDisable(GL_DEPTH_TEST);
        }
    };
}

auto CommandList::ClearScreenColor(float r, float g, float b, float a) noexcept -> CommandType {
    return [=]() {
        glClearColor(r, g, b, a);
        glClear(GL_COLOR_BUFFER_BIT);
    };
}

auto CommandList::ClearScreenColorAndDepthBuffer(float r, float g, float b, float a) noexcept -> CommandType {
    return [=]() {
        glClearColor(r, g, b, a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    };
}

auto CommandList::SetVieportSize(size_t w, size_t h) noexcept -> CommandType {
    return [=]() {
        glViewport(0, 0, w, h);
    };
}

}
