//
// Created by Ziyi.Lu 2022/12/12
//

#include <integrator.h>

namespace tira {

    Integrator::Integrator() {
        poisson_disk = generate_poisson_dist(POISSON_POINTS_NUM);
    }

    void Integrator::render(Image& image, Scene const& scene, RenderMode render_mode, int spp, SamplingScheme sampling_scheme) {

        float3(*tracer)(Ray&, Scene const&, int, bool&, SamplingScheme);
        switch (render_mode) {
        case RenderMode::Normal:
            tracer = trace_normal; break;
        case RenderMode::Reflect:
            tracer = trace_reflect; break;
        case RenderMode::DiffuseColor:
            tracer = trace_diffuse_color; break;
        case RenderMode::PathTrace:
            tracer = trace_path; break;
        case RenderMode::PathTraceWithAreaSamplingLights:
            tracer = trace_path_with_area_sampling_lights; break;
        }

#ifdef _OPENMP
        std::cout << "OpenMP max threads: " << omp_get_max_threads() << "\n";
#endif
        std::cout << "SPP: " << spp << "\n";

        timer.reset();
        bool direct_light;
        for (int y = 0; y < image.height; ++y) {
#ifdef _OPENMP
#pragma omp parallel for
#endif
            for (int x = 0; x < image.width; ++x) {
                colorf color;
                for (int s = 0; s < spp; ++s) {
                    auto u0 = poisson_disk[s % POISSON_POINTS_NUM];
                    // Pinhole camera model.
                    auto ray = scene.camera.get_ray_pinhole(x, y, image.width, image.height, u0);

                    // // Thin lens camera model.
                    // scene.camera.focus_length = 4.f;
                    // scene.camera.aperature = .1f;
                    // auto u1 = random_float2() - .5f;
                    // auto ray = scene.camera.get_ray_thin_lens(x, y, image.width, image.height, u0, u1);

                    color += (*tracer)(ray, scene, 0, direct_light, sampling_scheme);
                }
                color = reinhard_tone_mapping(color / spp);
                color = gamma_correction(color);
                image.set_pixel(x, y, color);
            }
            timer.update();
            auto used_time = timer.total_time();
            auto estimated_time = used_time * (image.height - y - 1) / (y + 1);
            print_progress(y / (float)image.height, estimated_time);
        }
        print_progress(1.f, 0.f);

        auto elapsed = timer.total_time();
        std::cout << "\nElapsed time: " << elapsed << "s\n";
    }

    void Integrator::render_N_samples(ImageFloat& image, Scene const& scene, RenderMode render_mode, int spp, int integrated_spp, SamplingScheme sampling_scheme) {
        float3(*tracer)(Ray&, Scene const&, int, bool&, SamplingScheme);
        switch (render_mode) {
        case RenderMode::Normal:
            tracer = trace_normal; break;
        case RenderMode::Reflect:
            tracer = trace_reflect; break;
        case RenderMode::DiffuseColor:
            tracer = trace_diffuse_color; break;
        case RenderMode::PathTrace:
            tracer = trace_path; break;
        case RenderMode::PathTraceWithAreaSamplingLights:
            tracer = trace_path_with_area_sampling_lights; break;
        }

        bool direct_light;
        for (int y = 0; y < image.height; ++y) {
#ifdef _OPENMP
#pragma omp parallel for
#endif
            for (int x = 0; x < image.width; ++x) {
                colorf color;
                for (int s = 0; s < spp; ++s) {
                    auto u0 = poisson_disk[s % POISSON_POINTS_NUM];
                    // Pinhole camera model.
                    auto ray = scene.camera.get_ray_pinhole(x, y, image.width, image.height, u0);

                    // // Thin lens camera model.
                    // scene.camera.focus_length = 4.f;
                    // scene.camera.aperature = .1f;
                    // auto u1 = random_float2() .5f;
                    // auto ray = scene.camera.get_ray_thin_lens(x, y, image.width, image.height, u0, u1);

                    color += (*tracer)(ray, scene, 0, direct_light, sampling_scheme);
                }
                color = color / spp;

                auto color_prev = image.color_at(x, y);
                float alpha = (float)spp / (spp + integrated_spp);
                color = color_prev * (1 - alpha) + color * alpha;
                image.set_pixel(x, y, color);
            }
        }
    }

    float3 Integrator::trace_reflect(Ray& ray, Scene const& scene, int bounce, bool& direct_light, SamplingScheme sampling_scheme) {
        if (bounce >= MAX_BOUNCE) return float3::zero();

        Intersection isect;
        scene.intersect(ray, isect);

        if (isect.hit) {
            if (isect.material->emissive) {
                return isect.material->emission;
            }

            auto wi = transform::reflect(ray.direction, isect.normal);
            auto r = Ray(isect.position, wi);
            auto attenuation = std::min(1 / isect.distance * isect.distance, 1.f);
            bool dl;
            return trace_reflect(r, scene, bounce + 1, dl, sampling_scheme) * attenuation;
        }

        return float3::zero();
    }

    float3 Integrator::trace_normal(Ray& ray, Scene const& scene, int bounce, bool& direct_light, SamplingScheme sampling_scheme) {
        Intersection isect;
        scene.intersect(ray, isect);

        if (isect.hit) {
            return isect.normal * .5f + .5f;
        }

        return float3::zero();
    }

    float3 Integrator::trace_diffuse_color(Ray& ray, Scene const& scene, int bounce, bool& direct_light, SamplingScheme sampling_scheme) {
        Intersection isect;
        scene.intersect(ray, isect);

        if (isect.hit) {
            auto mat = reinterpret_cast<BlinnPhongMaterial*>(isect.material);
            if (mat->diffuse_texture) {
                return mat->diffuse_texture->sample(isect.uv);
            }
            else {
                return mat->diffuse;
            }
        }

        return float3::zero();
    }

    float3 Integrator::trace_path(Ray& ray, Scene const& scene, int bounce, bool& direct_light, SamplingScheme sampling_scheme) {
        if (bounce >= MAX_BOUNCE) return float3::zero();

        Intersection isect;
        scene.intersect(ray, isect);

        if (isect.hit) {
            if (isect.material->emissive) {
                return isect.material->emission;
            }

            auto P = isect.position;
            auto N = isect.normal;
            auto wo = -ray.direction;

            float3 wi;
            float pdf;
            bool is_delta;
            switch (sampling_scheme) {
            case SamplingScheme::MonteCarlo:
                isect.material->sample_uniform(wo, N, wi, pdf);
                break;
            case SamplingScheme::Importance:
                isect.material->sample(wo, N, wi, isect.tangent, isect.bitangent, pdf, is_delta);
                break;
            }

            if (pdf < EPSILON) return float3::zero();

            Ray r(P, wi);
            bool dl;
            auto radiance = trace_path(r, scene, bounce + 1, dl, sampling_scheme);
            return radiance
                * isect.material->eval(wo, wi, N, isect.uv, isect.tangent, isect.bitangent)
                * std::max(std::abs(wi.dot(N)), 0.f)
                / pdf;
        }

        return float3::zero();
    }

    float3 Integrator::trace_path_with_area_sampling_lights(Ray& ray, Scene const& scene, int bounce, bool& direct_light, SamplingScheme sampling_scheme) {
        direct_light = false;
        if (bounce >= MAX_BOUNCE) return float3::zero();

        Intersection isect;
        scene.intersect(ray, isect);

        if (isect.hit) {
            if (isect.material->emissive) {
                direct_light = true;
                return isect.material->emission;
            }

            auto L_dir = float3::zero();
            auto L_indir = float3::zero();

            auto P = isect.position;
            auto N = isect.normal;
            auto wo = -ray.direction;
            bool dl;

            //// Indirect lighting.
            float3 wi;
            float pdf;
            bool is_delta;
            switch (sampling_scheme) {
            case SamplingScheme::MonteCarlo:
                isect.material->sample_uniform(wo, N, wi, pdf);
                break;
            case SamplingScheme::Importance:
                isect.material->sample(wo, N, wi, isect.tangent, isect.bitangent, pdf, is_delta);
                break;
            }

            Ray r(P, wi);
            auto radiance = trace_path_with_area_sampling_lights(r, scene, bounce + 1, dl, sampling_scheme);
            if (!dl && is_delta) {
                L_indir = radiance
                    * isect.material->eval(wo, wi, N, isect.uv, isect.tangent, isect.bitangent)
                    * std::max(std::abs(wi.dot(N)), 0.f)
                    / pdf;
            }

            //// Direct lighting.
            Intersection isect_light;
            float pdf_light;
            float3 ws;
            float geom;
            scene.sample_light(P, isect_light, ws, pdf_light, geom);

            if (geom > EPSILON) {

                L_dir = isect_light.material->emission
                    * isect.material->eval(wo, ws, N, isect.uv, isect.tangent, isect.bitangent)
                    * std::max(ws.dot(N), 0.f)
                    * geom
                    / pdf_light;
            }

            return L_dir + L_indir;
        }

        return float3::zero();
    }

} // namespace tira
