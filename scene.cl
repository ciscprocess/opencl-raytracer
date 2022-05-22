typedef struct __attribute__ ((packed)) Scene {
    cam_t camera;
    int shape_count;
} scene_t;

typedef enum ShapeType {
    SPHERE,
    SQUAREPLANE,
    BOX
} shapetype_t;

typedef struct Shape {
    float4 inv_transform[4];
    float4 inv_tr_transform[4];
    shapetype_t type;
    float3 le;
    bsdf_t bsdf;
} shape_t;

typedef struct Intersection {
    int shape_index;
    float3 point;
    float3 normal;
    float t;
} intersection_t;

void solve_quadratic(float A, float B, float C, float *t0, float *t1) {
    float invA = 1.0 / A;
    B *= invA;
    C *= invA;
    float neg_halfB = -B * 0.5;
    float u2 = neg_halfB * neg_halfB - C;
    float u = u2 < 0.0 ? neg_halfB = 0.0 : sqrt(u2);
    *t0 = neg_halfB - u;
    *t1 = neg_halfB + u;
}

//bool sphere_intersect(shape_t sphere, ray_t ray, intersection_t *intersect) {
//    const float RADIUS = 0.5f;
//    ray_t trans_ray = transform_ray(ray,sphere->inv_transform);
//    float3 trans_dir = trans_ray.dir;
//    float3 trans_eye = trans_ray.p;
//    float a = trans_dir.x * trans_dir.x + trans_dir.y * trans_dir.y + trans_dir.z * trans_dir.z;
//    float b = 2.f * (trans_dir.x * trans_eye.x + trans_dir.y * trans_eye.y + trans_dir.z * trans_eye.z);
//    float c = trans_eye.x * trans_eye.x + trans_eye.y * trans_eye.y + trans_eye.z * trans_eye.z - (RADIUS * RADIUS);

//    float t0, t1;
//    solve_quadratic(a, b, c, &t0, &t1);
//    float t = t0 > 0.0 ? t0 : (t1 > 0.f ? t1 : -1.f);
//    intersection_t new_intersect;
//    new_intersect.t = t;
//    new_intersect.normal = matrix_mult(sphere->inv_tr_transform, (float4)(normalize(trans_eye + trans_dir * t), 0.f)).xyz;
//    return t >= 0.f;
//}

int get_face_index(float3 p) {
    int idx = 0;
    float val = -1;
    for (int i = 0; i < 3; i++){
        if (fabs(p[i]) > val){
            idx = i * sign(p[i]);
            val = fabs(p[i]);
        }
    }
    return idx;
}

float3 get_cube_normal(float3 p) {
    int idx = abs(get_face_index(p));
    float3 N = (float3)(0,0,0);
    N[idx] = sign(p[idx]);
    return N;
}

//__constant static float bounds[2] = {-0.5, 0.5};
bool cube_intersect(shape_t *box, ray_t r, intersection_t *isect) {
    float tmin, tmax, tymin, tymax, tzmin, tzmax;
    ray_t r_loc = transform_ray(r, box->inv_transform);

    float3 invdir = (float3)(1 / r_loc.dir.x, 1 / r_loc.dir.y, 1 / r_loc.dir.z);
    int sign[3];
    sign[0] = (r_loc.dir.x < 0);
    sign[1] = (r_loc.dir.y < 0);
    sign[2] = (r_loc.dir.z < 0);

    float bounds[2] = {-0.5, 0.5};
    tmin = (bounds[sign[0]] - r_loc.p.x) * invdir.x;
    tmax = (bounds[1-sign[0]] - r_loc.p.x) * invdir.x;
    tymin = (bounds[sign[1]] - r_loc.p.y) * invdir.y;
    tymax = (bounds[1-sign[1]] - r_loc.p.y) * invdir.y;

    if ((tmin > tymax) || (tymin > tmax))
        return false;

    tmin = max(tymin, tmin);
    tmax = min(tymax, tmax);

    tzmin = (bounds[sign[2]] - r_loc.p.z) * invdir.z;
    tzmax = (bounds[1-sign[2]] - r_loc.p.z) * invdir.z;

    if ((tmin > tzmax) || (tzmin > tmax))
        return false;
//    if (tzmin > tmin)
//        tmin = tzmin;
    tmin = max(tzmin, tmin);
    if (tmin <= 0) return false;
    //InitializeIntersection(isect, tmin, r_loc.origin + tmin*r_loc.direction, r.origin + tmin*r.direction);
    intersection_t new_isect;
    new_isect.t = tmin;
    new_isect.point = r.p + tmin*r.dir;
    float4 local_normal = (float4)(get_cube_normal(r_loc.p + tmin*r_loc.dir), 0.f);
    new_isect.normal = fast_normalize(matrix_mult(box->inv_tr_transform, local_normal).xyz);//get_cube_normal(r_loc.p + tmin*r_loc.dir);
    *isect = new_isect;
    return true;
}

bool squareplane_intersect(shape_t *plane, ray_t ray, intersection_t *intersect) {
    ray_t trans_ray = transform_ray(ray, plane->inv_transform);
    if (fabs(trans_ray.dir.z) == 0) {
        return false;
    }

    float t = -trans_ray.p.z / trans_ray.dir.z;
    intersection_t intersection;
    intersection.t = t;
    intersection.point = ray.p + ray.dir * t;

    // Multiplying inv_tr_transform by (0, 0, 1, 0) is just the 3rd column of inv_tr_transform.
    intersection.normal = plane->inv_transform[2].xyz;//(float3)(plane->inv_tr_transform[0].z, plane->inv_tr_transform[1].z, plane->inv_tr_transform[2].z);
    //intersection.normal = normalize((float3)(0.f, 1.f, 1.f));
    float3 domain_point = trans_ray.p + trans_ray.dir * t;
    *intersect = intersection;
    return fabs(domain_point.x) <= 0.5 & fabs(domain_point.y) <= 0.5;
}

bool sphere_intersect(shape_t *sphere, ray_t ray, intersection_t *intersect) {
    const float RADIUS = 0.5f;

    ray_t trans_ray = transform_ray(ray,sphere->inv_transform);
    float3 trans_dir = trans_ray.dir;
    float3 trans_eye = trans_ray.p;
    float a = trans_dir.x * trans_dir.x + trans_dir.y * trans_dir.y + trans_dir.z * trans_dir.z;
    float b = 2.f * (trans_dir.x * trans_eye.x + trans_dir.y * trans_eye.y + trans_dir.z * trans_eye.z);
    float c = trans_eye.x * trans_eye.x + trans_eye.y * trans_eye.y + trans_eye.z * trans_eye.z - (RADIUS * RADIUS);

    float discriminant = b * b - 4 * a * c;
//    if (discriminant < 0) {
//        return false;
//    }

    float3 dir = ray.dir;
    float3 eye = ray.p;
    float t1 = (-b + sqrt(discriminant)) / (2.f*a), t2 = (-b - sqrt(discriminant)) / (2.f*a);
    float t = t1 >= 0.f && t1 <= t2 ? t1 : t2;

    // Avoid branching at heavy cost.
//    if (t < 0.f) {
//        return false;
//    }

    intersection_t intersection;
    intersection.t = t;
    intersection.point = eye + dir * t;
    intersection.normal = fast_normalize(matrix_mult(sphere->inv_tr_transform, (float4)(trans_eye + trans_dir * t, 0.f)).xyz);
    *intersect = intersection;
    return discriminant >= 0 && t >= 0.f;
}

bool shape_intersection(shape_t *shape, ray_t ray, intersection_t *intersect) {
    switch (shape->type) {
    case SPHERE:
        return sphere_intersect(shape, ray, intersect);
    case SQUAREPLANE:
        return squareplane_intersect(shape, ray, intersect);
    case BOX:
        return cube_intersect(shape, ray, intersect);
    default:
        return false;
    }
}

intersection_t scene_intersect(
        ray_t ray,
        __global const scene_t *scene,
        __global const shape_t *shapes) {
    float closest = 999999.f;
    intersection_t closest_intersection;
    closest_intersection.t = -1;
    for (int i = 0; i < scene->shape_count; i++) {
        shape_t shape = shapes[i];

        intersection_t intersection;
        if (!shape_intersection(&shape, ray, &intersection)) {
            continue;
        }

        intersection.shape_index = i;

        if (intersection.t > 0.f && intersection.t < closest) {
            closest_intersection = intersection;
            closest = intersection.t;
        }
    }

    return closest_intersection;
}
