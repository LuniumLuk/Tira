//
// Created by Ziyi.Lu 2023/02/17
//

#ifndef BIDIRECTIONAL_H
#define BIDIRECTIONAL_H

#include <integrator/integrator.h>

namespace tira {

    struct VertexInfo {
        float3 position;
        float3 normal;
        float3 tangent;
        float3 bitangent;
        float2 uv;
        float3 wi;
        float3 wo;
        float pdf = 1.f;
        float geom;
        Material* material = nullptr;
        float3 beta;
        bool is_delta = false;
    };

    struct BidirectionalIntegrator : Integrator {

        virtual float3 get_pixel_color(int x, int y, int sample_id, Scene const& scene) override;

        float3 connect_path(Scene const& scene, std::vector<VertexInfo> const& camera_path, std::vector<VertexInfo> const& light_path, int t, int s, float& mis_weight);
        float3 render_path(Ray const& camera_ray, Ray const& ligth_ray, float3 const& Le, Scene const& scene);

        float geometry_term(float3 const& p0, float3 const& n0, float3 const& p1, float3 const& n1);
        float3 eval_path(std::vector<VertexInfo> const& camera_path, std::vector<VertexInfo> const& light_path, int eye_idx, int light_idx);

        void generate_camera_path(Ray const& camera_ray, std::vector<VertexInfo>& camera_path, Scene const& scene);
        void generate_light_path(Ray const& light_ray, std::vector<VertexInfo>& light_path, Scene const& scene);
    };

} // namespace tira

#endif
