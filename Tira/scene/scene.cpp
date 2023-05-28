//
// Created by Ziyi.Lu 2022/12/05
//

#include <iostream>
#include <misc/utils.h>
#include <math/matrix.h>
#include <scene/scene.h>
#include <geometry/triangle.h>
#include <geometry/sphere.h>
#define TINYOBJLOADER_IMPLEMENTATION
#include <scene/bvh.h>
#include <scene/octree.h>
#include <thirdparty/tiny_obj_loader.h>
#include <thirdparty/pugixml.hpp>
#include <thirdparty/PoissonGenerator.h>

namespace tira {

    Scene::~Scene() {
        for (auto& m : materials) delete m;
    }

    float4x4 Scene::get_transform() const {
        return camera.get_proj_view() * model;
    }

    void Scene::draw_wireframe(Image& image, colorf const& color) const {
        auto transform = camera.get_proj_view() * model;
        for (auto& o : accel->objects) {
            o->draw_wireframe(image, get_transform(), color);
        }
    }

    void Scene::intersect(Ray& ray, Intersection& isect) const {
        if (accel) {
            accel->intersect(ray, isect);
        }
    }

    void Scene::load_envmap(std::string const& path) {
        if (envmap) delete envmap;

        envmap = new tira::TextureEnv(path);
    }

    void Scene::load(std::string const& obj_path, std::string const& xml_path, MaterialType material_type) {
        /////////////////////////////////////////////
        // Parse obj file.
        /////////////////////////////////////////////
        tinyobj::ObjReaderConfig reader_config;
        reader_config.mtl_search_path = obj_path.substr(0, obj_path.find_last_of('/') + 1);

        tinyobj::ObjReader reader;

        if (!reader.ParseFromFile(obj_path, reader_config)) {
            if (!reader.Error().empty()) {
                std::cerr << "TinyObjReader: " << reader.Error();
            }
            exit(1);
        }

        if (!reader.Warning().empty()) {
            std::cout << "[Tira] " << "TinyObjReader: " << reader.Warning();
        }

        // Open xml file.
        pugi::xml_document doc;
        pugi::xml_parse_result result = doc.load_file(xml_path.c_str());
        if (!result) {
            std::cerr << "Failed to load xml file: " << xml_path << "\n";
            exit(1);
        }

        // Load scene specs.
        if (!doc.child("scene").empty()) {
            auto const& node = doc.child("scene");

            REQUIRED_ATTRIBUTE(node, "scale");
            REQUIRED_ATTRIBUTE(node, "accel");

            scene_scale = node.attribute("scale").as_float();
            auto type = node.attribute("accel").as_string();
            if (!node.attribute("dirlight").empty()) {
                directional_area_light = node.attribute("dirlight").as_bool();
            }
            if (!node.attribute("dirsolidangle").empty()) {
                directional_area_light_solid_angle = node.attribute("dirsolidangle").as_float();
            }
            if (type == std::string("bvh")) {
                accel_type = AcceleratorType::BVH;
            }
            else if (type == std::string("octree")) {
                accel_type = AcceleratorType::BVH;
            }
        }

        auto& attrib = reader.GetAttrib();
        auto& shapes = reader.GetShapes();
        auto& _materials = reader.GetMaterials();

        // Load materials (only needed properties).
        for (auto m : _materials) {
            switch (material_type) {
            case MaterialType::BlinnPhong:
            {
#ifdef USE_GLASS_MATERIAL
                float3 transmittance = { m.transmittance[0],
                                         m.transmittance[1],
                                         m.transmittance[2] };
                if (isglass(m.ior, transmittance)) {
                    auto material = new GlassMaterial{};
                    material->type = MaterialClassType::Glass;
                    material->ior = m.ior;
                    material->transmittance = transmittance;
                    material->name = m.name;
                    material->emission = { m.emission[0],
                                           m.emission[1],
                                           m.emission[2] };
                    if (material->emission.max_component() > EPSILON) {
                        material->emissive = true;
                    }
                    materials.push_back(static_cast<Material*>(material));
                    std::cout << "[Tira] " << "load " << *material << "\n";
                    break;
                }
#endif
                auto material = new BlinnPhongMaterial{};
                material->type = MaterialClassType::BlinnPhong;
                material->diffuse = { m.diffuse[0],
                                      m.diffuse[1],
                                      m.diffuse[2] };
                material->specular = { m.specular[0],
                                       m.specular[1],
                                       m.specular[2] };
                material->emission = { m.emission[0],
                                       m.emission[1],
                                       m.emission[2] };
                if (material->emission.max_component() > EPSILON) {
                    material->emissive = true;
                }
                material->transmittance = transmittance;
                material->shininess = m.shininess;
                material->ior = m.ior;
                material->name = m.name;
                if (m.diffuse_texname.length() > 0) {
                    auto diffuse_texpath = reader_config.mtl_search_path + m.diffuse_texname;
                    material->diffuse_texture = new tira::Texture2D(diffuse_texpath, true);
                }

                materials.push_back(static_cast<Material*>(material));
                std::cout << "[Tira] " << "load " << *material << "\n";
            }
            break;
            case MaterialType::DisneyBSDF:
            {
                auto material = new DisneyBSDFMaterial{};
                material->type = MaterialClassType::DisneyBSDF;
                material->base_color = { m.diffuse[0],
                                         m.diffuse[1],
                                         m.diffuse[2] };
                material->emission = { m.emission[0],
                                       m.emission[1],
                                       m.emission[2] };
                if (material->emission.max_component() > EPSILON) {
                    material->emissive = true;
                }
                // Convertion based on http://graphicrants.blogspot.com/2013/08/specular-brdf-reference.html
                float s = m.shininess;
                material->roughness = clamp(std::pow(2.f / (s + 2.f), 0.25f), 0.f, 1.f);
                material->name = m.name;

                materials.push_back(static_cast<Material*>(material));
                std::cout << "[Tira] " << "load " << *material << "\n";
            }
            break;
            }
        }
        auto missing_material = new BlinnPhongMaterial{};
        missing_material->diffuse = { .0f, 1.f, .0f };
        missing_material->specular = { .0f };
        missing_material->ior = 1.f;
        missing_material->shininess = 0.f;
        missing_material->name = "Missing";
        materials.push_back(static_cast<Material*>(missing_material));

        std::vector<Object*> objects;

        timer.update();
        for (size_t s = 0; s < shapes.size(); s++) {
            size_t index_offset = 0;
            for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
                size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);
                Triangle* triangle = new Triangle;

                for (size_t v = 0; v < fv; v++) {
                    tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
                    tinyobj::real_t vx = attrib.vertices[3 * size_t(idx.vertex_index) + 0] * scene_scale;
                    tinyobj::real_t vy = attrib.vertices[3 * size_t(idx.vertex_index) + 1] * scene_scale;
                    tinyobj::real_t vz = attrib.vertices[3 * size_t(idx.vertex_index) + 2] * scene_scale;
                    triangle->pos[v] = { vx, vy, vz };

                    if (idx.normal_index >= 0) {
                        tinyobj::real_t nx = attrib.normals[3 * size_t(idx.normal_index) + 0];
                        tinyobj::real_t ny = attrib.normals[3 * size_t(idx.normal_index) + 1];
                        tinyobj::real_t nz = attrib.normals[3 * size_t(idx.normal_index) + 2];
                        triangle->vn[v] = { nx, ny, nz };
                        triangle->has_vn = true;
                    }

                    if (idx.texcoord_index >= 0) {
                        tinyobj::real_t tx = attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
                        tinyobj::real_t ty = attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];
                        triangle->vt[v] = { tx, ty };
                        triangle->has_vt = true;
                    }
                }

                triangle->e01 = triangle->pos[1] - triangle->pos[0];
                triangle->e02 = triangle->pos[2] - triangle->pos[0];
                triangle->normal = triangle->e01.cross(triangle->e02).normalized();

                index_offset += fv;

                auto material_id = shapes[s].mesh.material_ids[f];
                if (material_id < 0) {
                    material_id = materials.size() - 1;
                }
                triangle->material = materials[material_id];

                triangle->calc_center();
                triangle->calc_bound();
                triangle->calc_area();
                triangle->calc_tangent();

                objects.push_back((Object*)triangle);
            }
        }

        /////////////////////////////////////////////
        // Parse xml file.
        /////////////////////////////////////////////

        for (auto& node : doc.children("sphere")) {
            REQUIRED_ATTRIBUTE(node, "mtlname");
            REQUIRED_ATTRIBUTE(node, "center");
            REQUIRED_ATTRIBUTE(node, "radius");

            std::string name = node.attribute("mtlname").as_string();
            auto s = new Sphere{};
            CHECK_SCANNED_ITEMS(sscanf(node.attribute("center").as_string(), "%f,%f,%f", &s->center.x, &s->center.y, &s->center.z), 3);
            s->radius = node.attribute("radius").as_float();
            s->calc_area();
            s->calc_bound();
            for (auto& m : materials) {
                if (m->name == name) {
                    s->material = m;
                }
            }
            objects.push_back((Object*)s);
        }

        std::cout << "[Tira] " << "Materials load: " << materials.size() << "\n";
        std::cout << "[Tira] " << "Objects load: " << objects.size() << "\n";

        timer.update();
        std::cout << "[Tira] " << "Scene load elapsed time: " << timer.delta_time() << "s\n";

        // Load camera specs.
        if (doc.child("camera").empty()) {
            throw std::runtime_error("Camera tag is required");
        }

        {
            auto const& node = doc.child("camera");
            REQUIRED_ATTRIBUTE(node, "type");
            REQUIRED_ATTRIBUTE(node, "width");
            REQUIRED_ATTRIBUTE(node, "height");
            REQUIRED_ATTRIBUTE(node, "fovy");

            if (node.attribute("type").as_string() != std::string("perspective")) {
                throw std::runtime_error("Only support perspective type camera so far");
            }

            scr_w = node.attribute("width").as_int();
            scr_h = node.attribute("height").as_int();
            float fov = node.attribute("fovy").as_float();
            camera.fov = deg2rad(fov);
            camera.aspect = (float)scr_w / scr_h;

            auto const& eye = node.child("eye");
            camera.eye.x = eye.attribute("x").as_float() * scene_scale;
            camera.eye.y = eye.attribute("y").as_float() * scene_scale;
            camera.eye.z = eye.attribute("z").as_float() * scene_scale;

            auto const& at = node.child("lookat");
            camera.at.x = at.attribute("x").as_float() * scene_scale;
            camera.at.y = at.attribute("y").as_float() * scene_scale;
            camera.at.z = at.attribute("z").as_float() * scene_scale;

            auto const& up = node.child("up");
            camera.up.x = up.attribute("x").as_float();
            camera.up.y = up.attribute("y").as_float();
            camera.up.z = up.attribute("z").as_float();

            if (!node.child("thinlens").empty()) {
                auto const& thinlens = node.child("thinlens");
                REQUIRED_ATTRIBUTE(thinlens, "focus");
                REQUIRED_ATTRIBUTE(thinlens, "aperature");

                camera.mode = Camera::CameraMode::ThinLens;
                camera.focus_length = thinlens.attribute("focus").as_float();
                camera.aperature = thinlens.attribute("aperature").as_float();
                std::cout << "[Tira] Using thin lens camera\n";
            }
            else {
                std::cout << "[Tira] Using pinhole camera\n";
            }
        }

        // Load extra material specs.
        for (auto& node : doc.children("light")) {
            REQUIRED_ATTRIBUTE(node, "mtlname");
            REQUIRED_ATTRIBUTE(node, "radiance");
            std::string name = node.attribute("mtlname").as_string();
            float3 radiance;
            CHECK_SCANNED_ITEMS(sscanf(node.attribute("radiance").as_string(), "%f,%f,%f", &radiance.x, &radiance.y, &radiance.z), 3);
            for (auto& m : materials) {
                if (m->name == name) {
                    m->emissive = true;
                    m->emission = radiance;
                }
            }
        }

        // Load envmap specs.
        if (!doc.child("envmap").empty()) {
            auto const& node = doc.child("envmap");
            REQUIRED_ATTRIBUTE(node, "url");
            load_envmap(node.attribute("url").as_string());
            if (!node.attribute("scale").empty())
                envmap_scale = node.attribute("scale").as_float();
            std::cout << "[Tira] " << "Using envmap, url: " << node.attribute("url").as_string() << "\n";
        }

        // Load sunlight specs.
        if (!doc.child("sunlight").empty()) {
            auto const& node = doc.child("sunlight");
            REQUIRED_ATTRIBUTE(node, "direction");
            REQUIRED_ATTRIBUTE(node, "radiance");
            sun_enabled = true;
            CHECK_SCANNED_ITEMS(sscanf(node.attribute("direction").as_string(), "%f,%f,%f", &sun_direction.x, &sun_direction.y, &sun_direction.z), 3);
            sun_direction = normalize(sun_direction);
            CHECK_SCANNED_ITEMS(sscanf(node.attribute("radiance").as_string(), "%f,%f,%f", &sun_radiance.x, &sun_radiance.y, &sun_radiance.z), 3);
            if (!node.attribute("solidangle").empty())
                sun_solid_angle = node.attribute("solidangle").as_float();

            std::cout << "[Tira] " << "Using sunlight, direction: " << sun_direction << ", radiance: " << sun_radiance << "\n";
        }

        // Load integrator specs.
        if (!doc.child("integrator").empty()) {
            auto const& node = doc.child("integrator");
            REQUIRED_ATTRIBUTE(node, "spp");
            integrator_info.spp = node.attribute("spp").as_int();
            if (!node.attribute("mis").empty())
                integrator_info.use_mis = node.attribute("mis").as_bool();
            if (!node.attribute("maxbounce").empty())
                integrator_info.max_bounce = node.attribute("maxbounce").as_int();
            if (!node.attribute("robustlight").empty())
                integrator_info.robust_light = node.attribute("robustlight").as_bool();
            if (!node.attribute("type").empty()) {
                auto const& type = node.attribute("type").as_string();
                if (type == std::string("whitted")) integrator_info.type = IntegratorType::Whitted;
                if (type == std::string("mc")) integrator_info.type = IntegratorType::MonteCarlo;
                if (type == std::string("bdpt")) integrator_info.type = IntegratorType::Bidirectional;
            }

            if (!node.child("clamp").empty()) {
                integrator_info.clamping.min = node.child("clamp").attribute("min").as_float();
                integrator_info.clamping.max = node.child("clamp").attribute("max").as_float();
            }
        }

        // Load tiling specs.
        if (!doc.child("kernel").empty()) {
            auto const& node = doc.child("kernel");
            REQUIRED_ATTRIBUTE(node, "size");
            kernel_info.size = node.attribute("size").as_int();
            if (!node.attribute("macro").empty())
                kernel_info.macro = node.attribute("macro").as_string();
        }

        /////////////////////////////////////////////
        // Build accelerate structure.
        /////////////////////////////////////////////
        switch (accel_type) {
        case AcceleratorType::BVH:
            accel = new BVHAccel(); break;
        case AcceleratorType::Octree:
            accel = new OctreeAccel(); break;
        }
        std::cout << "[Tira] " << "Building accleration Structure ... please wait ...\n";
        timer.update();
        accel->build(std::forward<std::vector<Object*>>(objects));
        timer.update();
        std::cout << "[Tira] " << "Accleration Structure build elapsed time: " << timer.delta_time() << "s\n";

        setup_lights();
    }

    float3 uniform_sample_cone(float2 const& u, float cos_theta_max) {
        float cos_theta = (1.f - u.x) + u.x * cos_theta_max;
        float sin_theta = std::sqrt(1.f - cos_theta * cos_theta);
        float phi = u.y * TWO_PI;
        return spherical_to_cartesian(sin_theta, cos_theta, phi);
    }

    float3 Scene::sample_sun(float3 const& P, float3 const& N, float3& wi, float& pdf, float& geom) const {
        float2 u = random_float2();
        float cos_theta_max = 1.f - sun_solid_angle * TWO_PI;
        float3 dir = uniform_sample_cone(u, cos_theta_max);
        wi = normalize(local_to_world(dir, sun_direction));

        if (dot(wi, N) > 0) {
            pdf = 1.f / sun_solid_angle;
        }

        float3 offset = dot(wi, N) > 0 ? N * rEPSILON : -N * rEPSILON;
        Ray ray(P + offset, wi);
        ray.shadow_ray = true;

        Intersection isect;
        intersect(ray, isect);

        if (isect.hit) {
            geom = 0.f;
        }
        else {
            geom = 1.f;
        }

        return sun_radiance;
    }

    float3 Scene::sample_envmap(float3 const& P, float3 const& N, float3& wi, float& pdf, float& geom) const {
        float3 dir = random_float3_on_unit_hemisphere();
        wi = normalize(local_to_world(dir, N));
        pdf = INV_TWO_PI;

        float3 offset = dot(wi, N) > 0 ? N * rEPSILON : -N * rEPSILON;
        Ray ray(P + offset, wi);
        ray.shadow_ray = true;

        Intersection isect;
        intersect(ray, isect);

        if (isect.hit) {
            geom = 0.f;
        }
        else {
            geom = 1.f;
        }

        return envmap->sample(wi);
    }

    void Scene::setup_lights() {
        lights.clear();
        lights_cdf.clear();
        lights_total_area = 0;
        for (auto& o : accel->objects) {
            if (o->material->emissive) {
                lights_total_area += o->get_area();
                lights.push_back(o);
                lights_cdf.push_back(lights_total_area);
            }
        }
        std::cout << "[Tira] " << "Lights: " << lights.size() << "\n";
        std::cout << "[Tira] " << "Lights total area: " << lights_total_area << "\n";
    }

    void Scene::sample_light(float3 const& P, Intersection& isect, float3& wi, float& pdf, float& geom) const {
        float p = random_float() * lights_total_area;
        for (int i = 0; i < lights.size(); ++i) {
            if (lights_cdf[i] >= p) {
                lights[i]->sample(isect, pdf);
                pdf = 1 / lights_total_area;

                auto Q = isect.position;
                auto PQ = Q - P;
                wi = PQ.normalized();
                auto PQ2 = dot(PQ, PQ);

                // Avoid seam-like artifacts, query a shadow ray with a small step.
                float3 offset = dot(wi, isect.normal) > 0 ? isect.normal * rEPSILON : -isect.normal * rEPSILON;
#if 1
            // Visibility test by object pointer
                float visibility = visibility_test(P + offset, wi, isect.object);
#else
                // Visibility test by distance
                float visibility = visibility_test(P + offset, wi, length(PQ));
#endif

                // Geometric term as in:
                //  V(x<-->x')|N_x * x'x||N_x' * xx'|/||x-x'||^2
                //  here |N_x * x'x| will be calculated outside this function
                geom = visibility * std::max(-wi.dot(isect.normal), EPSILON) / PQ2;

                return;
            }
        }
    }

    Ray Scene::sample_light_ray(float3& emission, float& pdf) const {
        float p = random_float() * lights_total_area;
        for (int i = 0; i < lights.size(); ++i) {
            if (lights_cdf[i] >= p) {
                Intersection isect;
                lights[i]->sample(isect, pdf);

                // float3 dir = cosine_sample_hemisphere(random_float2());
                // Ray ray(isect.position, local_to_world(dir, isect.normal));
                // emission = isect.material->emission;
                // float pdf_pos = 1 / lights_total_area;
                // float pdf_dir = dir.z * INV_PI;
                // pdf = 1 / pdf_pos * pdf_dir;
                // return ray;

                if (directional_area_light) {
                    Ray ray(isect.position, isect.normal);
                    emission = isect.material->emission;
                    pdf = 1 / lights_total_area;
                    return ray;
                }
                else {
                    float3 dir = random_float3_on_unit_hemisphere();
                    Ray ray(isect.position, local_to_world(dir, isect.normal));
                    emission = isect.material->emission;
                    pdf = 1 / lights_total_area;
                    return ray;
                }
            }
        }
        return Ray(float3::zero(), float3::zero()); // Should not occur.
    }


    float Scene::visibility_test(float3 const& P, float3 const& wi, Object const* object) const {
        Ray ray(P, wi);
        ray.shadow_ray = true;

        Intersection isect;
        intersect(ray, isect);

        if (!isect.hit) return 1.f;

        if (isect.object == object) return 1.f;

        return 0.f;
    }

    float Scene::visibility_test(float3 const& P, float3 const& wi, float dist) const {
        Ray ray(P, wi);
        ray.shadow_ray = true;

        Intersection isect;
        intersect(ray, isect);

        if (!isect.hit) return 1.f;

        float d = isect.distance * (1.01f);

        if (d >= dist) return 1.f;

        return 0.f;
    }

    bool Scene::hit_sun(float3 const& wi) const {
        return std::abs(dot(sun_direction, wi)) > (1.0f - sun_solid_angle * TWO_PI);
    }

    std::vector<float2> generate_poisson_dist(size_t num) {
        PoissonGenerator::DefaultPRNG PRNG;
        auto const poisson_points = PoissonGenerator::generatePoissonPoints(num, PRNG, true);
        std::vector<float2> poisson_disk;
        for (auto& p : poisson_points) {
            poisson_disk.emplace_back(p.x - .5f, p.y - .5f);
        }
        return poisson_disk;
    }

    void Scene::generate_simple_scene() {
        auto light = new BlinnPhongMaterial{};
        light->emissive = true;
        light->emission = { 10.f };
        light->name = "Light";
        materials.push_back(static_cast<Material*>(light));

        auto white = new BlinnPhongMaterial{};
        white->diffuse = { .9f };
        white->specular = { .3f };
        white->ior = 1.f;
        white->shininess = 10.f;
        white->name = "White";
        materials.push_back(static_cast<Material*>(white));

        auto metal = new BlinnPhongMaterial{};
        metal->diffuse = { 0.f };
        metal->specular = { .9f };
        metal->ior = 1.f;
        metal->shininess = 100000.f;
        metal->name = "Metal";
        materials.push_back(static_cast<Material*>(metal));

        auto glass = new GlassMaterial{};
        glass->transmittance = { .9f };
        glass->ior = 1.5f;
        glass->name = "Glass";
        materials.push_back(static_cast<Material*>(glass));

        std::vector<Object*> objects;

        auto s0 = new Sphere{};
        s0->center = { 0.f, -1000.f, 0.f };
        s0->radius = 1000.f;
        s0->material = metal;
        s0->calc_area();
        s0->calc_bound();
        objects.push_back(static_cast<Object*>(s0));

        auto s1 = new Sphere{};
        s1->center = { 0.f, 4.f, 0.f };
        s1->radius = 4.f;
        s1->material = glass;
        s1->calc_area();
        s1->calc_bound();
        objects.push_back(static_cast<Object*>(s1));

        // auto s2 = new Sphere{};
        // s2->center = { -4.f, 4.f, 4.f };
        // s2->radius = 0.5f;
        // s2->material = light;
        // s2->calc_area();
        // s2->calc_bound();
        // objects.push_back(static_cast<Object*>(s2));

        accel = new BVHAccel();
        accel->build(std::forward<std::vector<Object*>>(objects));

        camera.fov = 20.f;
        camera.eye = { 0.f, 4.f, 12.f };
        camera.at = { 0.f, 4.f,  0.f };
        camera.up = { 0.f, 1.f,  0.f };

        setup_lights();
    }

} // namespace tira
