#include "entrypoint.hpp"
#include <cstdlib>
#include <unistd.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>

#include <dlfcn.h>
#include <iostream>

void (*drawFrameLib)(Context*);
void (*initWindowLib)(Context*);
void (*cleanupLib)(Context*);
void (*initVulkanLib)(Context*);

void*
loadLibrary()
{
    void* entryHandle = dlopen("./entrypoint.so", RTLD_NOW | RTLD_LOCAL);
    if (!entryHandle)
    {
        printf("Failed to load entrypoint.so: %s", dlerror());
        exit(EXIT_FAILURE);
    }

    initWindowLib = (void (*)(Context*))dlsym(entryHandle, "initWindow");
    if (!initWindowLib)
    {
        printf("Failed to load initWindow: %s", dlerror());
        exit(EXIT_FAILURE);
    }

    initVulkanLib = (void (*)(Context*))dlsym(entryHandle, "initVulkan");
    if (!initVulkanLib)
    {
        printf("Failed to load initVulkan: %s", dlerror());
        exit(EXIT_FAILURE);
    }

    drawFrameLib = (void (*)(Context*))dlsym(entryHandle, "drawFrame");
    if (!drawFrameLib)
    {
        printf("Failed to load drawFrame: %s", dlerror());
        exit(EXIT_FAILURE);
    }

    cleanupLib = (void (*)(Context*))dlsym(entryHandle, "cleanup");
    if (!cleanupLib)
    {
        printf("Failed to load cleanup: %s", dlerror());
        exit(EXIT_FAILURE);
    }

    char* err = dlerror();
    if (err != NULL)
    {
        printf("%s", err);
        exit(EXIT_FAILURE);
    }

    return entryHandle;
}

void
run()
{
    VulkanContext vulkanContext = {};
    ProfilingContext profilingContext = {};
    GlyphAtlas glyphAtlas = {};
    Vulkan_Rectangle vulkanRectangle = {};
    Vulkan_GlyphAtlas vulkanGlyphAtlas = {};
    Context context = {&vulkanContext, &profilingContext, &glyphAtlas, &vulkanRectangle,
                       &vulkanGlyphAtlas};

#ifndef PROFILING_ENABLE
    void* entryHandle = loadLibrary();
#else
    initWindowLib = initWindow;
    initVulkanLib = initVulkan;
    drawFrameLib = drawFrame;
    cleanupLib = cleanup;
#endif
    initWindowLib(&context);
    initVulkanLib(&context);
    while (!glfwWindowShouldClose(vulkanContext.window))
    {
        glfwPollEvents();
#ifndef PROFILING_ENABLE
        if (GLFW_PRESS == glfwGetKey(vulkanContext.window, GLFW_KEY_S) &&
            GLFW_PRESS == glfwGetKey(vulkanContext.window, GLFW_KEY_LEFT_CONTROL))
        {
            if (dlclose(entryHandle))
            {
                printf("Failed to close entrypoint.so: %s", dlerror());
                exit(EXIT_FAILURE);
            };
            // window = initWindowLib();
            entryHandle = nullptr;

            entryHandle = loadLibrary();
        }

#endif
        drawFrameLib(&context);
    }

    cleanupLib(&context);
}

// NOTE: Tracydebugger has a dlclose function and it takes precedence over the one in the standard
// library
//  This is only the case when -DTRACY_ENABLE is defined
// NOTE: For some reason dlclose is also not called when using std::map. If this is directly related
// to std::map I do not know
int
main()
{
    try
    {
        run();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
