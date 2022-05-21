float3 specular_brdf_f(bxdf_t brdf, float3 wo, float3 wi) {
    return (float3)(0.f, 0.f, 0.f);
}

float3 specular_brdf_sample_f(
        bxdf_t brdf,
        float3 wo,
        float3 *wi,
        float2 xi,
        float *pdf) {
    *wi = (float3)(-wo.x, -wo.y, wo.z);
    *pdf = 1.;
    return  brdf.color / fabs(wi->z);
}

float specular_brdf_pdf(float3 wi) {
    return 0;
}

float3 specular_btdf_f(bxdf_t brdf, float3 wo, float3 wi) {
    return (float3)(0.f, 0.f, 0.f);
}

inline bool refract(
        float3 wi,
        float3 n,
        float eta,
        float3 *wt) {
    // Compute cos theta using Snell's law
    float cosThetaI = dot(n, wi);
    float sin2ThetaI = max(0.f, 1.f - cosThetaI * cosThetaI);
    float sin2ThetaT = eta * eta * sin2ThetaI;

    // Handle total internal reflection for transmission

    float cosThetaT = sqrt(1 - sin2ThetaT);
    *wt = eta * -wi + (eta * cosThetaI - cosThetaT) * n;
    return sin2ThetaT < 1;
}

inline float3 face_forward(float3 n, float3 v) {
    return (dot(n, v) < 0.f) ? -n : n;
}

float3 specular_btdf_sample_f(
        bxdf_t brdf,
        float3 wo,
        float3 *wi,
        float2 xi,
        float *pdf) {
    bool entering = wo.z > 0;
    float etaI = entering ? 1.f : brdf.eta;
    float etaT = entering ? brdf.eta : 1.f;

    if (!refract(wo, face_forward((float3)(0.f, 0.f, 1.f), wo), etaI / etaT, wi)) {
        return (float3)(0.f, 0.f, 0.f);
    }

    *pdf = 1;
    float3 ft = brdf.color;
    return ft / fabs(wi->z);
}

float specular_btdf_pdf(float3 wi) {
    return 0;
}
