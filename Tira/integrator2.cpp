//
// Created by Ziyi.Lu 2023/02/10
//

#include <integrator2.h>

#define POISSON_POINTS_NUM 32

namespace tira {

    Integrator2::Integrator2() {
        poisson_disk = generate_poisson_dist(POISSON_POINTS_NUM);
    }

    void Integrator2::render(Image& image, Scene const& scene, int spp) {

#ifdef _OPENMP
        std::cout << "OpenMP max threads: " << omp_get_max_threads() << "\n";
#endif
        std::cout << "SPP: " << spp << " Width: " << scene.scr_w << " Height: " << scene.scr_h << "\n";

        ImageFloat buffer(scene.scr_w, scene.scr_h);

        timer.reset();
        bool direct_light;
        for (int s = 0; s < spp; ++s) {
            for (int y = 0; y < image.height; ++y) {
#ifdef _OPENMP
#pragma omp parallel for
#endif
                for (int x = 0; x < image.width; ++x) {
                    auto color = get_pixel_color(x, y, s, scene);
                    if (isfinite(color)) {
                        buffer.increment_pixel(x, y, color);
                    }
                }
            }
            timer.update();
            print_progress2(s, spp, timer.delta_time(), timer.total_time());
        }
        print_progress2(spp - 1, spp, timer.delta_time(), timer.total_time());
        auto elapsed = timer.total_time();
        std::cout << "\nTotal time: " << elapsed << "s\n";

        for (int y = 0; y < image.height; ++y) for (int x = 0; x < image.width; ++x) {
            auto color = buffer.color_at(x, y);
            // color = reinhard_tone_mapping(color / spp);
            // color = ACES_tone_mapping(color / spp);
            color = gamma_correction(color / spp);
            color = saturate(color);
            image.set_pixel(x, y, color);
        }
    }

    void Integrator2::render_N_samples(ImageFloat& image, Scene const& scene, int spp, int integrated_spp) {
        for (int y = 0; y < image.height; ++y) {
#ifdef _OPENMP
#pragma omp parallel for
#endif
            for (int x = 0; x < image.width; ++x) {
                colorf color;
                for (int s = 0; s < spp; ++s) {
                    color += get_pixel_color(x, y, s, scene);
                }
                color = color / spp;

                auto color_prev = image.color_at(x, y);
                float alpha = (float)spp / (spp + integrated_spp);
                color = color_prev * (1 - alpha) + color * alpha;
                image.set_pixel(x, y, color);
            }
        }
    }

    float3 Integrator2::get_pixel_color(int x, int y, int sample_id, Scene const& scene) {

        float3 L = float3::zero();
        float3 attenuation = float3::one();

        auto u0 = poisson_disk[sample_id % POISSON_POINTS_NUM];
        // Pinhole camera model.
        auto ray = scene.camera.get_ray_pinhole(x, y, scene.scr_w, scene.scr_h, u0);

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
                if (scene.envmap) {
                    L += attenuation * scene.envmap->sample(ray.direction);
                }
                break;
            }

            if (isect.material->emissive) {
                if (depth == 0 || ray.is_delta) L += attenuation * isect.material->emission;
                break;
            }

            float3 wi;
            float3 f = float3::zero();
            float pdf = 0.f;

            if (isect.material->is_delta) {
                isect.material->sample(-ray.direction, isect.normal, wi, isect.tangent, isect.bitangent, pdf, ray.is_delta);
                attenuation = attenuation * isect.material->eval(-ray.direction, wi, isect.normal, isect.uv, isect.tangent, isect.bitangent);

                ray.set_direction(wi);
                ray.set_origin(isect.position + wi * rEPSILON);
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

            if (pdf > EPSILON && dot(f, f) > EPSILON) {
                attenuation = attenuation * f / pdf;
            }

            ray.set_direction(wi);
            // Avoid seam-like artifacts.
            ray.set_origin(isect.position + wi * rEPSILON);

            ++depth;
            // if (depth >= 4) {
            //     if (random_float() < attenuation.max_component()) {
            //         attenuation = attenuation / attenuation.max_component();
            //     }
            //     else {
            //         break;
            //     }
            // }
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

    float3 Integrator2::calculate_direct_light(LightType type, Scene const& scene, Ray const& ray, Intersection const& isect, float3 & wi, float3 & bsdf, float& pdf, bool& is_delta) {
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
        float3 Li;
        switch (type) {
        case LightType::AreaLights:
            // Sampling the lights ---- area sampling strategy.
            scene.sample_light(isect.position, light_isect, wi, light_pdf, geom);
            Li = light_isect.material->emission;
            break;
        case LightType::SunLight:
            Li = scene.sample_sun(isect.position, isect.normal, wi, light_pdf, geom);
            break;
        case LightType::Envmap:
            Li = scene.sample_envmap(isect.position, isect.normal, wi, light_pdf, geom);
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
                    if (isect.hit) {
                        if (isect.material->emissive) {
                            // Calculate light pdf.
                            switch (type) {
                            case LightType::AreaLights: light_pdf = 1 / scene.lights_total_area; break;
                            case LightType::SunLight: light_pdf = 1 / scene.sun_solid_angle; break;
                            case LightType::Envmap: light_pdf = INV_TWO_PI; break;
                            }

                            Li = isect.material->emission;
                        }
                        else {
                            is_black = true;
                        }
                    }
                    else if (scene.envmap) {
                        light_pdf = INV_TWO_PI;
                        Li = scene.envmap->sample(wi);
                    }
                    else {
                        is_black = true;
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

