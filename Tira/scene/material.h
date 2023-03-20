//
// Created by Ziyi.Lu 2022/12/06
//

#ifndef MATERIAL_H
#define MATERIAL_H

#include <string>
#include <misc/utils.h>
#include <scene/texture.h>

namespace tira {

    enum struct MaterialClassType {
        BlinnPhong,
        DisneyBSDF,
        Glass,
    };

    struct Material {
        std::string name;
        MaterialClassType type;

        bool emissive = false;
        float3 emission = float3::zero();
        bool is_delta = false;

        virtual ~Material() {}

        /**
         * Sample a Material
         * Given an outgoing direction (world space) and a surface normal (world space) along with
         * tangent and bitangent (for anisotropic materials), the sampled incident direction
         * and pdf will be stored in the 'wi' and 'pdf' parameter passed in.
         * \param wo outgoing light direction in world space (normalized)
         * \param N normal vector (normalized) of the surface
         * \param wi reference to store sampled incident light direction (normalized)
         * \param tangent tangent vector (normalized) of the surface
         * \param bitangent bitangent vector (normalized) of the surface
         * \param pdf reference to store the pdf of the output incident direction
         * \param is_delta if this sample is sampled from a delta distribution (e.g. refraction or mirror reflection)
         * \return void
         */
        virtual void sample(float3 const& wo, float3 const& N, float3& wi, float3 const& tangent, float3 const& bitangent, float& pdf, bool& is_delta) = 0;

        /**
         * Evaluate a Material's BSDF
         * Given an outgoing direction (world space) incident direction (world space) and
         * surface normal (world space), together with other information (uv, tangent and bitangent,
         * which is included in the Intersection sturcture) to get the evaluation of a material's
         * BSDF function.
         * \param wo outgoing light direction in world space (normalized)
         * \param wi incident light direction in world space (normalized)
         * \param N normal vector (normalized) of the surface
         * \param uv UV coordinates of the surface
         * \param tangent tangent vector of the surface (normalized)
         * \param bitangent bitangent vector of the surface (normalized)
         * \return evaluation of BSDF function
         */
        virtual float3 eval(float3 const& wo, float3 const& wi, float3 const& N, float2 const& uv, float3 const& tangent, float3 const& bitangent) = 0;

        /**
         * Calculate the pdf given directions
         * Given the outgoing direction (world space), incident direction (world space) and
         * surface normal (world space), together with other information (tangent and bitangent,
         * which is included in the Intersection sturcture) to get the pdf.
         * \param wo outgoing light direction in world space (normalized)
         * \param wi incident light direction in world space (normalized)
         * \param N normal vector (normalized) of the surface
         * \param tangent tangent vector of the surface (normalized)
         * \param bitangent bitangent vector of the surface (normalized)
         * \return value of pdf
         */
        virtual float pdf(float3 const& wo, float3 const& wi, float3 const& tangent, float3 const& bitangent, float3 const& N) = 0;

        void sample_uniform(float3 const& wo, float3 const& N, float3& wi, float& pdf) {
            auto dir = random_float3_on_unit_hemisphere();
            wi = local_to_world(dir, N).normalized();
            pdf = INV_TWO_PI;
        }
    };

    struct DisneyBSDFMaterial : Material {
        float3 base_color = { 1.f, 0.f, 1.f };
        float subsurface = 0.f;
        float roughness = .5f;
        float metallic = 0.f;
        float specular = .5f;
        float specular_tint = 0.f;
        float clearcoat = 0.f;
        float clearcoat_gloss = 1.f;
        float anisotropic = 0.f;
        float sheen = 0.f;
        float sheen_tint = .5f;

        float3 disney_diffuse(float NoL, float NoV, float LoH);
        float3 disney_subsurface(float NoL, float NoV, float LoH);
        float3 disney_microfacet_aniso(
            float NoL, float NoV, float NoH, float LoH,
            float3 const& L, float3 const& V, float3 const& H,
            float3 const& tangent, float3 const& bitangent);
        float3 disney_clearcoat(float NoL, float NoV, float NoH, float LoH);
        float3 disney_sheen(float LoH);

        void sample_diffuse(float3& wi, float3 const& wo, float2 const& u, float3 const& N);
        void sample_subsurface(float3& wi, float3 const& wo, float2 const& u, float3 const& N);
        void sample_microfacet_aniso(float3& wi, float3 const& wo, float3 const& tangent, float3 const& bitangent, float2 const& u, float3 const& N);
        void sample_clearcoat(float3& wi, float3 const& wo, float2 const& u, float3 const& N);
        void sample_sheen(float3& wi, float3 const& wo, float2 const& u, float3 const& N);

        float pdf_diffuse(float3 const& wi, float3 const& wo, float3 const& N);
        float pdf_microfacet_aniso(float3 const& wi, float3 const& wo, float3 const& tangent, float3 const& bitangent, float3 const& N);
        float pdf_clearcoat(float3 const& wi, float3 const& wo, float3 const& N);

        virtual void sample(float3 const& wo, float3 const& N, float3& wi, float3 const& tangent, float3 const& bitangent, float& pdf, bool& is_delta) override;
        virtual float3 eval(float3 const& wo, float3 const& wi, float3 const& N, float2 const& uv, float3 const& tangent, float3 const& bitangent) override;
        virtual float pdf(float3 const& wo, float3 const& wi, float3 const& tangent, float3 const& bitangent, float3 const& N) override;
    };

    struct BlinnPhongMaterial : Material {
        float3 diffuse;
        float3 specular;
        float3 transmittance;
        float shininess = 1.f;
        float ior = 1.f;
        Texture* diffuse_texture = nullptr;

        float pd = -1.f; // diffuse probability
        float ps = -1.f; // specular probability
        float pr = -1.f; // refract probability
        void calc_probabilities(float3 const& wo, float3 const& N);

        virtual ~BlinnPhongMaterial() {
            if (diffuse_texture) delete diffuse_texture;
        }

        float3 bsdf_diffuse(float2 const& uv);
        float3 bsdf_specular(float3 const& wo, float3 const& wi, float3 const& N);
        float3 bsdf_refract(float3 const& wo, float3 const& wi, float3 const& N);

        void sample_diffuse(float3& wi, float3 const& wo, float2 const& u, float3 const& N);
        void sample_specular(float3& wi, float3 const& wo, float2 const& u, float3 const& N);
        void sample_refract(float3& wi, float3 const& wo, float2 const& u, float3 const& N);

        float pdf_diffuse(float3 const& wi, float3 const& wo, float3 const& N);
        float pdf_specular(float3 const& wi, float3 const& wo, float3 const& N);
        float pdf_refract(float3 const& wi, float3 const& wo, float3 const& N);

        virtual void sample(float3 const& wo, float3 const& N, float3& wi, float3 const& tangent, float3 const& bitangent, float& pdf, bool& is_delta) override;
        virtual float3 eval(float3 const& wo, float3 const& wi, float3 const& N, float2 const& uv, float3 const& tangent, float3 const& bitangent) override;
        virtual float pdf(float3 const& wo, float3 const& wi, float3 const& tangent, float3 const& bitangent, float3 const& N) override;
    };

    struct GlassMaterial : Material {
        float3 transmittance;
        float ior;

        GlassMaterial() { is_delta = true; }

        virtual void sample(float3 const& wo, float3 const& N, float3& wi, float3 const& tangent, float3 const& bitangent, float& pdf, bool& is_delta) override;
        virtual float3 eval(float3 const& wo, float3 const& wi, float3 const& N, float2 const& uv, float3 const& tangent, float3 const& bitangent) override;
        virtual float pdf(float3 const& wo, float3 const& wi, float3 const& tangent, float3 const& bitangent, float3 const& N) override;
    };

    std::ostream& operator<<(std::ostream& os, DisneyBSDFMaterial const& m);
    std::ostream& operator<<(std::ostream& os, BlinnPhongMaterial const& m);
    std::ostream& operator<<(std::ostream& os, GlassMaterial const& m);

} // namespace tira

#endif
