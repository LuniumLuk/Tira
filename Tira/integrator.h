//
// Created by Ziyi.Lu 2022/12/08
//

#ifndef INTEGRATOR_H
#define INTEGRATOR_H

#include <scene.h>
#include <timer.h>

namespace tira {

    struct Integrator {

        enum struct SamplingScheme {
            MonteCarlo,
            Importance,
        };

        enum struct RenderMode {
            Normal,
            Reflect,
            DiffuseColor,
            PathTrace,
            PathTraceWithAreaSamplingLights,
        };

        Timer timer;
        static float russian_roulette;
        static int max_bounce;
        SamplingScheme sampling_scheme;
        std::vector<float2> poisson_disk;

        Integrator();
        ~Integrator() {}

        void render(Image& image, Scene const& scene, RenderMode render_mode,
            int spp = 64,
            SamplingScheme sampling_scheme = SamplingScheme::Importance);
        void render_N_samples(ImageFloat& image, Scene const& scene, RenderMode render_mode,
            int spp = 64,
            int integrated_spp = 0,
            SamplingScheme sampling_scheme = SamplingScheme::Importance);

        static float3 trace_normal(Ray& ray, Scene const& scene, int bounce, bool& direct_light, SamplingScheme sampling_scheme);
        static float3 trace_reflect(Ray& ray, Scene const& scene, int bounce, bool& direct_light, SamplingScheme sampling_scheme);
        // Deprecate, do not call this function unless you are sure that you loaded BlinnPhongMaterial.
        static float3 trace_diffuse_color(Ray& ray, Scene const& scene, int bounce, bool& direct_light, SamplingScheme sampling_scheme);
        static float3 trace_path(Ray& ray, Scene const& scene, int bounce, bool& direct_light, SamplingScheme sampling_scheme);
        static float3 trace_path_with_area_sampling_lights(Ray& ray, Scene const& scene, int bounce, bool& direct_light, SamplingScheme sampling_scheme);
    };

} // namespace tira

#endif
