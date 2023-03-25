//
// Created by Ziyi.Lu 2022/12/08
//

#ifndef WHITTED_H
#define WHITTED_H

#include <integrator/integrator.h>

namespace tira {

    struct WhittedIntegrator : Integrator {
        virtual float3 get_pixel_color(int x, int y, int sample_id, Scene const& scene) override;
    };

} // namespace tira

#endif
