/* Types first, to avoid circular dependency! */
typedef enum BxDFProps {
    BSDF_REFLECTION = 1 << 0,
    BSDF_TRANSMISSION = 1 << 1,
    BSDF_DIFFUSE = 1 << 2,
    BSDF_GLOSSY = 1 << 3,
    BSDF_SPECULAR = 1 << 4,
    BSDF_ALL = 0x1FF
} bxdfprops_t;

typedef enum BxDFType {
    BRDF_LAMBERT,
    BTDF_LAMBERT,
    BRDF_SPECULAR,
    BTDF_SPECULAR
} bxdftype_t;

typedef struct Bxdf {
    bxdfprops_t props;
    bxdftype_t type;
    float3 color;
    float eta;
} bxdf_t;

typedef struct Bsdf {
    int id;
    size_t bxdfs_count;

    // Allocating a variable-length array would be difficult, and most likely, slow.
    // Most materials won't have more than a few bxdfs so this should be ok. For now.
    bxdf_t bxdfs[5];
} bsdf_t;

#include "bsdf.lambert.cl"
#include "bsdf.specular.cl"

float bxdf_pdf(bxdf_t bxdf, float3 wo, float3 wi) {
    switch (bxdf.type) {
    case BRDF_LAMBERT:
        return lambert_brdf_pdf(wi);
    case BTDF_LAMBERT:
        return lambert_btdf_pdf(wi);
    case BRDF_SPECULAR:
        return specular_brdf_pdf(wi);
    case BTDF_SPECULAR:
        return specular_btdf_pdf(wi);
    default:
        return 0.f;
    }

    return 0.f;
}

float3 bxdf_f(bxdf_t bxdf, float3 wo, float3 wi) {
    switch (bxdf.type) {
    case BRDF_LAMBERT:
        return lambert_brdf_f(bxdf, wo, wi);
    case BTDF_LAMBERT:
        return lambert_btdf_f(bxdf, wo, wi);
    case BRDF_SPECULAR:
        return specular_brdf_f(bxdf, wo, wi);
    case BTDF_SPECULAR:
        return specular_btdf_f(bxdf, wo, wi);
    default:
        return (float3)(0, 0, 0);
    }

    return (float3)(0, 0, 0);
}

float3 bxdf_sample_f(bxdf_t bxdf, float3 wo, float3 *wi, float2 xi, float *pdf) {
    switch (bxdf.type) {
    case BRDF_LAMBERT:
        return lambert_brdf_sample_f(bxdf, wo, wi, xi, pdf);
    case BTDF_LAMBERT:
        return lambert_btdf_sample_f(bxdf, wo, wi, xi, pdf);
    case BRDF_SPECULAR:
        return specular_brdf_sample_f(bxdf, wo, wi, xi, pdf);
    case BTDF_SPECULAR:
        return specular_btdf_sample_f(bxdf, wo, wi, xi, pdf);
    default:
        return (float3)(0, 0, 0);
    }

    return (float3)(0, 0, 0);
}

float3 bsdf_sample_f(
        __global bsdf_t *bsdf,
        float3 nor,
        float3 woW,
        float3 *wiW,
        float2 xi,
        float *pdf) {
    int matching_comps = bsdf->bxdfs_count;

    // Choose a BxDF at random from the set of candidates:
    int comp = min((int)floor(xi[0] * matching_comps), matching_comps - 1);
    bxdf_t bxdf = bsdf->bxdfs[comp];

    // Remap BxDF sample xi to [0,1) in x and y
    // If we don't do this, then we bias the _wi_ sample
    // we'll get from BxDF::Sample_f, e.g. if we have two
    // BxDFs each with a probability of 0.5 of being chosen, then
    // when we sample the first BxDF (xi[0] = [0, 0.5)) we'll ALWAYS
    // use a value between 0 and 0.5 to generate our wi for that BxDF.
    float2 xi2 = (float2)(min(xi[0] * matching_comps - comp, OneMinusEpsilon), xi[1]);

    // Sample chosen BxDF
    float3 wi;

    float3 world_to_tangent[3];
    create_world_to_tangent(nor, world_to_tangent);

    float3 wo = matrix_mult3(world_to_tangent, woW);
    //if (wo.z == 0) return Color3f(0.f); // The tangent-space wo is perpendicular to the normal,
    // so Lambert's law reduces its contribution to 0.

    *pdf = 0;

    float3 f = bxdf_sample_f(bxdf, wo, &wi, xi2, pdf);
    //if (*pdf == 0) return (float3)(0.f, 0.f, 0.f);

   // float3 tangent_to_world[3];
   // create_tangent_to_world(nor, tangent_to_world);

    *wiW = matrix_mult3_tr(world_to_tangent, wi);

//    if (!(bxdf.props & BSDF_SPECULAR)) {
//        *pdf = 0;
//        for (int i = 0; i < bsdf->bxdfs_count; ++i) {
//            *pdf +=  bxdf_pdf(bsdf->bxdfs[i], wo, wi);
//        }
//        *pdf /= matching_comps;
//    }

//    if (!(bxdf.props & BSDF_SPECULAR)) {
//        f = (float3)(0.f, 0.f, 0.f);
//        bool reflect = dot(*wiW, nor) * dot(woW, nor) > 0;
//        for (int i = 0; i < bsdf->bxdfs_count; ++i) {
//            if ((reflect && (bsdf->bxdfs[i].props & BSDF_REFLECTION)) || (!reflect && (bsdf->bxdfs[i].props & BSDF_TRANSMISSION))) {
//                f += bxdf_f(bsdf->bxdfs[i], wo, wi);
//            }
//        }
//    }

    return f;
}
