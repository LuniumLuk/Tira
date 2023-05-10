//
// Created by Ziyi.Lu 2023/03/20
//

#include <integrator/bidirectional.h>

#define POISSON_POINTS_NUM 32

namespace tira {

    float3 BidirectionalIntegrator::get_pixel_color(int x, int y, int sample_id, Scene const& scene) {
        // [TODO] Use Spectrum struct to represent light.
        auto const& u0 = poisson_disk[sample_id % POISSON_POINTS_NUM];
        auto const& u1 = concentric_sample_dist(random_float2());

        Ray camera_ray = scene.camera.get_ray(x, y, scene.scr_w, scene.scr_h, u0, u1);
        camera_ray.depth = max_depth;

        // MIS weights and L per depth.
        std::vector<float> mis_weights(max_depth, 0.0f);
        std::vector<float3> Ls(max_depth, float3::zero());
        for (int i = 0; i < NUM_LIGHT_SAMPLES; ++i) {
            float3 Le;
            float light_pdf;
            Ray light_ray = scene.sample_light_ray(Le, light_pdf);
            light_ray.depth = max_depth;
            Le = Le / light_pdf;
            render_paths(camera_ray, light_ray, Le, scene, mis_weights, Ls);
        }

        float3 L = float3::zero();
        for (int d = 0; d < max_depth; ++d) {
            if (mis_weights[d] > EPSILON)
                L += Ls[d] / mis_weights[d];
        }
        return L;
    }

    void BidirectionalIntegrator::render_paths(Ray const& camera_ray, Ray const& ligth_ray, float3 const& Le, Scene const& scene, std::vector<float>& mis_weights, std::vector<float3>& Ls) {
        std::vector<VertexInfo> camera_path;
        std::vector<VertexInfo> light_path;
        float totalWeight = 0.0f;

        generate_path(camera_ray, camera_path, scene, PathType::Camera);
        generate_path(ligth_ray, light_path, scene, PathType::Light);

        int n_camera = camera_path.size();
        int n_light = light_path.size();

        for (size_t t = 1; t <= n_camera; ++t) {
            for (size_t s = 0; s <= n_light; ++s) {
                size_t depth = t + s;
                if (depth > max_depth)
                    continue;

                float pdf = 0.0f;
                float3 L = eval_path(scene, camera_path, light_path, Le, t, s, pdf);

                float w = pdf * pdf;
                Ls[t - 1] += L * w;
                mis_weights[t - 1] += w;
            }
        }
    }

    float3 BidirectionalIntegrator::eval_path(Scene const& scene, std::vector<VertexInfo> const& camera_path, std::vector<VertexInfo> const& light_path, float3 const& Le, size_t t, size_t s, float& pdf) {
        pdf = 0.0f;

        // Handle camera path terminate at light object.
        {
            auto const& v = camera_path[t - 1];
            if (v.material->emissive) {
                if (dot(v.wo, v.normal) > 0)
                    pdf = v.pdf;
                    return v.material->emission * v.attenuation;
            }
        }

        if (s == 0) {
            auto const& v = camera_path[t - 1];
            pdf = v.pdf;
            if (v.material->is_delta) {
                Intersection isect;
                float _pdf;
                bool is_delta;
                float3 wi;
                v.material->sample(v.wo, v.normal, wi, v.tangent, v.bitangent, _pdf, is_delta);
                Ray ray(v.position, wi);
                scene.intersect(ray, isect);
                if (isect.hit && isect.material->emissive && dot(wi, isect.normal) < 0) {
                    if (!scene.directional_area_light || std::abs(dot(wi, -isect.normal) > (1.0 - scene.directional_area_light_solid_angle)))
                        return isect.material->emission * v.attenuation * v.material->eval(v.wo, wi, v.normal, v.uv, v.tangent, v.bitangent);
                }
            }
            else {
                Intersection light_isect;
                float3 wi;
                float light_pdf;
                float geom;
                float3 Li = float3::zero();
                scene.sample_light(v.position, light_isect, wi, light_pdf, geom);
                if (!scene.directional_area_light || std::abs(dot(wi, -light_isect.normal) > (1.0 - scene.directional_area_light_solid_angle)))
                    Li = light_isect.material->emission;

                if (light_pdf > EPSILON) {
                    return Li * v.attenuation * v.material->eval(v.wo, wi, v.normal, v.uv, v.tangent, v.bitangent) * geom * std::abs(dot(wi, v.normal)) / light_pdf;
                }
            }
        }
        else {
            auto const& vl = light_path[s - 1];
            auto const& vc = camera_path[t - 1];
            if (vc.material->is_delta) return float3::zero();

            float geom;

            float3 L_dir;
            float3 L_indir;

            float3 f = vl.attenuation * vl.material->eval(normalize(vc.position - vl.position), vl.wi, vl.normal, vl.uv, vl.tangent, vl.bitangent)
                     * vc.attenuation * vc.material->eval(vc.wo, normalize(vl.position - vc.position), vc.normal, vc.uv, vc.tangent, vc.bitangent);

            // Indirect lighting.
            float3 d = vl.position - vc.position;
            geom = geometry_term(vc.position, vc.normal, vl.position, vl.normal);
            float visibility = scene.visibility_test(vc.position, normalize(d), length(d));
            pdf = vc.pdf * vl.pdf;
            L_indir = Le * f * geom * visibility;

            // Direct lighting.
            Intersection light_isect;
            float3 wi;
            float light_pdf;
            scene.sample_light(vc.position, light_isect, wi, light_pdf, geom);
            float3 Li = float3::zero();
            if (!scene.directional_area_light || std::abs(dot(wi, -light_isect.normal) > (1.0 - scene.directional_area_light_solid_angle)))
                Li = light_isect.material->emission;

            if (light_pdf > EPSILON) {
                L_dir = Li * vc.attenuation * vc.material->eval(vc.wo, wi, vc.normal, vc.uv, vc.tangent, vc.bitangent) * geom / light_pdf;
                if (!vc.material->is_delta) L_dir = L_dir * std::abs(dot(wi, vc.normal));
            }

            return L_dir + L_indir;
        }

        return float3::zero();
    }

    float BidirectionalIntegrator::geometry_term(float3 const& p0, float3 const& n0, float3 const& p1, float3 const& n1) {
        float3 w = p1 - p0;
        float dist = length(w);
        w = w / dist;
        return std::abs(dot(w, n0) * dot(w, n1)) / (dist * dist);
    }

    void BidirectionalIntegrator::generate_path(Ray const& init_ray, std::vector<VertexInfo>& path, Scene const& scene, PathType type) {
        Ray ray = init_ray;
        float3 attenuation = float3::one();
        float accum_pdf = 1.0f;

        while (true) {
            if (ray.depth == 0) break;

            Intersection isect;
            scene.intersect(ray, isect);

            if (!isect.hit) break;

            VertexInfo v;
            v.position = isect.position;
            v.normal = isect.normal;
            v.bitangent = isect.bitangent;
            v.tangent = isect.tangent;
            v.uv = isect.uv;
            v.material = isect.material;
            v.attenuation = attenuation;
            switch (type) {
            case PathType::Camera:
                v.wo = -ray.direction; break;
            case PathType::Light:
                v.wi = -ray.direction; break;
            }

            // Handle hitting emissive material.
            if (v.material->emissive) {
                if (type == PathType::Camera && (ray.depth == init_ray.depth || ray.is_delta)) {
                    path.push_back(v);
                }
                break;
            }

            // Sample new ray.
            float pdf = 1.0f;
            switch (type) {
            case PathType::Camera:
                v.material->sample(v.wo, v.normal, v.wi, v.tangent, v.bitangent, pdf, ray.is_delta); break;
            case PathType::Light:
                v.material->sample(v.wi, v.normal, v.wo, v.tangent, v.bitangent, pdf, ray.is_delta); break;
            }
            
            float3 f = v.material->eval(v.wo, v.wi, v.normal, v.uv, v.tangent, v.bitangent);
            if (!ray.is_delta) f = f * std::abs(dot(v.wi, v.normal));

            if (pdf > EPSILON) attenuation = attenuation * f / pdf;

#if 0
            if (path.size() > 0) {
                auto const& pv = path[path.size() - 1];
                auto d = v.position - pv.position;
                switch (type) {
                case PathType::Camera:
                    accum_pdf *= pdf * (std::abs(dot(normalize(d), v.normal)) / d.norm2()); break;
                case PathType::Light:
                    accum_pdf *= pdf * (std::abs(dot(normalize(d), pv.normal)) / d.norm2()); break;
                }
            }
            v.pdf = accum_pdf;
            path.push_back(v);
#else
            v.pdf = accum_pdf;
            path.push_back(v);
            accum_pdf *= pdf;
#endif

            float3 dir;
            switch (type) {
            case PathType::Camera: dir = v.wi; break;
            case PathType::Light: dir = v.wo; break;
            }
            float3 offset = dot(dir, v.normal) > 0 ? v.normal * rEPSILON : -v.normal * rEPSILON;
            ray.set_origin(v.position + offset);
            ray.set_direction(dir);

            ray.depth -= 1;
        }
    }

} // namespace tira
