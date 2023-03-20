#pragma once

#if defined(_MSC_VER)
// type conversion warning
#pragma warning(disable : 4267)
#endif

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

struct Texture {
    unsigned int handle;
    TextureType type;

    Texture();
    ~Texture();
    Texture(Texture const&) = delete;
    Texture(Texture&& other) noexcept
        : handle(other.handle)
        , type(other.type)
    {
        other.handle = 0;
    }
    auto operator=(Texture const&)->Texture & = delete;
    auto operator=(Texture&& other) noexcept -> Texture& {
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

struct Texture2D : public Texture {
    Texture2D(std::string const& filename, bool hdr = false, bool genMipmap = false, bool flipY = true);
    Texture2D(size_t width, size_t height, InternalFormat format, void* data = NULL);
};

struct Cubemap : public Texture {
    Cubemap(std::vector<std::string> const& filenames, bool genMipmap = true, bool flipY = false);
};

auto saveSnapshot(std::string const& path, int w, int h) -> void;

}
