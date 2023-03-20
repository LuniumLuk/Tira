//
// Created by Ziyi.Lu 2022/12/05
//

#ifndef SCENE_H
#define SCENE_H

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <misc/utils.h>
#include <misc/image.h>
#include <misc/timer.h>
#include <geometry/object.h>
#include <geometry/ray.h>
#include <scene/material.h>
#include <scene/accel.h>
#include <scene/camera.h>

namespace tira {

    struct Scene {
        struct IntegratorInfo {
            int spp = 1;
            int version = 2;
            bool use_mis = true;
            int max_bounce = 8;
            bool robust_light = true; // Accept intersection very close to light as light intersection.
        };

        struct TilingInfo {
            int size = 64;
            int num = 16;
            int spf = 8;
        };

        int scr_w = 1024;
        int scr_h = 1024;

        Timer timer;
        std::vector<Object*> lights;
        std::vector<float> lights_cdf;
        float lights_total_area;
        float scene_scale = 1.f;
        IntegratorInfo integrator_info;
        TilingInfo tiling_info;

        enum struct AcceleratorType {
            BVH,
            Octree,
        };

        enum struct MaterialType {
            BlinnPhong,
            DisneyBSDF,
        };

        Scene() {};
        ~Scene();

        AcceleratorType accel_type = AcceleratorType::BVH;
        Camera camera;
        float4x4 model = float4x4::identity();
        Accelerator* accel;
        std::vector<Material*> materials;

        /// Envmap ///
        TextureEnv* envmap = nullptr;
        float envmap_scale = 1.0f;
        void load_envmap(std::string const& path);

        /// Sun ///
        bool sun_enabled = false;
        float3 sun_direction = normalize(float3(-1.f, 1.f, 1.f));
        float sun_solid_angle = 6.87e-2f;
        float3 sun_radiance = { 50.f };


        float3 sample_sun(float3 const& P, float3 const& N, float3& wi, float& pdf, float& geom) const;
        float3 sample_envmap(float3 const& P, float3 const& N, float3& wi, float& pdf, float& geom) const;

        float4x4 get_transform() const;
        void draw_wireframe(Image& image, colorf const& color) const;
        void intersect(Ray& ray, Intersection& isect) const;

        void generate_simple_scene();
        void load(std::string const& obj_path, std::string const& xml_path, MaterialType material_type);
        void setup_lights();
        void sample_light(float3 const& P, Intersection& isect, float3& wi, float& pdf, float& geom) const;
        Ray sample_light_ray(float3& emission, float& pdf) const;
        float visibility_test(float3 const& P, float3 const& wi, Object const* object) const;
        float visibility_test(float3 const& P, float3 const& wi, float dist) const;
        bool hit_sun(float3 const& wi) const;

    };

    std::vector<float2> generate_poisson_dist(size_t num);

} // namespace tira

#endif
