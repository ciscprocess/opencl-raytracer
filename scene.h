#ifndef SCENE_H
#define SCENE_H
#include "opencltypes.h"
#include <vector>
#include <glm/glm.hpp>
class Scene
{
public:
    Scene(
            int width,
            int height,
            float fov,
            float near,
            float far,
            glm::vec3 eye,
            glm::vec3 ref,
            glm::vec3 world_up);

    scene_t get_cl_scene() const { return m_scene; };
    std::vector<shape_t>  get_cl_shapes() const { return m_shapes; }
    void add_shapes(std::vector<shape_t> shapes);
private:
    std::vector<shape_t> m_shapes;
    scene_t m_scene;
};

#endif // SCENE_H
