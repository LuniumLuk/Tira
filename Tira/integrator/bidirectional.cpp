//
// Created by Ziyi.Lu 2023/03/20
//

#include <integrator/bidirectional.h>

#define POISSON_POINTS_NUM 32

namespace tira {

    float3 BidirectionalIntegrator::get_pixel_color(int x, int y, int sample_id, Scene const& scene) {
        // [TODO] Use Spectrum struct to represent light.
        float3 L = float3::zero();
        auto const& u = poisson_disk[sample_id % POISSON_POINTS_NUM];

        Ray camera_ray = scene.camera.get_ray_pinhole(x, y, scene.scr_w, scene.scr_h, u);
        camera_ray.depth = MAX_RAY_DEPTH;

        float3 L_accum = float3::zero();
        for (int i = 0; i < NUM_LIGHT_SAMPLES; ++i) {
            float3 light_L;
            float light_pdf;
            Ray light_ray = scene.sample_light_ray(light_L, light_pdf);
            light_ray.depth = MAX_RAY_DEPTH;
            light_L = light_L / light_pdf;
            L_accum += render_path(camera_ray, light_ray, light_L, scene);
        }

        L = L_accum / NUM_LIGHT_SAMPLES;

        return L;
    }

    float3 BidirectionalIntegrator::connect_path(Scene const& scene, std::vector<VertexInfo> const& camera_path, std::vector<VertexInfo> const& light_path, int t, int s, float& mis_weight) {
        float3 L = float3::zero();

        if (s == 0) {
            // No vertices on the light subpath are used, only valid when the camera subpath contains a light source
            auto const& v = camera_path[t - 1];
            if (v.material->emissive) {
                L = v.material->emission * v.beta;
            }
        }
        else if (t == 1) {
            auto const& v = light_path[s - 1];
            // [TODO] determine v is connectible
            float3 wo = scene.camera.eye - v.position;
            float dist = wo.norm();
            float visibility = scene.visibility_test(v.position, wo, dist);

            float3 camera_n = normalize(scene.camera.at - scene.camera.eye);
            float cos_theta = dot(-wo, camera_n);
            float pdf = (dist * dist) / std::abs(cos_theta);

            float3 Wi = float3::zero();

            if (cos_theta > 0) {
                auto raster_to_screen = scene.camera.get_raster_to_screen();
                auto screen = raster_to_screen * -wo;
                if (screen.x > -1.f && screen.x < 1.f && screen.y > -1.f && screen.y < 1.f) {
                    float cos2theta = cos_theta * cos_theta;
                    // The screen area is 4 (x : [-1, 1], y : [-1, 1])
                    Wi = float3(1 / (4 * cos2theta * cos2theta));
                }
            }

            if (Wi.norm2() > EPSILON && pdf > EPSILON) {
                L = v.beta * v.material->eval(wo, v.wi, v.normal, v.uv, v.tangent, v.bitangent) * (Wi / pdf) * visibility * std::abs(dot(wo, v.normal));
            }
        }
        else if (s == 1) {
            // Sample a point on a light and connect it to the light subpath
            auto const& v = camera_path[t - 1];
            // [TODO] determine v is connectible
            Intersection light_isect;
            float3 wi;
            float pdf;
            float geom;
            scene.sample_light(v.position, light_isect, wi, pdf, geom);
            float3 Li = light_isect.material->emission;

            if (pdf > EPSILON) {
                L = v.beta * v.material->eval(v.wo, wi, v.normal, v.uv, v.tangent, v.bitangent) * geom * std::abs(dot(wi, v.normal)) / pdf;
            }
        }
        else {
            // Handle other bidirectional connection cases
            auto const& vl = light_path[s - 1];
            auto const& vc = camera_path[t - 1];
            // [TODO] determine v is connectible
            L = vl.beta * vl.material->eval(normalize(vc.position - vl.position), vl.wi, vl.normal, vl.uv, vl.tangent, vl.bitangent)
                * vc.beta * vc.material->eval(vc.wo, normalize(vl.position - vc.position), vc.normal, vc.uv, vc.tangent, vc.bitangent);

            if (dot(L, L) > EPSILON) {
                float3 d = vl.position - vc.position;
                float dist = d.norm();
                float g = 1 / (dist * dist);
                d = normalize(d);
                g *= std::abs(dot(vl.normal, d));
                g *= std::abs(dot(vc.normal, d));
                float visibility = scene.visibility_test(vc.position, d, dist);
                L = L * g * visibility;
            }
        }

        // Multiple Importace Sampling
        return L;
    }

    float3 BidirectionalIntegrator::render_path(Ray const& camera_ray, Ray const& ligth_ray, float3 const& Le, Scene const& scene) {
        float3 L = float3::zero();
        std::vector<VertexInfo> camera_path;
        std::vector<VertexInfo> light_path;

        VertexInfo v_camera;
        v_camera.position = camera_ray.origin;
        camera_path.push_back(v_camera);
        VertexInfo v_light;
        v_light.position = ligth_ray.origin;
        light_path.push_back(v_light);

        generate_camera_path(camera_ray, camera_path, scene);
        generate_light_path(ligth_ray, light_path, scene);

        int n_camera = camera_path.size();
        int n_light = light_path.size();

        for (int t = 1; t <= n_camera; ++t) for (int s = 0; s <= n_light; ++s) {
            // if t == 1, this is identical to standard light sampling approach
            // if s == 1, this is camera sampling approach
            int depth = t + s - 2;
            if ((t == 1 && s == 1) || depth < 0 || depth > MAX_RAY_DEPTH)
                continue;

            float mis_weight = 0.f;
            float3 L_path = connect_path(scene, camera_path, light_path, t, s, mis_weight);

            if (t != 1) {
                L += L_path;
            }
            else {
                // record immediate sample contribution to another pixel
            }
        }


        // // Case #1 - path tracing & bidirectional path tracing.
        // for (int s = 0; s < camera_path.size(); ++s) {

        //     if (s > 1) continue;

        //     auto const& ev = camera_path[s];
        //     if (ev.material->emissive) {
        //         L +=  ev.material->emission;
        //         break;
        //     }

        //     // if (!ev.is_delta) {
        //     //     Intersection light_isect;
        //     //     float3 wi;
        //     //     float light_pdf;
        //     //     float geom;
        //     //     scene.sample_light(ev.position, light_isect, wi, light_pdf, geom);
        //     //     float3 Li = light_isect.material->emission;

        //     //     if (s > 0) {
        //     //         Li = Li * camera_path[s - 1].beta;
        //     //     }

        //     //     if (light_pdf < EPSILON || geom < EPSILON) continue;

        //     //     float3 f = ev.material->eval(ev.wo, wi, ev.normal, ev.uv, ev.tangent, ev.bitangent);

        //     //     L += Li * f * geom  * std::abs(dot(wi, ev.normal)) / light_pdf;
        //     // }

        //     // Case #2 - bidirectional path tracing
        //     for (int t = 0; t < light_path.size(); ++t) {
        //         if (t + s + 1 >= MAX_RAY_DEPTH) break;

        //         auto const& lv = light_path[t];

        //         float3 P = ev.position + ev.normal * sEPSILON;
        //         float3 wi = P - lv.position;
        //         float dist = length(wi);
        //         wi = normalize(wi);

        //         float visibility = scene.visibility_test(lv.position, wi, dist); 

        //         if (visibility < EPSILON) continue;

        //         L += Le * eval_path(camera_path, light_path, s, t);
        //     }
        // }

        // // Case #3 - light tracing.
        // for (int t = 0; t < light_path.size(); ++t) {
        //     auto const& lv = light_path[t];
        //     auto const& eye = camera_ray.origin;
        //     float3 wo = eye - lv.position;
        //     float dist_sqr = dot(wo, wo);

        //     if (dist_sqr < sEPSILON) continue;

        //     float3 Li = Le;
        //     if (t > 0) {
        //         Li = Li * light_path[t - 1].beta;
        //     }

        //     wo = normalize(wo);
        //     float visibility = scene.visibility_test(lv.position, wo, std::sqrt(dist_sqr));

        //     if (visibility < EPSILON) continue;

        //     float3 f = lv.material->eval(wo, lv.wi, lv.normal, lv.uv, lv.tangent, lv.bitangent) * std::abs(dot(lv.wi, lv.normal));

        //     float We = 1 - dot(camera_ray.direction, -wo) < EPSILON ? 1.f : 0.f;

        //     L += Li * f * We;
        // }

        return L;
    }

    float BidirectionalIntegrator::geometry_term(float3 const& p0, float3 const& n0, float3 const& p1, float3 const& n1) {
        float3 w = p1 - p0;
        float dist = length(w);
        w = w / dist;
        return std::abs(dot(w, n0) * dot(w, n1)) / (dist * dist);
    }

    float3 BidirectionalIntegrator::eval_path(std::vector<VertexInfo> const& camera_path, std::vector<VertexInfo> const& light_path, int eye_idx, int light_idx) {
        auto const& ev = camera_path[eye_idx];
        auto const& lv = light_path[light_idx];

        float3 L = float3::one();

        if (eye_idx > 0) L = L * camera_path[eye_idx - 1].beta;
        if (light_idx > 0) L = L * light_path[light_idx - 1].beta;

        float3 PQ = lv.position - ev.position;
        float PQ2 = dot(PQ, PQ);
        float3 wi = normalize(PQ);

        float geom_term = std::abs(dot(wi, ev.normal)) * std::abs(dot(-wi, lv.normal)) / PQ2;

        if (PQ2 < sEPSILON) return float3::zero();

        L = L * ev.material->eval(ev.wo, wi, ev.normal, ev.uv, ev.tangent, ev.bitangent);
        L = L * lv.material->eval(-wi, lv.wi, lv.normal, lv.uv, lv.tangent, lv.bitangent);

        L = L * geom_term;

        return L;
    }

    void BidirectionalIntegrator::generate_camera_path(Ray const& camera_ray, std::vector<VertexInfo>& camera_path, Scene const& scene) {
        Ray r = camera_ray;
        float3 beta = float3::one();
        unsigned int initial_depth = r.depth;

        while (true) {
            if (r.depth == 0) break;

            Intersection isect;
            scene.intersect(r, isect);

            if (!isect.hit) break;

            VertexInfo v;
            v.position = isect.position;
            v.normal = isect.normal;
            v.bitangent = isect.bitangent;
            v.tangent = isect.tangent;
            v.uv = isect.uv;
            v.material = isect.material;
            v.wo = -r.direction;

            // Handle hitting emissive material.
            if (v.material->emissive) {
                if (r.depth == initial_depth) {
                    v.beta = beta;
                    camera_path.push_back(v);
                }
                break;
            }

            if (r.depth == initial_depth) {
                float3 const& p = scene.camera.eye;
                float3 const& n = normalize(scene.camera.at - scene.camera.eye);
                v.geom = geometry_term(p, n, v.position, v.normal);
            }
            else {
                VertexInfo const& pv = camera_path.back();
                v.geom = geometry_term(pv.position, pv.normal, v.position, v.normal);
            }

            // Sample new ray.
            float pdf;
            v.material->sample(v.wo, v.normal, v.wi, v.tangent, v.bitangent, pdf, v.is_delta);
            float3 f = v.material->eval(v.wo, v.wi, v.normal, v.uv, v.tangent, v.bitangent) * std::abs(dot(v.wi, v.normal));

            if (pdf > EPSILON && dot(f, f) > EPSILON) {
                beta = beta * f / pdf;
            }

            r.set_origin(v.position + v.wi * rEPSILON);
            r.set_direction(v.wi);
            r.depth -= 1;

            // [TODO] Russian roulette.
            // float rr_prob = clamp(color_to_luminance(f), 0.f, 1.f);
            // if (random_float() > rr_prob) break;

            // Compute Monte Carlo estimator, scale pdf by Russian roulette probability.
            // float denom = pdf * rr_prob;

            v.beta = beta;
            camera_path.push_back(v);
        }
    }

    void BidirectionalIntegrator::generate_light_path(Ray const& light_ray, std::vector<VertexInfo>& light_path, Scene const& scene) {
        Ray r = light_ray;
        float3 beta = float3::one();
        unsigned int initial_depth = r.depth;

        while (true) {
            if (r.depth == 0) break;

            Intersection isect;
            scene.intersect(r, isect);

            if (!isect.hit) break;

            VertexInfo v;
            v.position = isect.position;
            v.normal = isect.normal;
            v.bitangent = isect.bitangent;
            v.tangent = isect.tangent;
            v.uv = isect.uv;
            v.material = isect.material;
            v.wi = -r.direction;

            // Sample new ray.
            float pdf;
            v.material->sample(v.wi, v.normal, v.wo, v.tangent, v.bitangent, pdf, v.is_delta);
            // [TODO] Russian roulette
            float3 f = v.material->eval(v.wo, v.wi, v.normal, v.uv, v.tangent, v.bitangent) * std::abs(dot(v.wi, v.normal));

            if (pdf > EPSILON && dot(f, f) > EPSILON) {
                beta = beta * f / pdf;
            }

            r.set_origin(v.position + v.wo * rEPSILON);
            r.set_direction(v.wo);
            r.depth -= 1;

            v.beta = beta;
            light_path.push_back(v);
        }
    }

} // namespace tira
