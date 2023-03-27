//
// Created by Ziyi.Lu 2023/02/10
//

#include <integrator/montecarlo.h>

#define POISSON_POINTS_NUM 32

namespace tira {

    float3 MonteCarloIntegrator::get_pixel_color(int x, int y, int sample_id, Scene const& scene) {

        float3 L = float3::zero();
        float3 attenuation = float3::one();

        auto const& u = poisson_disk[sample_id % POISSON_POINTS_NUM];
        // Pinhole camera model.
        auto ray = scene.camera.get_ray_pinhole(x, y, scene.scr_w, scene.scr_h, u);

        // // Thin lens camera model.
        // scene.camera.focus_length = 4.f;
        // scene.camera.aperature = .1f;
        // auto u1 = random_float2() - .5f;
        // auto ray = scene.camera.get_ray_thin_lens(x, y, scene.scr_w, scene.scr_h, u0, u1);

        int depth = 0;
#ifdef USE_RUSSIAN_ROULETTE
        while (true) {
#else
        while (depth < max_depth) {
#endif
            Intersection isect;
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

            if (isect.material->emissive) {
                if (depth == 0 || ray.is_delta)
                    if (dot(ray.direction, isect.normal) < 0)
                        L += attenuation * isect.material->emission;
                break;
            }

            float3 wi;
            float3 f = float3::zero();
            float pdf = 0.f;

            if (isect.material->is_delta) {
                isect.material->sample(-ray.direction, isect.normal, wi, isect.tangent, isect.bitangent, pdf, ray.is_delta);
                attenuation = attenuation * isect.material->eval(-ray.direction, wi, isect.normal, isect.uv, isect.tangent, isect.bitangent);

                ray.set_direction(wi);
                float3 offset = dot(wi, isect.normal) > 0 ? isect.normal * rEPSILON : -isect.normal * rEPSILON;
                ray.set_origin(isect.position + offset);
                ray.is_delta = true;
                ++depth;
                continue;
            }

#ifdef USE_RUSSIAN_ROULETTE
            float rnd = random_float();
            if (rnd > RUSSIAN_ROULETTE) break;

            if (scene.lights_total_area > 0)
                L += attenuation * calculate_direct_light(LightType::AreaLights, scene, ray, isect, wi, f, pdf, ray.is_delta) / RUSSIAN_ROULETTE;
            if (scene.sun_enabled)
                L += attenuation * calculate_direct_light(LightType::SunLight, scene, ray, isect, wi, f, pdf, ray.is_delta) / RUSSIAN_ROULETTE;
            if (scene.lights_total_area <= 0 && !scene.sun_enabled) {
                isect.material->sample(-ray.direction, isect.normal, wi, isect.tangent, isect.bitangent, pdf, ray.is_delta);
                f = isect.material->eval(-ray.direction, wi, isect.normal, isect.uv, isect.tangent, isect.bitangent) * std::abs(dot(wi, isect.normal)) / RUSSIAN_ROULETTE;
            }
#else
            if (scene.lights_total_area > 0)
                L += attenuation * calculate_direct_light(LightType::AreaLights, scene, ray, isect, wi, f, pdf, ray.is_delta);
            if (scene.sun_enabled)
                L += attenuation * calculate_direct_light(LightType::SunLight, scene, ray, isect, wi, f, pdf, ray.is_delta);
            if (scene.envmap) {
                L += attenuation * calculate_direct_light(LightType::Envmap, scene, ray, isect, wi, f, pdf, ray.is_delta);
            }
#endif

            if (pdf > EPSILON) {
                attenuation = attenuation * f / pdf;
            }

            ray.set_direction(wi);
            // Avoid seam-like artifacts.
            float3 offset = dot(wi, isect.normal) > 0 ? isect.normal * rEPSILON : -isect.normal * rEPSILON;
            ray.set_origin(isect.position + offset);

            ++depth;
        }

        return L;
    }

    // Heuristics from the paper: Eric Veach et al., Optimally Combining Sampling Techniques for Monte Carlo Rendering
    // n0 and n1 are sample counts
    // pdf0 and pdf1 are distribution functions

    static float balanced_heuristic(float n0, float pdf0, float n1, float pdf1) {
        float f0 = n0 * pdf0;
        float f1 = n1 * pdf1;
        return f0 / (f0 + f1);
    }

    static float cutoff_heuristic(float n0, float pdf0, float n1, float pdf1, float alpha = .1f) {
        float f0 = n0 * pdf0;
        float f1 = n1 * pdf1;
        float fmax = std::max(f0, f1);
        float cutoff = alpha * fmax;
        if (f0 < cutoff) return 0.f;
        else if (f1 < cutoff) return 1.f;
        else return f0 / (f0 + f1);
    }

    static float power_heuristic(float n0, float pdf0, float n1, float pdf1) {
        float f0 = n0 * pdf0;
        float f1 = n1 * pdf1;
        return (f0 * f0) / (f0 * f0 + f1 * f1);
    }

    static float maximum_heuristic(float n0, float pdf0, float n1, float pdf1) {
        float f0 = n0 * pdf0;
        float f1 = n1 * pdf1;
        return f0 > f1 ? 1.f : 0.f;
    }

    float3 MonteCarloIntegrator::calculate_direct_light(LightType type, Scene const& scene, Ray const& ray, Intersection const& isect, float3 & wi, float3 & bsdf, float& pdf, bool& is_delta) {
        is_delta = false;
        float3 wo = -ray.direction;
        float3 Ld = float3::zero();
        float light_pdf = 0.f;
        float geom = 1.f;
        pdf = 0.f;
        bool is_black = false;
        // MIS weight
        float weight = 1.f;

        Intersection light_isect;
        float3 Li = float3::zero();
        switch (type) {
        case LightType::AreaLights:
            // Sampling the lights ---- area sampling strategy.
            scene.sample_light(isect.position, light_isect, wi, light_pdf, geom);
            if (!scene.directional_area_light || dot(wi, -light_isect.normal) > (1.0 - scene.directional_area_light_solid_angle))
                Li = light_isect.material->emission;
            break;
        case LightType::SunLight:
            Li = scene.sample_sun(isect.position, isect.normal, wi, light_pdf, geom);
            break;
        case LightType::Envmap:
            Li = scene.sample_envmap(isect.position, isect.normal, wi, light_pdf, geom) * scene.envmap_scale;
            break;
        }

        is_black = dot(Li, Li) < EPSILON;

        if (light_pdf > EPSILON && !is_black) {
            float3 f = isect.material->eval(wo, wi, isect.normal, isect.uv, isect.tangent, isect.bitangent) * std::abs(dot(wi, isect.normal));

            if (use_mis) {
                pdf = isect.material->pdf(wo, wi, isect.tangent, isect.bitangent, isect.normal);
                weight = power_heuristic(1.f, light_pdf, 1.f, pdf);
            }
            is_black = dot(f, f) < EPSILON;
            if (!is_black) {
                Ld += Li * f * geom * weight / light_pdf;
            }
        }

        if (use_mis) {
            // Sampling the BRDF ---- directional sampling strategy.
            isect.material->sample(wo, isect.normal, wi, isect.tangent, isect.bitangent, pdf, is_delta);
            bsdf = isect.material->eval(wo, wi, isect.normal, isect.uv, isect.tangent, isect.bitangent);
            // Fix specular object with silhouette edge.
            if (!is_delta) {
                bsdf = bsdf * std::abs(dot(wi, isect.normal));
            }

            is_black = dot(bsdf, bsdf) < EPSILON;
            if (!is_black && pdf > EPSILON) {

                float3 P = isect.position;
                {
                    Intersection isect;
                    Ray ray(P, wi);
                    ray.shadow_ray = true;
                    scene.intersect(ray, isect);
                    is_black = true;
                    switch (type) {
                    case LightType::AreaLights:
                        if (isect.hit && isect.material->emissive && dot(wi, isect.normal) < 0) {
                            if (!scene.directional_area_light || dot(wi, -isect.normal) > (1.0 - scene.directional_area_light_solid_angle))
                                Li = isect.material->emission;
                            light_pdf = 1 / scene.lights_total_area;
                            is_black = false;
                        }
                        break;
                    case LightType::SunLight:
                        if (!isect.hit && scene.hit_sun(wi)) {
                            light_pdf = 1 / scene.sun_solid_angle;
                            Li = scene.sun_radiance;
                            is_black = false;
                        }
                        break;
                    case LightType::Envmap:
                        if (!isect.hit) {
                            light_pdf = INV_TWO_PI;
                            Li = scene.envmap->sample(wi) * scene.envmap_scale;
                            is_black = false;
                        }
                        break;
                    }
                }

                weight = power_heuristic(1.f, pdf, 1.f, light_pdf);

                if (!is_black) {
                    Ld += Li * bsdf * weight / pdf;
                }
            }
        }

        isect.material->sample(wo, isect.normal, wi, isect.tangent, isect.bitangent, pdf, is_delta);
        bsdf = isect.material->eval(wo, wi, isect.normal, isect.uv, isect.tangent, isect.bitangent);
        if (!is_delta)
            bsdf = bsdf * std::abs(dot(wi, isect.normal));

        return Ld;
    }

} // namespace tira

