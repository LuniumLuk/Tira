//
// Created by Ziyi.Lu 2022/02/11
//

#include <utils.h>
#include <material.h>

namespace tira {

    static float fresnel_schlick(float NoV, float eta) {
        float R0 = (1 - eta) / (1 + eta);
        R0 *= R0;
        return R0 + (1 - R0) * std::pow(1 - NoV, 5);
    }

    //// Disney Principled BSDF ////

    // Reference: https://github.com/wdas/brdf/blob/main/src/brdfs/disney.brdf

    // Schlick's approximation of Fresnel factor.
    float Schlick_F(float cosTheta) {
        float m = clamp(1.f - cosTheta, 0.f, 1.f);
        float m2 = m * m;
        return m2 * m2 * m;
    }

    // Generalized Trowbridge-Reitz Microfacet Distribution.
    float GTR1(float NoH, float a) {
        if (a >= 1.f) return 1 / PI;
        float a2 = a * a;
        float t = 1.f + (a2 - 1.f) * NoH * NoH;
        return (a2 - 1.f) / (PI * std::log(a2) * t);
    }

    float GTR2(float NoH, float a) {
        float a2 = a * a;
        float t = 1.f + (a2 - 1.f) * NoH * NoH;
        return a2 / (PI * t * t);
    }

    // Anisotropic GTR Microfacet Distribution.
    float GTR2_aniso(float NoH, float HoX, float HoY, float ax, float ay) {
        return 1 / (PI * ax * ay * pow2(pow2(HoX / ax) + pow2(HoY / ay) + NoH * NoH));
    }

    float smithG_GGX(float NoV, float alphaG) {
        float a = alphaG * alphaG;
        float b = NoV * NoV;
        return 1 / (std::abs(NoV) + std::max(std::sqrt(a + b - a * b), EPSILON));
    }

    float smithG_GGX_aniso(float NoV, float VoX, float VoY, float ax, float ay) {
        return 1 / (NoV + std::sqrt(pow2(VoX * ax) + pow2(VoY * ay) + pow2(NoV)));
    }

    float DisneyBSDFMaterial::pdf_diffuse(float3 const& wi, float3 const& wo, float3 const& N) {
        return same_hemisphere(wo, wi, N) ? std::abs(dot(N, wi)) / PI : 0.f;
    }

    float DisneyBSDFMaterial::pdf_microfacet_aniso(float3 const& wi, float3 const& wo, float3 const& tangent, float3 const& bitangent, float3 const& N) {
        if (!same_hemisphere(wo, wi, N)) return 0.f;
        float3 H = normalize(wo + wi);
        float aspect = std::sqrt(1.f - anisotropic * .9f);
        float ax = std::max(sEPSILON, pow2(roughness) / aspect);
        float ay = std::max(sEPSILON, pow2(roughness) * aspect);

        float ax2 = ax * ax;
        float ay2 = ax * ay;

        float HoX = dot(H, tangent);
        float HoY = dot(H, bitangent);
        float NoH = dot(N, H);

        float denom = HoX * HoX / ax2 + HoY * HoY / ay2 + NoH * NoH;
        if (denom == 0.f) return 0.f;
        float pdf = NoH / (PI * ax * ay * denom * denom);

        return pdf / (4.f * dot(wo, H));
    }

    float DisneyBSDFMaterial::pdf_clearcoat(float3 const& wi, float3 const& wo, float3 const& N) {
        if (!same_hemisphere(wo, wi, N)) return 0.f;

        float3 H = normalize(wi + wo);

        float NoH = std::abs(dot(H, N));
        float pdf = GTR1(NoH, lerp(.1f, sEPSILON, clearcoat_gloss)) * NoH;

        return pdf / (4.f * dot(wo, H));
    }

    float3 DisneyBSDFMaterial::disney_diffuse(float NoL, float NoV, float LoH) {
        float FL = Schlick_F(NoL);
        float FV = Schlick_F(NoV);

        float Fd90 = 0.5f + 2.f * LoH * LoH * roughness;
        float Fd = lerp(1.f, Fd90, FL) * lerp(1.f, Fd90, FV);

        return base_color * INV_PI * Fd;
    }

    float3 DisneyBSDFMaterial::disney_subsurface(float NoL, float NoV, float LoH) {
        float FL = Schlick_F(NoL);
        float FV = Schlick_F(NoV);

        float Fss90 = LoH * LoH * roughness;
        float Fss = lerp(1.f, Fss90, FL) * lerp(1.f, Fss90, FV);
        float ss = 1.25f * (Fss * (1 / (NoL + NoV) - .5f) + .5f);

        return base_color * INV_PI * ss;
    }

    float3 DisneyBSDFMaterial::disney_microfacet_aniso(float NoL, float NoV, float NoH, float LoH,
        float3 const& L, float3 const& V, float3 const& H,
        float3 const& tangent, float3 const& bitangent) {
        float Cdlum = color_to_luminance(base_color);

        float3 Ctint = Cdlum > 0.f ? base_color / Cdlum : float3::one();
        float3 Cspec0 = lerp(lerp(float3::one(), Ctint, specular_tint) * specular * .08f, base_color, metallic);

        float aspect = std::sqrt(1.f - anisotropic * .9f);
        float ax = std::max(sEPSILON, pow2(roughness) / aspect);
        float ay = std::max(sEPSILON, pow2(roughness) * aspect);
        float Ds = GTR2_aniso(NoH, dot(H, tangent), dot(H, bitangent), ax, ay);
        float FH = Schlick_F(LoH);
        float3 Fs = lerp(Cspec0, float3::one(), FH);
        float Gs = smithG_GGX_aniso(NoL, dot(L, tangent), dot(L, bitangent), ax, ay) * smithG_GGX_aniso(NoV, dot(V, tangent), dot(V, bitangent), ax, ay);

        return Fs * Gs * Ds;
    }

    float3 DisneyBSDFMaterial::disney_clearcoat(float NoL, float NoV, float NoH, float LoH) {
        float gloss = lerp(.1f, sEPSILON, clearcoat_gloss);
        float Dr = GTR1(std::abs(NoH), gloss);
        float FH = Schlick_F(LoH);
        float Fr = lerp(.04f, 1.f, FH);
        float Gr = smithG_GGX(NoL, .25f) * smithG_GGX(NoV, .25f);

        return { clearcoat * Fr * Gr * Dr };
    }

    float3 DisneyBSDFMaterial::disney_sheen(float LoH) {
        float FH = Schlick_F(LoH);
        float Cdlum = color_to_luminance(base_color);

        float3 Ctint = Cdlum > 0.f ? base_color / Cdlum : float3::one();
        float3 Csheen = lerp(float3::one(), Ctint, sheen_tint);

        return Csheen * FH * sheen;
    }

    void DisneyBSDFMaterial::sample_diffuse(float3& wi, float3 const& wo, float2 const& u, float3 const& N) {
        float3 dir = random_float3_on_unit_hemisphere();
        wi = normalize(local_to_world(dir, N));
    }

    void DisneyBSDFMaterial::sample_subsurface(float3& wi, float3 const& wo, float2 const& u, float3 const& N) {
        float3 dir = random_float3_on_unit_hemisphere();
        wi = normalize(local_to_world(dir, N));

        float3 H = normalize(wo + wi);
        float NoH = dot(N, H);
    }

    void DisneyBSDFMaterial::sample_sheen(float3& wi, float3 const& wo, float2 const& u, float3 const& N) {
        float3 dir = random_float3_on_unit_hemisphere();
        wi = normalize(local_to_world(dir, N));
    }

    void DisneyBSDFMaterial::sample_microfacet_aniso(float3& wi, float3 const& wo, float3 const& tangent, float3 const& bitangent, float2 const& u, float3 const& N) {
        float cos_theta = 0.f;
        float phi = 0.f;

        float aspect = std::sqrt(1.f - anisotropic * .9f);
        float alphax = std::max(sEPSILON, pow2(roughness) / aspect);
        float alphay = std::max(sEPSILON, pow2(roughness) * aspect);

        phi = atan(alphay / alphax * std::tan(2.f * PI * u.y + .5f * PI));

        if (u.y > .5f) phi += PI;
        float sin_phi = std::sin(phi);
        float cos_phi = std::cos(phi);
        float alphax2 = alphax * alphax;
        float alphay2 = alphay * alphay;
        float alpha2 = 1 / (cos_phi * cos_phi / alphax2 + sin_phi * sin_phi / alphay2);
        float tan2theta = alpha2 * u.x / (1.f - u.x);
        cos_theta = 1 / std::sqrt(1.f + tan2theta);

        float sin_theta = std::sqrt(std::max(EPSILON, 1.f - cos_theta * cos_theta));
        float3 wh_local = spherical_to_cartesian(sin_theta, cos_theta, phi);
        float3 wh = tangent * wh_local.x + bitangent * wh_local.y + N * wh_local.z;

        if (!same_hemisphere(wo, wh, N)) {
            wh = -wh;
        }

        wi = transform::reflect(-wo, wh);
    }

    void DisneyBSDFMaterial::sample_clearcoat(float3& wi, float3 const& wo, float2 const& u, float3 const& N) {
        float gloss = lerp(.1f, sEPSILON, clearcoat_gloss);
        float alpha2 = gloss * gloss;
        float cos_theta = std::sqrt(std::max(EPSILON, (1.f - std::pow(alpha2, 1.f - u.x)) / (1.f - alpha2)));
        float sin_theta = std::sqrt(std::max(EPSILON, 1.f - cos_theta * cos_theta));
        float phi = TWO_PI * u.y;

        float3 wh_local = spherical_to_cartesian(sin_theta, cos_theta, phi);
        float3 wh = local_to_world(wh_local, N);

        if (!same_hemisphere(wo, wh, N)) {
            wh = -wh;
        }

        wi = transform::reflect(-wo, wh);
    }

    void DisneyBSDFMaterial::sample(float3 const& wo, float3 const& N, float3& wi, float3 const& tangent, float3 const& bitangent, float& pdf, bool& is_delta) {
        pdf = 0.f;
        wi = float3::zero();

        float2 u = random_float2();
        float rnd = random_float();

        if (rnd < .5f) {
            sample_diffuse(wi, wo, u, N);
        }
        else {
            sample_microfacet_aniso(wi, wo, tangent, bitangent, u, N);
        }

        pdf = this->pdf(wo, wi, tangent, bitangent, N);
    }

    float3 DisneyBSDFMaterial::eval(float3 const& wo, float3 const& wi, float3 const& N, float2 const& uv, float3 const& tangent, float3 const& bitangent) {
        if (!same_hemisphere(wo, wi, N)) return float3::zero();

        float NoL = N.dot(wi);
        float NoV = N.dot(wo);

        if (NoL < 0.f || NoV < 0.f) return float3::zero();

        float3 H = (wo + wi).normalized();
        float NoH = N.dot(H);
        float LoH = wo.dot(H);

        float3 f_diffuse = disney_diffuse(NoL, NoV, LoH);
        float3 f_microfacet = disney_microfacet_aniso(NoL, NoV, NoH, LoH, wi, wo, H, tangent, bitangent);

        return f_diffuse * (1.f - metallic) + f_microfacet;
    }

    float DisneyBSDFMaterial::pdf(float3 const& wo, float3 const& wi, float3 const& tangent, float3 const& bitangent, float3 const& N) {
        float p_diffuse = pdf_diffuse(wi, wo, N);
        float p_microfacet = pdf_microfacet_aniso(wi, wo, tangent, bitangent, N);
        return (p_diffuse + p_microfacet) * .5f;
    }

    //// BlinnPhong BSDF ////

    void BlinnPhongMaterial::calc_probabilities(float3 const& wo, float3 const& N) {
        pd = color_to_luminance(diffuse);
        ps = color_to_luminance(specular);
        pr = 0.f;

        if (std::abs(1 - ior) > EPSILON) {
            float NoV = std::abs(dot(wo, N));
            pr = color_to_luminance(transmittance) * (1.f - fresnel_schlick(NoV, ior));
        }

        float inv_wt = 1 / (pd + ps + pr);
        pd *= inv_wt;
        ps *= inv_wt;
        pr *= inv_wt;
    }

    float3 BlinnPhongMaterial::bsdf_diffuse(float2 const& uv) {
        if (diffuse_texture) {
            return diffuse_texture->sample(uv) * INV_PI;
        }
        else {
            return diffuse * INV_PI;
        }
    }

    float3 BlinnPhongMaterial::bsdf_specular(float3 const& wo, float3 const& wi, float3 const& N) {
        float NoL = dot(N, wi);
        float NoV = dot(N, wo);

        if (NoL > 0 && NoV > 0) {
            // reflection
            float3 refl = transform::reflect(-wo, N);
            float a = std::max(dot(refl, wi), 0.f);
            return specular * (2.f + shininess) * INV_TWO_PI * std::pow(a, shininess);
        }

        return float3::zero();
    }

    float3 BlinnPhongMaterial::bsdf_refract(float3 const& wo, float3 const& wi, float3 const& N) {
        float NoV = dot(N, wo);
        float NoL = dot(N, wi);

        if (NoL * NoV < 0 && pr > EPSILON) {
            return transmittance;
        }

        return float3::zero();
    }

    void BlinnPhongMaterial::sample_diffuse(float3& wi, float3 const& wo, float2 const& u, float3 const& N) {
        float theta = std::acos(std::sqrt(u.x));
        float phi = u.y * TWO_PI;

        float3 dir = spherical_to_cartesian(theta, phi);
        wi = normalize(local_to_world(dir, N));
    }

    void BlinnPhongMaterial::sample_specular(float3& wi, float3 const& wo, float2 const& u, float3 const& N) {
        float cos = std::pow(u.x, 1 / (shininess + 1));
        float3 refl = transform::reflect(-wo, N);
        float theta = std::acos(cos);
        float phi = u.y * TWO_PI;

        float3 dir = spherical_to_cartesian(theta, phi);
        wi = normalize(local_to_world(dir, refl));
    }

    void BlinnPhongMaterial::sample_refract(float3& wi, float3 const& wo, float2 const& u, float3 const& N) {
        bool back_face = dot(N, wo) < 0;
        bool can_refract = true;
        if (back_face) {
            can_refract = transform::refract(-wo, -N, ior, wi);
        }
        else {
            can_refract = transform::refract(-wo, N, 1 / ior, wi);
        }

        if (!can_refract) {
            wi = transform::reflect(-wo, N);
        }
    }

    float BlinnPhongMaterial::pdf_diffuse(float3 const& wi, float3 const& wo, float3 const& N) {
        return INV_PI * dot(wi, N);
    }

    float BlinnPhongMaterial::pdf_specular(float3 const& wi, float3 const& wo, float3 const& N) {
        float3 refl = transform::reflect(-wo, N);
        float cos = std::max(dot(refl, wi), 0.f);
        return (shininess + 1) * INV_TWO_PI * std::pow(cos, shininess);
    }

    float BlinnPhongMaterial::pdf_refract(float3 const& wi, float3 const& wo, float3 const& N) {
        // We only sample one direction, which is perfect refraction.
        return 1.f;
    }

    // Took most of the idea of importance sampling Blinn-Phong model from the paper below:
    //  - https://www.cs.princeton.edu/courses/archive/fall16/cos526/papers/importance.pdf
    // The following articles are helpful dealing with error specular lights:
    //  - https://www.cnblogs.com/warpengine/p/3555028.html
    //  - https://raytracing.github.io/books/RayTracingTheRestOfYourLife.html#mixturedensities
    void BlinnPhongMaterial::sample(float3 const& wo, float3 const& N, float3& wi, float3 const& tangent, float3 const& bitangent, float& pdf, bool& is_delta) {
        pdf = 0.f;
        wi = float3::zero();
        is_delta = false;

        float2 u = random_float2();
        float rnd = random_float();

        calc_probabilities(wo, N);

        // [TODO] Combine Diffuse and Specular to Microfacet term, and sample reflect additionally as a delta distribution
        if (rnd < pd) {
            sample_diffuse(wi, wo, u, N);
        }
        else if (rnd < pd + ps) {
            sample_specular(wi, wo, u, N);
            if (shininess >= BLINN_PHONG_SHININESS_THRESHOLD) {
                is_delta = true;
            }
        }
        else {
            sample_refract(wi, wo, u, N);
            is_delta = true;
        }

        pdf = this->pdf(wo, wi, tangent, bitangent, N);
    }

    float3 BlinnPhongMaterial::eval(float3 const& wo, float3 const& wi, float3 const& N, float2 const& uv, float3 const& tangent, float3 const& bitangent) {
        float NoL = N.dot(wi);
        float NoV = N.dot(wo);

        // Blinn Phong.
        float3 f_diffuse = float3::zero();
        if (NoL > 0 && NoV > 0) {
            f_diffuse = bsdf_diffuse(uv);
        }
        float3 f_specular = bsdf_specular(wo, wi, N);
        float3 f_refract = bsdf_refract(wo, wi, N);

        return f_diffuse + f_specular + f_refract;
    }

    float BlinnPhongMaterial::pdf(float3 const& wo, float3 const& wi, float3 const& tangent, float3 const& bitangent, float3 const& N) {
        calc_probabilities(wo, N);

        float p_diffuse = pdf_diffuse(wi, wo, N);
        float p_specular = pdf_specular(wi, wo, N);
        float p_refract = pdf_refract(wi, wo, N);
        return pd * p_diffuse + ps * p_specular + pr * p_refract;
    }

    //// Glass BSDF ////

    void GlassMaterial::sample(float3 const& wo, float3 const& N, float3& wi, float3 const& tangent, float3 const& bitangent, float& pdf, bool& is_delta) {
        float NoV = dot(N, wo);
        bool back_face = NoV < 0;
        float eta = back_face ? ior : 1 / ior;
        auto const& normal = back_face ? -N : N;

        float cos_theta = dot(wo, normal);
        float sin_theta = std::sqrt(1 - cos_theta * cos_theta);

        bool cannot_refract = eta * sin_theta > 1.f;

        if (cannot_refract || random_float() < fresnel_schlick(cos_theta, eta)) {
            wi = transform::reflect(-wo, normal);
        }
        else {
            wi = transform::refract(-wo, normal, eta);
        }

        pdf = 1.f;
        is_delta = true;
    }

    float3 GlassMaterial::eval(float3 const& wo, float3 const& wi, float3 const& N, float2 const& uv, float3 const& tangent, float3 const& bitangent) {
        return transmittance;
    }

    float GlassMaterial::pdf(float3 const& wo, float3 const& wi, float3 const& tangent, float3 const& bitangent, float3 const& N) {
        return 1.f;
    }

    std::ostream& operator<<(std::ostream& os, DisneyBSDFMaterial const& m) {
        os << "DisneyBSDFMaterial { name: " << m.name
            << ", emission: " << m.emission
            << ", base_color: " << m.base_color
            << ", subsurface: " << m.subsurface
            << ", roughness: " << m.roughness
            << ", metallic: " << m.metallic
            << ", specular: " << m.specular
            << ", specular_tint: " << m.specular_tint
            << ", clearcoat: " << m.clearcoat
            << ", clearcoat_gloss: " << m.clearcoat_gloss
            << ", anisotropic: " << m.anisotropic
            << ", sheen: " << m.sheen
            << ", sheen_tint: " << m.sheen_tint << " }";
        return os;
    }

    std::ostream& operator<<(std::ostream& os, BlinnPhongMaterial const& m) {
        os << "BlinnPhongMaterial { name: " << m.name
            << ", emission: " << m.emission
            << ", diffuse: " << m.diffuse
            << ", specular: " << m.specular
            << ", transmittance: " << m.transmittance
            << ", shininess: " << m.shininess
            << ", ior: " << m.ior << " }";
        return os;
    }

    std::ostream& operator<<(std::ostream& os, GlassMaterial const& m) {
        os << "GlassMaterial { name: " << m.name
            << ", emission: " << m.emission
            << ", transmittance: " << m.transmittance
            << ", ior: " << m.ior << " }";
        return os;
    }

} // namespace tira
