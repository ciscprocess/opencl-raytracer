#include "pcg.cl"
#define PI               3.14159265358979323
#define TWO_PI           6.28318530717958648
#define FOUR_PI          12.5663706143591729
#define INV_PI           0.31830988618379067
#define INV_TWO_PI       0.15915494309
#define INV_FOUR_PI      0.07957747154594767
#define PI_OVER_TWO      1.57079632679489662
#define ONE_THIRD        0.33333333333333333
#define E                2.71828182845904524
#define OneMinusEpsilon  0.99999994
#define NULL 0

typedef struct __attribute__ ((packed)) Camera {
    float3 eye;
    float far;
    float4 inv_view_proj[4];
} cam_t;

typedef struct Ray {
    float3 dir;
    float3 p;
} ray_t;

inline float rng(unsigned long *state) {
    return _pcg6432_uint(state) * (1.0 / (float)(0xffffffffU));
}

float2 get_xi(unsigned long *state) {
    return (float2)(rng(state), rng(state));
}

inline float3 matrix_mult3(float3 m[3], float3 v) {
    float3 new_v = (float3)(0.f, 0.f, 0.f);
    new_v.x = dot(m[0], v);
    new_v.y = dot(m[1], v);
    new_v.z = dot(m[2], v);
    return new_v;
}

inline float3 matrix_mult43(float4 m[4], float3 v) {
    float3 new_v = (float3)(0.f, 0.f, 0.f);
    new_v.x = m[0].x * v.x + m[0].y * v.y + m[0].z * v.z;
    new_v.y = m[1].x * v.x + m[1].y * v.y + m[1].z * v.z;
    new_v.z = m[2].x * v.x + m[2].y * v.y + m[2].z * v.z;
    return new_v;
}

inline float3 matrix_mult3_tr(float3 m[3], float3 v) {
    float3 new_v = (float3)(0.f, 0.f, 0.f);
    new_v.x = v.x * m[0].x + v.y * m[1].x + v.z * m[2].x;//dot(m[0], v);
    new_v.y = v.x * m[0].y + v.y * m[1].y + v.z * m[2].y;
    new_v.z = v.x * m[0].z + v.y * m[1].z + v.z * m[2].z;
    return new_v;
}

float4 matrix_mult(float4 m[4], float4 v) {
    float4 new_v = (float4)(0.f, 0.f, 0.f, 0.f);
    new_v.x = dot(m[0], v);
    new_v.y = dot(m[1], v);
    new_v.z = dot(m[2], v);
    new_v.w = dot(m[3], v);
    return new_v;
}

ray_t transform_ray(ray_t ray, float4 m[4]) {
    float4 new_p = matrix_mult(m, (float4)(ray.p, 1.f));
    float4 new_dir = matrix_mult(m, (float4)(ray.dir, 0.f));
    ray_t new_ray;
    new_ray.p = new_p.xyz;
    new_ray.dir = new_dir.xyz;
    return new_ray;
}

ray_t raycast(cam_t cam, float2 ndc, unsigned long *rng_state) {
    float2 offset = (float2)((rng(rng_state) - 0.5f) / 256.f, (rng(rng_state) - 0.5f) / 256.f);
    float4 wp = matrix_mult(cam.inv_view_proj, (float4)(ndc.x + offset.x, ndc.y + offset.y, 1.f, 1.f)) * cam.far;
    float3 vec = wp.xyz - cam.eye;

    ray_t ray;
    ray.p = cam.eye;
    ray.dir = normalize(vec);
    return ray;
}

ray_t spawn_ray(float3 pos, float3 wi) {
    ray_t ray;
    ray.p = pos;
    ray.dir = wi;
    return ray;
}

/* ---- SAMPLING stuff ----  */
float2 square_to_disk(float2 xi) {
    float2 offset = 2.f * xi - (float2)(1.f, 1.f);

    // Ternary operator avoids branching. I think most GPU architectures
    // have a specific instruction that makes simple selections like this.
    float large = maxmag(offset.x, offset.y);
    float theta = (large == offset.x) ?
                    PI / 4.f * (offset.y / offset.x) :
                    PI / 2.f - PI / 4.f * (offset.x / offset.y);

    return large * (float2)(cos(theta), sin(theta));
}

float3 square_to_hemi(float2 xi, float *pdf) {
    float3 p = (float3)(square_to_disk(xi), 0.f);
    p.z = sqrt(max(0.f, 1 - p.x * p.x - p.y * p.y));
    *pdf = p.z * INV_PI;
    return p;
}

float square_to_hemi_pdf(float3 p) {
    return p.z * INV_PI;
}

float3 square_to_hemi_uniform(float2 xi, float *pdf) {
    float z = xi.x,
        y = sin(2.f*PI*xi.y) * sqrt(1.f - z*z),
        x = cos(2.f*PI*xi.y) * sqrt(1.f - z*z);
    *pdf = INV_TWO_PI;
    return (float3)(x, y, z);
}

float square_to_hemi_pdf_uniform(float3 xi) {
    return INV_TWO_PI;
}

void coordinate_system(float3 nor, float3 *tan, float3 *bit) {
    // Use an arbitrary vector to make the likelihood of a degenerate crossproduct near 0.
    // Removes the need for a branch. Bit hacky though, revisit for elegance.
    const float4 arb_vec = (float4)(0.109724f, 0.123286f, -0.986287f, 0.f);

    float4 nor4 = (float4)(nor, 0.f);
    float4 tan4 = cross(nor4, arb_vec);
    *bit = normalize(cross(nor4, tan4).xyz);
    *tan = normalize(tan4.xyz);
}

//void coordinate_system( float3 v1, float3 *v2, float3 *v3) {
//    if (fabs(v1.x) > fabs(v1.y))
//            *v2 = (float3)(-v1.z, 0, v1.x) / sqrt(v1.x * v1.x + v1.z * v1.z);
//        else
//            *v2 = (float3)(0, v1.z, -v1.y) / sqrt(v1.y * v1.y + v1.z * v1.z);
//        *v3 = cross(v1, *v2);
//}

void create_tangent_to_world(float3 nor, float3 matrix[3]) {
    float3 tan, bit;
    coordinate_system(nor, &tan, &bit);
    matrix[0] = (float3)(tan.x, bit.x, nor.x);
    matrix[1] = (float3)(tan.y, bit.y, nor.y);
    matrix[2] = (float3)(tan.z, bit.z, nor.z);
}

void create_world_to_tangent(float3 nor, float3 matrix[3]) {
    float3 tan, bit;
    coordinate_system(nor, &tan, &bit);
    matrix[0] = tan;
    matrix[1] = bit;
    matrix[2] = nor;
}
