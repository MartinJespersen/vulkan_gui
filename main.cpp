#include <cstdlib>
#include <stdexcept>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>

#include "entrypoint.hpp"
#include <dlfcn.h>

Context context;

void (*drawFrameLib)(Context*);
void (*initWindowLib)(Context*);
void (*cleanupLib)(Context*);
void (*initVulkanLib)(Context*);

void*
loadLibrary()
{
    void* entryHandle = NULL;
    if (entryHandle != nullptr)
    {
        if (dlclose(entryHandle))
        {
            printf("Failed to close entrypoint.so: %s", dlerror());
            exit(EXIT_FAILURE);
        };

        entryHandle = nullptr;
        initWindowLib(&context);
    }
    entryHandle = dlopen("./entrypoint.so", RTLD_NOW | RTLD_LOCAL);
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

    // initVulkanLib = (void (*)(Context*))dlsym(entryHandle, "initVulkan");
    // if (!initVulkanLib)
    // {
    //     printf("Failed to load initVulkan: %s", dlerror());
    //     exit(EXIT_FAILURE);
    // }

    // drawFrameLib = (void (*)(Context*))dlsym(entryHandle, "drawFrame");
    // if (!drawFrameLib)
    // {
    //     printf("Failed to load drawFrame: %s", dlerror());
    //     exit(EXIT_FAILURE);
    // }

    // cleanupLib = (void (*)(Context*))dlsym(entryHandle, "cleanup");
    // if (!cleanupLib)
    // {
    //     printf("Failed to load cleanup: %s", dlerror());
    //     exit(EXIT_FAILURE);
    // }

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
    void* entryHandle = loadLibrary();

    initWindowLib(&context);
    // initVulkanLib(&context);
    while (!glfwWindowShouldClose(context.window))
    {
        glfwPollEvents();
        if (GLFW_PRESS == glfwGetKey(context.window, GLFW_KEY_S) &&
            GLFW_PRESS == glfwGetKey(context.window, GLFW_KEY_LEFT_CONTROL))
        {
            initWindowLib(&context);
            if (dlclose(entryHandle))
            {
                printf("Failed to close entrypoint.so: %s", dlerror());
                exit(EXIT_FAILURE);
            };

            entryHandle = nullptr;

            entryHandle = loadLibrary();
        }

        // drawFrameLib(&context);
    }

    // vkDeviceWaitIdle(context.device);
    // cleanupLib(&context);
}

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
