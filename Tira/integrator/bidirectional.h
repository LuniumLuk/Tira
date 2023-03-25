//
// Created by Ziyi.Lu 2023/02/17
//

#ifndef BIDIRECTIONAL_H
#define BIDIRECTIONAL_H

#include <integrator/integrator.h>

namespace tira {

    struct BidirectionalIntegrator : Integrator {

        struct VertexInfo {
            float3 position;
            float3 normal;
            float3 tangent;
            float3 bitangent;
            float2 uv;
            float3 wi;
            float3 wo;
            float pdf = 1.0f;
            float geom;
            Material* material = nullptr;
            float3 attenuation;
            bool is_delta = false;
        };

        enum struct PathType {
            Camera,
            Light,
        };

        virtual float3 get_pixel_color(int x, int y, int sample_id, Scene const& scene) override;

        void render_paths(Ray const& camera_ray, Ray const& ligth_ray, float3 const& Le, Scene const& scene, std::vector<float>& mis_weights, std::vector<float3>& Ls);
        float3 eval_path(Scene const& scene, std::vector<VertexInfo> const& camera_path, std::vector<VertexInfo> const& light_path, float3 const& Le, size_t t, size_t s, float& pdf);
        float geometry_term(float3 const& p0, float3 const& n0, float3 const& p1, float3 const& n1);

        void generate_path(Ray const& init_ray, std::vector<VertexInfo>& path, Scene const& scene, PathType type);
    };

} // namespace tira

#endif
