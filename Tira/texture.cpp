//
// Created by Ziyi.Lu 2022/12/23
//

#include <texture.h>
#include <utils.h>
#include <stb_image.h>
#define TINYEXR_IMPLEMENTATION
#define TINYEXR_USE_MINIZ (0)
#define TINYEXR_USE_STB_ZLIB (1)
#include <tinyexr.h>

namespace tira {

    Texture2D::Texture2D(std::string const& path, bool gamma) {
        stbi_set_flip_vertically_on_load(true);
        unsigned char* _data = stbi_load(path.c_str(), &width, &height, &channel, 4);
        channel = 4;

        if (!_data) {
            std::cout << "Error loading image: " << path << "\n";
            return;
        }

        auto size = width * height * channel;
        data = new float[width * height * channel];
        for (int i = 0; i < size; ++i) {
            data[i] = (float)_data[i] / 255;
            if (gamma) {
                data[i] = std::pow(data[i], GAMMA);
            }
        }

        stbi_image_free(_data);
    }

    Texture2D::~Texture2D() {
        if (data) delete[] data;
    }

    colorf Texture2D::at(int x, int y) const {
        x = clamp(x, 0, width - 1);
        y = clamp(y, 0, height - 1);

        auto offset = (y * width + x) * channel;
        if (channel == 1) {
            return {
                data[offset + 0],
            };
        }
        else {
            return {
                data[offset + 0],
                data[offset + 1],
                data[offset + 2],
            };
        }
    }

    colorf Texture2D::sample(float2 const& coords) const {
        // Repeat mode.
        auto u = coords.u - std::floor(coords.u);
        auto v = coords.v - std::floor(coords.v);
        u *= width;
        v *= height;
        auto x = std::floor(u);
        auto y = std::floor(v);
        u -= x;
        v -= y;

        return (at(x + 0, y + 0) * (1 - u) + at(x + 1, y + 0) * u) * (1 - v)
            + (at(x + 0, y + 1) * (1 - u) + at(x + 1, y + 1) * u) * v;
    }

    colorf Texture2D::sample(float3 const& coords) const {
        return { 1, 0, 1 };
    }

    TextureEnv::TextureEnv(std::string const& path) {
        auto ext = path.substr(path.find_last_of('.') + 1);
        if (ext == "exr") {
            char const* err = nullptr;
            float* _data;
            int ret = LoadEXR(&_data, &width, &height, path.c_str(), &err);

            if (ret != TINYEXR_SUCCESS) {
                std::cout << "Error loading EXR image: " << path << "\n";
                if (err) {
                    std::cout << err << "\n";
                    FreeEXRErrorMessage(err);
                }
            }

            channel = 3;
            auto size = width * height * channel;
            data = new float[width * height * channel];
            for (int i = 0; i < width * height; ++i) {
                data[i * 3 + 0] = _data[i * 4 + 0];
                data[i * 3 + 1] = _data[i * 4 + 1];
                data[i * 3 + 2] = _data[i * 4 + 2];
            }

            free(_data);
        }
        else if (ext == "hdr") {
            stbi_set_flip_vertically_on_load(true);
            float* _data = stbi_loadf(path.c_str(), &width, &height, &channel, 3);

            if (!_data) {
                std::cout << "Error loading HDR image: " << path << "\n";
                return;
            }

            auto size = width * height * channel;
            data = new float[width * height * channel];
            for (int i = 0; i < size; ++i) {
                data[i] = _data[i];
            }

            stbi_image_free(_data);
        }
    }

    TextureEnv::~TextureEnv() {
        if (data) delete[] data;
    }

    colorf TextureEnv::at(int x, int y) const {
        x = clamp(x, 0, width - 1);
        y = clamp(y, 0, height - 1);

        auto offset = (y * width + x) * channel;
        if (channel == 1) {
            return {
                data[offset + 0],
            };
        }
        else {
            return {
                data[offset + 0],
                data[offset + 1],
                data[offset + 2],
            };
        }
    }

    colorf TextureEnv::sample(float2 const& coords) const {
        return { 1, 0, 1 };
    }

    static float2 direction_to_equirectangular(float3 const& dir) {
        return {
            std::atan2(dir.z, dir.x) * INV_TWO_PI + .5f,
            std::asin(dir.y) * INV_PI + .5f,
        };
    }

    colorf TextureEnv::sample(float3 const& coords) const {
        auto uv = direction_to_equirectangular(normalize(coords));
        auto u = uv.u;
        auto v = uv.v;
        u *= width;
        v *= height;
        auto x = std::floor(u);
        auto y = std::floor(v);
        u -= x;
        v -= y;

        return (at(x + 0, y + 0) * (1 - u) + at(x + 1, y + 0) * u) * (1 - v)
            + (at(x + 0, y + 1) * (1 - u) + at(x + 1, y + 1) * u) * v;
    }

} // namespace tira
