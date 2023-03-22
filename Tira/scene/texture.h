//
// Created by Ziyi.Lu 2022/12/23
//

#ifndef TEXTURE_H
#define TEXTURE_H

#include <misc/utils.h>

namespace tira {

    struct Texture {
        virtual ~Texture() {};

        virtual colorf sample(float2 const& coords) const = 0;
        virtual colorf sample(float3 const& coords) const = 0;
    };

    struct Texture2D : Texture {
        float* data = nullptr;
        int width = 0;
        int height = 0;
        int channel = 0;

        Texture2D() = delete;
        Texture2D(std::string const& path, bool gamma = false);
        virtual ~Texture2D();

        Texture2D(Texture2D const&) = delete;
        Texture2D(Texture2D&&) = delete;

        colorf at(int x, int y) const;
        virtual colorf sample(float2 const& coords) const override;
        virtual colorf sample(float3 const& coords) const override;
    };

    struct TextureEnv : Texture {
        float* data = nullptr;
        int width = 0;
        int height = 0;
        int channel = 0;
        // weights are in grid of spherical coordinates
        float* weight = nullptr;
        int weight_grid_size = 16;

        TextureEnv() = delete;
        TextureEnv(std::string const& path);
        virtual ~TextureEnv();

        TextureEnv(TextureEnv const&) = delete;
        TextureEnv(TextureEnv&&) = delete;

        void calc_weights(int num_samples = 512);
        colorf at(int x, int y) const;
        virtual colorf sample(float2 const& coords) const override;
        virtual colorf sample(float3 const& coords) const override;
    };

} // namespace tira

#endif