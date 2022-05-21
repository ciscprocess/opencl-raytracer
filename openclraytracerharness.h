#ifndef OPENCLRAYTRACERHARNESS_H
#define OPENCLRAYTRACERHARNESS_H
#include <OpenCL/cl.h>
#include <string>
#include <vector>
#include "opencltypes.h"

class OpenCLRayTracerHarness
{
public:
    OpenCLRayTracerHarness(int width, int height, std::string main_file, uint32_t iterations);
    void send_scene_to_gpu(scene_t scene, std::vector<shape_t> shapes);
    std::vector<cl_float3> run_kernel();
private:
    int m_width, m_height;
    std::vector<cl_float2> m_ndcs;
    cl_context m_context;
    cl_command_queue m_command_queue;

    cl_mem m_scene_mem_obj;
    cl_mem m_shapes_mem_obj;
    cl_mem m_ndcs_mem_obj;
    cl_mem m_colors_mem_obj;
    cl_mem m_rng_seed_mem_obj;
    cl_mem m_iterations_mem_obj;

    cl_program m_program;
    cl_kernel m_kernel;

    cl_uint m_iterations;
};

#endif // OPENCLRAYTRACERHARNESS_H
