#include "opencltypes.h"
#include <glm/gtc/matrix_transform.hpp>

shape_t create_shape(shapetype_t type, glm::vec3 t, glm::vec3 r, glm::vec3 s) {
    shape_t shape;
    shape.type = type;
    auto transform = glm::mat4(1.f);
    transform = glm::translate(transform, t)
            * glm::rotate(glm::mat4(1.0f), glm::radians(r.x), glm::vec3(1,0,0))
            * glm::rotate(glm::mat4(1.0f), glm::radians(r.y), glm::vec3(0,1,0))
            * glm::rotate(glm::mat4(1.0f), glm::radians(r.z), glm::vec3(0,0,1)) * glm::scale(transform, s);
    copy_mat(shape.inv_transform, glm::inverse(transform));
    copy_mat(shape.inv_tr_transform, glm::transpose(glm::inverse(transform)));
    return shape;
}

void copy_mat(cl_float4 dest[], glm::mat4 src) {
    dest[0] = mk_clf4(src[0][0], src[1][0], src[2][0], src[3][0]);
    dest[1] = mk_clf4(src[0][1], src[1][1], src[2][1], src[3][1]);
    dest[2] = mk_clf4(src[0][2], src[1][2], src[2][2], src[3][2]);
    dest[3] = mk_clf4(src[0][3], src[1][3], src[2][3], src[3][3]);
}

cl_float3 mk_clf3(float x, float y, float z) {
    cl_float3 ret;
    ret.x = x;
    ret.y = y;
    ret.z = z;
    return ret;
}

cl_float4 mk_clf4(float x, float y, float z, float w) {
    cl_float4 ret;
    ret.x = x;
    ret.y = y;
    ret.z = z;
    ret.w = w;
    return ret;
}
