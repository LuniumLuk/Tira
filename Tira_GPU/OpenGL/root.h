#pragma once

#include <string>

namespace GL {

struct AssetManager;
struct CommandList;

struct WindowDesc;
struct Timer;
struct Window;

using Filepath = std::string;

struct Root {
    Root();
    Root(WindowDesc desc);
    ~Root();

    Timer* timer = nullptr;
    Window* window = nullptr;
    AssetManager* assetManager = nullptr;
    CommandList* commandList = nullptr;

    Filepath shaderRoot = "./opengl/shader";
    Filepath assetRoot = "./opengl/asset";
    Filepath projRoot = ".";

    static inline auto get() noexcept -> Root* { return root; }
    static auto startFrame() noexcept -> bool;
    static auto endFrame() noexcept -> bool;

private:
    static Root* root;
};

}
