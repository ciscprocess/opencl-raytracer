float3 lambert_brdf_f(bxdf_t brdf, float3 wo, float3 wi) {
    return brdf.color * INV_PI;
}

float3 lambert_brdf_sample_f(
        bxdf_t brdf,
        float3 wo,
        float3 *wi,
        float2 xi,
        float *pdf) {
    *wi = square_to_hemi(xi, pdf);
    return lambert_brdf_f(brdf, wo, *wi);
}

float lambert_brdf_pdf(float3 wi) {
    return square_to_hemi_pdf(wi);
}

float3 lambert_btdf_f(bxdf_t brdf, float3 wo, float3 wi) {
    return brdf.color * INV_PI;
}

float3 lambert_btdf_sample_f(
        bxdf_t brdf,
        float3 wo,
        float3 *wi,
        float2 xi,
        float *pdf) {
    *wi = square_to_hemi(xi, pdf);
    if (wo.z > 0) wi->z = -wi->z;
    return lambert_brdf_f(brdf, wo, *wi);
}

float lambert_btdf_pdf(float3 wi) {
    return square_to_hemi_pdf((float3)(wi.x, wi.y, -wi.z));
}

