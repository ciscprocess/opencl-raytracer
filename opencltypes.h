#ifndef OPENCLTYPES_H
#define OPENCLTYPES_H
#include <OpenCL/cl.h>
#include <glm/glm.hpp>

/**** Ray 'class' ****/
typedef struct RayCL {
    cl_float3 dir;
    cl_float3 p;
} ray_t;

/**** Camera 'class' ****/
typedef struct __attribute__ ((packed)) CameraCL {
    cl_float3 eye;
    cl_float far;
    cl_float4 inv_view_proj[4];
} cam_t;

/**** Shape ****/
typedef enum ShapeTypeCL {
    SPHERE,
    SQUAREPLANE,
    BOX
} shapetype_t;

typedef enum BxDFPropsCL {
    BSDF_REFLECTION = 1 << 0,
    BSDF_TRANSMISSION = 1 << 1,
    BSDF_DIFFUSE = 1 << 2,
    BSDF_GLOSSY = 1 << 3,
    BSDF_SPECULAR = 1 << 4,
    BSDF_ALL = 0x1FF
} bxdfprops_t;

typedef enum BxDFTypeCL {
    BRDF_LAMBERT,
    BTDF_LAMBERT,
    BRDF_SPECULAR,
    BTDF_SPECULAR
} bxdftype_t;

typedef struct BxdfCL {
    bxdfprops_t props;
    bxdftype_t type;
    cl_float3 color;
    cl_float eta;
} bxdf_t;

typedef struct Bsdf {
    int id;
    size_t bxdfs_count;
    bxdf_t bxdfs[5];
} bsdf_t;

typedef struct ShapeCL {
    cl_float4 inv_transform[4];
    cl_float4 inv_tr_transform[4];
    shapetype_t type;
    cl_float3 le;
    bsdf_t bsdf;
} shape_t;

/**** Intersection ****/
typedef struct IntersectionCL {
    cl_int shape_index;
    cl_float3 point;
    cl_float3 normal;
    cl_float t;
} intersection_t;

/**** Scene ****/
typedef struct __attribute__ ((packed)) SceneCL {
    cam_t camera;
    int shape_count;
} scene_t;

shape_t create_shape(shapetype_t type, glm::vec3 t, glm::vec3 r, glm::vec3 s);
cl_float4 mk_clf4(float x, float y, float z, float w);
cl_float3 mk_clf3(float x, float y, float z);
void copy_mat(cl_float4 dest[4], glm::mat4 src);

#endif // OPENCLTYPES_H
