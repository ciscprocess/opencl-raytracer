#pragma once
#include "openclraytracerharness.h"
#include "scene.h"
#include <QImage>
#include <optional>


class Raytracer
{
public:
    Raytracer(
            int width,
            int height,
            std::string kernel_file,
            int kernel_iterations);

    void set_scene(Scene &scene);
    QImage render(int iterations);
protected:
    OpenCLRayTracerHarness m_harness;
    std::optional<Scene> m_scene;
    int m_width, m_height;
};
