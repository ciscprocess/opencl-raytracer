#include "scene.h"
#include <glm/gtc/matrix_transform.hpp>

Scene::Scene(
        int width,
        int height,
        float fov,
        float near,
        float far,
        glm::vec3 eye,
        glm::vec3 ref,
        glm::vec3 world_up) {
    auto view_proj =
            glm::perspective(glm::radians(fov), (float)width/height, near + 0.05f, far) *
            glm::lookAt(glm::vec3(eye.x, eye.y, eye.z), ref, world_up);
    auto inv_view_proj = glm::inverse(view_proj);
    copy_mat(m_scene.camera.inv_view_proj, inv_view_proj);

    m_scene.shape_count = m_shapes.size();
    m_scene.camera.eye = mk_clf3(eye.x, eye.y, eye.z);
    m_scene.camera.far = far;
}

void Scene::add_shapes(std::vector<shape_t> shapes) {
    for (auto &shape : shapes) {
        m_shapes.push_back(shape);
    }

    m_scene.shape_count = m_shapes.size();
}
