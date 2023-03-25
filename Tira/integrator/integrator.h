//
// Created by Ziyi.Lu 2023/02/10
//

#ifndef INTEGRATOR_H
#define INTEGRATOR_H

#include <scene/scene.h>
#include <misc/timer.h>

namespace tira {

    struct Integrator {
        Timer timer;
        std::vector<float2> poisson_disk;

        int max_depth = 8;
        bool use_mis = true;
        float russian_roulette = 0.8f;

        Integrator();

        void render(Image& image, Scene const& scene, int spp = 64);
        void render_N_samples(ImageFloat& image, Scene const& scene, int spp = 64, int integrated_spp = 0);

        virtual float3 get_pixel_color(int x, int y, int sample_id, Scene const& scene) = 0;
    };

} // namespace tira

#endif
