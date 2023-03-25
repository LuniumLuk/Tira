//
// Created by Ziyi.Lu 2023/03/20
//

#ifndef MONTECARLO_H
#define MONTECARLO_H

#include <integrator/integrator.h>

namespace tira {

    struct MonteCarloIntegrator : Integrator {
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

        virtual float3 get_pixel_color(int x, int y, int sample_id, Scene const& scene) override;
        float3 calculate_direct_light(LightType type, Scene const& scene, Ray const& ray, Intersection const& isect, float3& wi, float3& bsdf, float& pdf, bool& is_delta);
    };

} // namespace tira

#endif
