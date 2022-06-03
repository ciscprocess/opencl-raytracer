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

float3 get_cube_normal2(float3 p) {
    float3 abs_p = fabs(p);
    float max_dim = fmax(abs_p.x, fmax(abs_p.y, abs_p.z));
    float3 N = floor(abs_p / max_dim) * sign(p);
    return N;
}

float3 get_cube_normal3(float3 p) {
    float3 abs_p = fabs(p);
    float max_dim = fmax(abs_p.x, fmax(abs_p.y, abs_p.z));
    float3 N = fmax(sign(abs_p + float3(0.000001f) - max_dim), (float3)(0.f));
    return N;
}

//__constant static float bounds[2] = {-0.5, 0.5};
bool cube_intersect(local shape_t *box, ray_t r, float *t, float3 *local_normal) {
    ray_t r_loc = transform_ray(r, box->inv_transform);

    // This division is slow. Is it really needed?
    float3 inv_dir = native_divide(1.f, r_loc.dir);
    float tx1 = (-0.5 - r_loc.p.x) * inv_dir.x;
    float tx2 = (0.5 - r_loc.p.x) * inv_dir.x;

    float tmin = fmin(tx1, tx2);
    float tmax = fmax(tx1, tx2);

    float ty1 = (-0.5 - r_loc.p.y) * inv_dir.y;
    float ty2 = (0.5 - r_loc.p.y) * inv_dir.y;

    tmin = fmax(tmin, fmin(ty1, ty2));
    tmax = fmin(tmax, fmax(ty1, ty2));

    float tz1 = (-0.5 - r_loc.p.z) * inv_dir.z;
    float tz2 = (0.5 - r_loc.p.z) * inv_dir.z;

    tmin = fmax(tmin, fmin(tz1, tz2));
    tmax = fmin(tmax, fmax(tz1, tz2));

//    intersection_t new_isect;
//    new_isect.t = tmin < 0 ? tmax : tmin;
//    new_isect.point = new_isect.t * r.dir + r.p;
//    new_isect.normal = get_cube_normal(new_isect.t * r_loc.dir + r_loc.p);
//    new_isect.normal = fast_normalize(matrix_mult(box->inv_tr_transform, (float4)(new_isect.normal, 0.f)).xyz);
//    *isect = new_isect;
    *t = tmin < 0 ? tmax : tmin;
    *local_normal = get_cube_normal((*t) * r_loc.dir + r_loc.p);
    return tmax >= tmin;
}

bool squareplane_intersect(local shape_t *plane, ray_t ray, float *t, float3 *local_normal) {
    ray_t trans_ray = transform_ray(ray, plane->inv_transform);
    *t = -trans_ray.p.z / trans_ray.dir.z;

    float3 domain_point = trans_ray.p + trans_ray.dir * (*t);
    *local_normal = (float3)(0, 0, 1);

    return fabs(domain_point.x) <= 0.5 & fabs(domain_point.y) <= 0.5;
}

bool sphere_intersect(local shape_t *sphere, ray_t ray, float *t, float3 *local_normal) {
    const float RADIUS = 0.5f;

    ray_t trans_ray = transform_ray(ray,sphere->inv_transform);
    float3 trans_dir = trans_ray.dir;
    float3 trans_eye = trans_ray.p;
    float a = trans_dir.x * trans_dir.x + trans_dir.y * trans_dir.y + trans_dir.z * trans_dir.z;
    float b = 2.f * (trans_dir.x * trans_eye.x + trans_dir.y * trans_eye.y + trans_dir.z * trans_eye.z);
    float c = trans_eye.x * trans_eye.x + trans_eye.y * trans_eye.y + trans_eye.z * trans_eye.z - (RADIUS * RADIUS);

    float discriminant = b * b - 4 * a * c;
    float t1 = (-b + sqrt(discriminant)) / (2.f*a), t2 = (-b - sqrt(discriminant)) / (2.f*a);
    *t = t1 >= 0.f && t1 <= t2 ? t1 : t2;

    *local_normal = trans_eye + trans_dir * (*t);
    return discriminant >= 0 && (*t) >= 0.f;
}

bool shape_intersection(local shape_t *shape, ray_t ray, float *t, float3 *local_normal) {
    switch (shape->type) {
    case SPHERE:
        return sphere_intersect(shape, ray, t, local_normal);
    case SQUAREPLANE:
        return squareplane_intersect(shape, ray, t, local_normal);
    case BOX:
        return cube_intersect(shape, ray, t, local_normal);
    default:
        return false;
    }
}

intersection_t scene_intersect(
        ray_t ray,
        int shape_count,
        local const shape_t *shapes) {
    float closest = 999999.f;
    float closest_t = -1.f;
    float3 closest_local_normal;
    closest_local_normal.z = 1.f;
    int closest_i = 0;
    for (int i = 0; i < shape_count; i++) {
        //shape_t shape = shapes[i];

        float t;
        float3 local_normal;
        if (!shape_intersection(&shapes[i], ray, &t, &local_normal)) {
            continue;
        }

        bool condition = t > 0.f && t < closest;
        closest_t = condition ? t : closest_t;
        closest_local_normal = condition ? local_normal : closest_local_normal;
        closest_i = condition ? i : closest_i;
    }

    shape_t shape = shapes[closest_i];
    intersection_t intersection;
    intersection.shape_index = closest_i;
    intersection.point = ray.p + ray.dir * closest_t;
    intersection.t = closest_t;
    intersection.normal = fast_normalize(matrix_mult43(shape.inv_tr_transform, closest_local_normal));
    return intersection;
}
