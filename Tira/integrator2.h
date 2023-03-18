//
// Created by Ziyi.Lu 2023/02/10
//

#ifndef INTEGRATOR2_H
#define INTEGRATOR2_H

#include <scene.h>
#include <timer.h>

namespace tira {

    struct Integrator2 {
        enum struct LightPathOption {
            MaxDepth,
            RussianRoulette,
        };

        enum struct LightType {
            AreaLights,
            SunLight,
            Envmap,
        };

        LightPathOption path_option = LightPathOption::MaxDepth;

        int max_depth = 8;
        bool use_mis = true;

        Timer timer;
        std::vector<float2> poisson_disk;

        Integrator2();
        ~Integrator2() {}

        void render(Image& image, Scene const& scene, int spp = 64);
        void render_N_samples(ImageFloat& image, Scene const& scene, int spp = 64, int integrated_spp = 0);

        float3 get_pixel_color(int x, int y, int sample_id, Scene const& scene);
        float3 calculate_direct_light(LightType type, Scene const& scene, Ray const& ray, Intersection const& isect, float3& wi, float3& bsdf, float& pdf, bool& is_delta);
    };

} // namespace tira

#endif
