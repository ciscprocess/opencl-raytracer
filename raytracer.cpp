#include "raytracer.h"
#include <iostream>

inline glm::vec3 reinhard_op(glm::vec3 c) {
    return c / (glm::vec3(1) + c);
}

inline glm::vec3 gamma_correct(glm::vec3 c) {
    return glm::pow(c, glm::vec3(1 / 2.2));
}

Raytracer::Raytracer(
        int width,
        int height,
        std::string kernel_file,
        int kernel_iterations) :
    m_harness(width, height, kernel_file, kernel_iterations), m_width(width), m_height(height) {}

void Raytracer::set_scene(Scene &scene) {
    m_scene = scene;
    m_harness.send_scene_to_gpu(m_scene.value().get_cl_scene(), m_scene.value().get_cl_shapes());
}

QImage Raytracer::render(int iterations) {
    std::vector<cl_double4> average(m_width * m_height);
    for (int i = 0; i < iterations; i++) {
        std::cout << "Iteration: " << i+1 << " / " << iterations << ", ";
        auto colors = m_harness.run_kernel();
        for (size_t j = 0; j < average.size(); j++) {
            average[j].x += colors[j].x;
            average[j].y += colors[j].y;
            average[j].z += colors[j].z;
            average[j].w += colors[j].w;
        }
    }

    for (size_t j = 0; j < average.size(); j++) {
        average[j].x /= (double)iterations;
        average[j].y /= (double)iterations;
        average[j].z /= (double)iterations;
        average[j].w /= (double)iterations;
    }

    // Create and pass-by-value since Qt pools the data in the backend anyway.
    QImage render(m_width, m_height, QImage::Format_RGB32);

    for (int x = 0; x < m_width; x++) {
        for (int y = 0; y < m_height; y++) {
            auto color_cl = average[x + y * m_width];
            glm::vec3 color(color_cl.x, color_cl.y, color_cl.z);
            color = gamma_correct(reinhard_op(color));
            render.setPixel(x, m_height - y - 1, qRgb(
                                (int)(color.x * 255),
                                (int)(color.y * 255),
                                (int)(color.z * 255)));
        }
    }

    return render;
}
