#pragma once

#if defined(_MSC_VER)
// type conversion warning
#pragma warning(disable : 4244)
#pragma warning(disable : 4267)
#pragma warning(disable : 4305)
// sscanf warning
#pragma warning(disable : 4996)
#endif

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
