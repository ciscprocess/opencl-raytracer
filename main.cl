#include "rt_math.cl"
#include "bsdf.cl"
#include "scene.cl"

float3 li(float2 ndc,
          ray_t ray,
          unsigned long *rng_state,
          __global const scene_t *scene,
          __global const shape_t *shapes) {
    float3 scale = (float3)(1.f, 1.f, 1.f), total_radiance = (float3)(0.f, 0.f, 0.f);
    float3 wo = (float3)(0.f, 0.f, 0.f);

    int depth = 4;
    for (int i = 0; i <= depth; i++) {
        wo = -ray.dir;
        intersection_t current_isect = scene_intersect(ray, scene, shapes);

        if (current_isect.t < 0.f) {
            break;
        }

        //float3 isect_p = ray.p + current_isect.t * ray.dir;
        total_radiance += scale * shapes[current_isect.shape_index].le;

        if (shapes[current_isect.shape_index].bsdf.bxdfs_count == 0) {
            break;
        }

        float3 wi;
        float pdf = 1.f;
        int flags;
        float2 samp = get_xi(rng_state);
        float3 bsdf_f = bsdf_sample_f(
                    &shapes[current_isect.shape_index].bsdf,
                    current_isect.normal,
                    wo,
                    &wi,
                    samp,
                    &pdf);

        if (pdf == 0.f) {
            break;
        }

        //wi = normalize((float3)(0,7.45,0) - current_isect.point)l

        scale *= bsdf_f;
        scale *= fabs(dot(wi, current_isect.normal));
        scale /= pdf;
        //ray = spawn_ray(current_isect.point, wi);
        ray.p = current_isect.point + wi * 0.00001;
        ray.dir = wi;
    }

    return total_radiance;
}

__kernel void pathtrace(
    __global const scene_t *scene,
    __global const shape_t *shapes,
    __global const float2 *ndcs,
    __global float3 *colors,
    __global unsigned long *rngs,
    const unsigned int iterations) {
    int id = get_global_id(0);
    unsigned long rng_state = rngs[id];
    rng(&rng_state);

    float3 accum_color = (float3)(0.f, 0.f, 0.f);
    for (unsigned int i = 0; i < iterations; i++) {
        ray_t ray = raycast(scene->camera, ndcs[id], &rng_state);
        accum_color += li(ndcs[id], ray, &rng_state, scene, shapes);
    }

    colors[id] = accum_color / iterations;

//    float v = get_xi(&rng_state).x;
//    colors[id] = (float3)(v);
//    rngs[id] = rng_state;
}


//    //colors[id] = ray.dir;
//    intersection_t isect = scene_intersect(ray, scene, shapes);
//    if (isect.t >= 0) {
//        colors[id] = (isect.normal + 1) * 0.5;
//    } else {
//        colors[id] = (float3)(0, 1, 1);
//    }

    //unsigned int iterations = *iterations_ptr;

    //float2 ndc = ndcs[id];

//ray_t ray = raycast(scene->camera, ndcs[id], &rng_state);
//intersection_t isect = scene_intersect(ray, scene, shapes);
//if (isect.t >= 0) {
//    colors[id] = (isect.normal + 1) * 0.5;
//} else {
//    colors[id] = (float3)(0, 1, 1);
//}
//colors[id] = ray.dir;
