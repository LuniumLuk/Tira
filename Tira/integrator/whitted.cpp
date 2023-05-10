//
// Created by Ziyi.Lu 2022/12/12
//

#include <integrator/whitted.h>

namespace tira {

    float3 WhittedIntegrator::get_pixel_color(int x, int y, int sample_id, Scene const& scene) {
        float3 L = float3::zero();
        float3 attenuation = float3::one();

        auto const& u0 = poisson_disk[sample_id % POISSON_POINTS_NUM];
        auto const& u1 = concentric_sample_dist(random_float2());

        auto ray = scene.camera.get_ray(x, y, scene.scr_w, scene.scr_h, u0, u1);

        Intersection isect, light_isect;
        float3 Li;
        float3 wi, wo, f;
        float pdf;
        float visibility;
        bool is_black;

        for (int depth = 0; depth < max_depth; ++depth) {
            scene.intersect(ray, isect);

            if (!isect.hit) {
                if (scene.sun_enabled && scene.hit_sun(ray.direction)) {
                    L += attenuation * scene.sun_radiance;
                }
                if (scene.envmap) {
                    L += attenuation * scene.envmap->sample(ray.direction) * scene.envmap_scale;
                }
                break;
            }

            wo = -ray.direction;

            if (isect.material->is_delta) {
                isect.material->sample(wo, isect.normal, wi, isect.tangent, isect.bitangent, pdf, ray.is_delta);
                attenuation = attenuation * isect.material->eval(wo, wi, isect.normal, isect.uv, isect.tangent, isect.bitangent);

                ray.set_direction(wi);
                float3 offset = dot(wi, isect.normal) > 0 ? isect.normal * rEPSILON : -isect.normal * rEPSILON;
                ray.set_origin(isect.position + offset);
                ray.is_delta = true;
                continue;
            }

            if (isect.material->emissive) {
                if (depth == 0 || ray.is_delta) L += attenuation * isect.material->emission;
                break;
            }

            if (scene.lights_total_area > 0) {
                scene.sample_light(isect.position, light_isect, wi, pdf, visibility);
                Li = light_isect.material->emission;
                is_black = dot(Li, Li) < EPSILON;
                if (!is_black && pdf > EPSILON) {
                    f = isect.material->eval(wo, wi, isect.normal, isect.uv, isect.tangent, isect.bitangent);
                    L += attenuation * f * Li * visibility * std::abs(dot(wi, isect.normal)) / pdf;
                }
            }

            if (scene.sun_enabled) {
                Li = scene.sample_sun(isect.position, isect.normal, wi, pdf, visibility);
                is_black = dot(Li, Li) < EPSILON;
                if (!is_black && pdf > EPSILON) {
                    f = isect.material->eval(wo, wi, isect.normal, isect.uv, isect.tangent, isect.bitangent);
                    L += attenuation * f * Li * visibility * std::abs(dot(wi, isect.normal)) / pdf;
                }
            }

            if (scene.envmap) {
                Li = scene.sample_envmap(isect.position, isect.normal, wi, pdf, visibility) * scene.envmap_scale;
                is_black = dot(Li, Li) < EPSILON;
                if (!is_black && pdf > EPSILON) {
                    f = isect.material->eval(wo, wi, isect.normal, isect.uv, isect.tangent, isect.bitangent);
                    L += attenuation * f * Li * visibility * std::abs(dot(wi, isect.normal)) / pdf;
                }
            }

            isect.material->sample(wo, isect.normal, wi, isect.tangent, isect.bitangent, pdf, ray.is_delta);
            f = isect.material->eval(wo, wi, isect.normal, isect.uv, isect.tangent, isect.bitangent);
            if (!ray.is_delta) {
                f = f * std::abs(dot(wi, isect.normal));
            }

            if (pdf > EPSILON) {
                attenuation = attenuation * f / pdf;
            }

            ray.set_direction(wi);
            float3 offset = dot(wi, isect.normal) > 0 ? isect.normal * rEPSILON : -isect.normal * rEPSILON;
            ray.set_origin(isect.position + offset);
        }
        return L;
    }

} // namespace tira
