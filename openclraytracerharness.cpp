#include "opencltypes.h"
#include "openclraytracerharness.h"
#include <OpenCL/cl.h>
#include <stdexcept>
#include <fstream>
#include <iostream>
#include <QString>
#include <QFile>
#include <QTextStream>

void assert_success(cl_int code) {
    if (code != CL_SUCCESS) {
        throw std::domain_error("Invalid code");
    }
}

std::string load_kernel(std::string filename) {
    QString text;
    QFile file(filename.c_str());
    if (file.open(QFile::ReadOnly)) {
        QTextStream in(&file);
        text = in.readAll();
        text.append('\0');
    }

    return text.toStdString();
}

OpenCLRayTracerHarness::OpenCLRayTracerHarness(
        int width,
        int height,
        std::string main_file,
        uint32_t iterations)
    : m_width(width), m_height(height), m_ndcs(width * height) {
    m_iterations = (cl_uint)iterations;
    cl_platform_id platform_id = NULL;
    cl_device_id device_id = NULL;
    cl_uint ret_num_devices;
    cl_uint ret_num_platforms;

    assert_success(
        clGetPlatformIDs(1, &platform_id, &ret_num_platforms));
    assert_success(
        clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_DEFAULT, 1, &device_id, &ret_num_devices));

    char driver_version[10];
    size_t size;
    clGetDeviceInfo(device_id, CL_DRIVER_VERSION, sizeof(char*), &driver_version, &size);
    std::cout << "OpenCL Driver version: " << driver_version << ", size: " << size <<  std::endl;

    cl_uint cu_count;
    clGetDeviceInfo(device_id, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cl_uint), &cu_count, NULL);
    std::cout << "Number of compute units: " << cu_count << std::endl;

//    cl_device_id logical_device;
//    const cl_device_partition_property props[] = {CL_DEVICE_PARTITION_EQUALLY, 8, 0};
//    clCreateSubDevices (device_id,
//        props,
//        1,
//        &logical_device,
//        NULL);

    cl_int ret;
    m_context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &ret);
    assert_success(ret);

    m_command_queue = clCreateCommandQueue(m_context, device_id, 0, &ret);
    assert_success(ret);

    m_scene_mem_obj = clCreateBuffer(m_context, CL_MEM_READ_ONLY, sizeof(scene_t), NULL, &ret);
    assert_success(ret);

    //m_shapes_mem_obj = clCreateBuffer(m_context, CL_MEM_READ_ONLY, scene.get_primitives().size() * sizeof(shape_t), NULL, &ret);
    m_ndcs_mem_obj = clCreateBuffer(m_context, CL_MEM_READ_ONLY,  width * height * sizeof(cl_float2), NULL, &ret);
    assert_success(ret);

    m_colors_mem_obj = clCreateBuffer(m_context, CL_MEM_READ_WRITE,  width * height * sizeof(cl_float3), NULL, &ret);
    assert_success(ret);

    m_rng_seed_mem_obj = clCreateBuffer(m_context, CL_MEM_READ_WRITE,  width * height * sizeof(cl_ulong), NULL, &ret);
    assert_success(ret);

    m_iterations_mem_obj = clCreateBuffer(m_context, CL_MEM_READ_ONLY, sizeof(cl_uint), NULL, &ret);
    assert_success(ret);

    std::string source_str = load_kernel(main_file);
    const char *source_cstr = source_str.c_str();
    size_t souce_len = source_str.size();
    m_program = clCreateProgramWithSource(m_context, 1, (const char **)&source_cstr, (const size_t *)&souce_len, &ret);
    assert_success(ret);

    // Build the program
    ret = clBuildProgram(m_program, 1, &device_id, "-I /Users/nathankorzekwa/Projects/opencl-raytracer", NULL, NULL);
    if (ret == CL_BUILD_PROGRAM_FAILURE) {
        // Determine the size of the log
        size_t log_size;
        clGetProgramBuildInfo(m_program, device_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);

        // Allocate memory for the log
        char *log = (char *) malloc(log_size);

        // Get the log
        clGetProgramBuildInfo(m_program, device_id, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);

        // Print the log
        std::cerr << log << std::endl;
        free(log);
    } else if (ret == CL_SUCCESS) {
        std::cout << "OpenCL kernel built successfully." << std::endl;
    }

    for (int y = 0; y < height; ++y) {
        float ndc_y = -1 + 2 * (y/(float)width);
        for (int x = 0; x < height; ++x) {
            float ndc_x = -1 + 2 * (x/(float)width);
            cl_float2 clf2;
            clf2.x = ndc_x;
            clf2.y = ndc_y;
            m_ndcs[y * height + x] = clf2;
        }
    }

    m_kernel = clCreateKernel(m_program, "pathtrace", &ret);
    assert_success(ret);
}

void OpenCLRayTracerHarness::send_scene_to_gpu(scene_t scene, std::vector<shape_t> shapes) {
    cl_int ret = clEnqueueWriteBuffer(m_command_queue, m_scene_mem_obj, CL_TRUE, 0, sizeof(scene_t), &scene, 0, NULL, NULL);
    assert_success(ret);

    m_shapes_mem_obj = clCreateBuffer(m_context, CL_MEM_READ_ONLY, scene.shape_count * sizeof(shape_t), NULL, &ret);
    assert_success(ret);

    ret = clEnqueueWriteBuffer(m_command_queue, m_shapes_mem_obj, CL_TRUE, 0, scene.shape_count* sizeof(shape_t), shapes.data(), 0, NULL, NULL);
    assert_success(ret);

    ret = clEnqueueWriteBuffer(m_command_queue, m_ndcs_mem_obj, CL_TRUE, 0, m_width * m_height * sizeof(cl_float2), m_ndcs.data(), 0, NULL, NULL);
    assert_success(ret);

    std::vector<cl_ulong> seeds(m_width * m_height);
    for (size_t i = 0; i < seeds.size(); ++i) {
        seeds[i] = std::rand();
    }

    ret = clEnqueueWriteBuffer(m_command_queue, m_rng_seed_mem_obj, CL_TRUE, 0, m_width * m_height * sizeof(cl_ulong), seeds.data(), 0, NULL, NULL);
    assert_success(ret);

    ret = clEnqueueWriteBuffer(m_command_queue, m_iterations_mem_obj, CL_TRUE, 0, sizeof(cl_uint), &m_iterations, 0, NULL, NULL);
    assert_success(ret);
}

std::vector<cl_float3> OpenCLRayTracerHarness::run_kernel() {
    cl_int ret;
    ret = clSetKernelArg(m_kernel, 0, sizeof(cl_mem), (void *)&m_scene_mem_obj);
    if (ret != CL_SUCCESS) {
        std::cerr << "clSetKernelArg: " << ret << std::endl;
    }
    ret = clSetKernelArg(m_kernel, 1, sizeof(cl_mem), (void *)&m_shapes_mem_obj);
    if (ret != CL_SUCCESS) {
        std::cerr << "clSetKernelArg: " << ret << std::endl;
    }
    ret = clSetKernelArg(m_kernel, 2, sizeof(cl_mem), (void *)&m_ndcs_mem_obj);
    if (ret != CL_SUCCESS) {
        std::cerr << "clSetKernelArg: " << ret << std::endl;
    }
    ret = clSetKernelArg(m_kernel, 3, sizeof(cl_mem), (void *)&m_colors_mem_obj);
    if (ret != CL_SUCCESS) {
        std::cerr << "clSetKernelArg: " << ret << std::endl;
    }
    ret = clSetKernelArg(m_kernel, 4, sizeof(cl_mem), (void *)&m_rng_seed_mem_obj);
    if (ret != CL_SUCCESS) {
        std::cerr << "clSetKernelArg: " << ret << std::endl;
    }

    ret = clSetKernelArg(m_kernel, 5, sizeof(cl_uint), (void *)&m_iterations);
    assert_success(ret);

    size_t global_item_size = m_width * m_height;
    size_t local_item_size = 64;
    auto before = std::chrono::high_resolution_clock::now();

    ret = clEnqueueNDRangeKernel(m_command_queue, m_kernel, 1, NULL,
            &global_item_size, &local_item_size, 0, NULL, NULL);

    if (ret != CL_SUCCESS) {
        std::cerr << "clEnqueueNDRangeKernel: " << ret << std::endl;
    }

    ret = clFinish(m_command_queue);
    if (ret != CL_SUCCESS) {
        std::cerr << "clFinish: " << ret << std::endl;
    }
    auto after = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> dur = after - before;
    std::cout << "FPS: "
              << m_iterations / dur.count()
              << ", Iterations: "
              << m_iterations << std::endl;

    std::vector<cl_float3> colors(m_width * m_height);
    // Read the memory buffer C on the device to the local variable C
    ret = clEnqueueReadBuffer(m_command_queue, m_colors_mem_obj, CL_TRUE, 0,
            m_width * m_height * sizeof(cl_float3), colors.data(), 0, NULL, NULL);
    if (ret != CL_SUCCESS) {
        std::cerr << "clEnqueueReadBuffer: " << ret << std::endl;
    }

    return colors;
}
