#include "tira.h"
#include "glwrapper.h"

namespace GL {

    struct Triangle {
        glm::vec4 pos[3];
        glm::vec4 normal;
        glm::vec4 center;
        glm::vec4 tangent;
        glm::vec4 bitangent;
        glm::vec4 vn[3];
        glm::vec4 vt[3];
        glm::vec4 e01, e02;
        glm::vec4 min;
        glm::vec4 max;
        glm::vec4 info;     // has_vn, has_vt, area, material_idx
    };

    struct Material {
        glm::vec4 emission; // emission, emissive
        glm::vec4 diffuse;
        glm::vec4 specular;
        glm::vec4 transmittance;
        glm::vec4 info;     // shininess, ior, is_delta, type [0 -> BlinnPhong, 1 -> Glass]
    };

    struct BVHNode {
        glm::vec4 min;
        glm::vec4 max;
        int hit_idx, miss_idx;
        int first_prim, prim_count;
    };

}

static glm::vec4 to_vec4(tira::float2 const& v) { return { v.x, v.y, 0.f, 0.f }; }
static glm::vec4 to_vec4(tira::float3 const& v) { return { v.x, v.y, v.z, 0.f }; }
static glm::vec3 to_vec3(tira::float3 const& v) { return { v.x, v.y, v.z }; }

inline std::vector<GL::Triangle> getTriangleList(tira::Scene const& scene) {
    auto const& objs = scene.accel->objects;
    auto const& mats = scene.materials;
    std::cout << "[GL] Triangle count: " << objs.size() << "\n";

    std::vector<GL::Triangle> list;
    for (auto o : objs) {
        auto t = reinterpret_cast<tira::Triangle*>(o);

        GL::Triangle gt;
        gt.pos[0] = to_vec4(t->pos[0]);
        gt.pos[1] = to_vec4(t->pos[1]);
        gt.pos[2] = to_vec4(t->pos[2]);
        gt.normal = to_vec4(t->normal);
        gt.center = to_vec4(t->center);
        gt.tangent = to_vec4(t->tangent);
        gt.bitangent = to_vec4(t->bitangent);
        gt.vn[0] = to_vec4(t->vn[0]);
        gt.vn[1] = to_vec4(t->vn[1]);
        gt.vn[2] = to_vec4(t->vn[2]);
        gt.vt[0] = to_vec4(t->vt[0]);
        gt.vt[1] = to_vec4(t->vt[1]);
        gt.vt[2] = to_vec4(t->vt[2]);
        gt.e01 = to_vec4(t->e01);
        gt.e02 = to_vec4(t->e02);
        gt.min = to_vec4(t->bound.min);
        gt.max = to_vec4(t->bound.max);
        gt.info.x = static_cast<float>(t->has_vn);
        gt.info.y = static_cast<float>(t->has_vt);
        gt.info.z = t->area;

        for (int i = 0; i < mats.size(); ++i) {
            if (t->material == mats[i]) {
                gt.info.w = static_cast<float>(i);
            }
        }

        list.push_back(gt);
    }

    return list;
}

inline std::vector<GL::Material> getMaterialList(tira::Scene const& scene, std::vector<tira::Texture*>& textures) {
    auto const& mats = scene.materials;
    std::cout << "[GL] Material count: " << mats.size() << "\n";

    std::vector<GL::Material> list;
    for (auto m : mats) {
        if (m->type == tira::MaterialClassType::BlinnPhong) {
            auto bm = reinterpret_cast<tira::BlinnPhongMaterial*>(m);

            GL::Material gm;
            gm.emission = to_vec4(bm->emission);
            gm.emission.w = bm->emissive;
            gm.diffuse = to_vec4(bm->diffuse);
            gm.specular = to_vec4(bm->specular);
            gm.transmittance = to_vec4(bm->transmittance);
            gm.info.x = bm->shininess;
            gm.info.y = bm->ior;
            gm.info.z = 0;
            gm.info.w = -1;

            if (bm->diffuse_texture) {
                gm.info.w = textures.size();
                textures.push_back(bm->diffuse_texture);
            }

            list.push_back(gm);
        }
        else {
            auto bm = reinterpret_cast<tira::GlassMaterial*>(m);

            GL::Material gm;
            gm.emission = to_vec4(bm->emission);
            gm.emission.w = bm->emissive;
            gm.transmittance = to_vec4(bm->transmittance);
            gm.info.y = bm->ior;
            gm.info.z = 1;

            list.push_back(gm);
        }
    }

    return list;
}

inline std::vector<GL::BVHNode> getBVHNodeList(tira::Scene const& scene) {
    auto const& bvh = *reinterpret_cast<tira::BVHAccel*>(scene.accel);
    std::cout << "[GL] BVHNode count: " << bvh.nodes.size() << "\n";
    std::cout << "[GL] BVH max height: " << bvh.max_height << std::endl;

    std::vector<GL::BVHNode> list;
    for (auto n : bvh.nodes) {

        GL::BVHNode gn;
        gn.min = to_vec4(n.bound.min);
        gn.max = to_vec4(n.bound.max);
        gn.hit_idx = n.hit_idx;
        gn.miss_idx = n.miss_idx;
        gn.first_prim = n.first_prim;
        gn.prim_count = n.prim_count;

        list.push_back(gn);
    }

    return list;
}

inline std::vector<int> getAreaLights(tira::Scene const& scene) {
    auto const& objs = scene.accel->objects;

    std::vector<int> list;
    for (int i = 0; i < objs.size(); ++i) {
        if (objs[i]->material->emissive) {
            list.push_back(i);
        }
    }

    return list;
}