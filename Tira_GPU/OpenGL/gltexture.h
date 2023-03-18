#pragma once

#include <vector>
#include <string>

namespace GL {

struct ShaderProgram;

enum struct TextureType {
    None,
    Texture2D,
    RenderTarget,
    Cubemap,
};

struct GLTexture {
    unsigned int handle;
    TextureType type;

    GLTexture();
    ~GLTexture();
    GLTexture(GLTexture const&) = delete;
    GLTexture(GLTexture&& other) noexcept
        : handle(other.handle)
        , type(other.type)
    {
        other.handle = 0;
    }
    auto operator=(GLTexture const&)->GLTexture & = delete;
    auto operator=(GLTexture&& other) noexcept -> GLTexture& {
        handle = other.handle;
        type = other.type;
        other.handle = 0;

        return *this;
    }

    auto bind(unsigned int binding) -> void;
    auto bind(ShaderProgram* shader, std::string const& name, unsigned int binding) -> void;
};

enum struct InternalFormat {
    Default,
    RED,
    RGB,
    RGBA,
    FloatRED,
    FloatRGB,
    FloatRGBA,
    Depth,
};

struct GLTexture2D : public GLTexture {
    GLTexture2D(std::string const& filename, bool hdr = false, bool genMipmap = false, bool flipY = true);
    GLTexture2D(size_t width, size_t height, InternalFormat format, void* data = NULL);
};

struct GLCubemap : public GLTexture {
    GLCubemap(std::vector<std::string> const& filenames, bool genMipmap = true, bool flipY = false);
};

auto saveSnapshot(std::string const& path, int w, int h) -> void;

}
