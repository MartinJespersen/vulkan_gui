#include "entrypoint.hpp"
#include <cstdlib>
#include <cstring>
#include <sys/types.h>
#include <unistd.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>

#include <dlfcn.h>
#include <fcntl.h>
#include <iostream>
#include <sys/inotify.h>
#include <sys/signal.h>

void (*drawFrameLib)(Context*);
void (*initWindowLib)(Context*);
void (*cleanupLib)(Context*);
void (*initVulkanLib)(Context*);

#define BUF_LEN (10 * (sizeof(struct inotify_event) + NAME_MAX + 1))
#define CONCAT(a, b) a b
#define MAIN_LIB "./build/debug/"
#define MAIN_LIB_FILE "entrypoint.so"

const char* mainLibFile = CONCAT(MAIN_LIB, MAIN_LIB_FILE);
int fd;
bool libChanged = false;
u8 notifyBuffer[BUF_LEN] __attribute__((aligned(__alignof__(struct inotify_event))));

void*
loadLibrary()
{
    void* entryHandle = dlopen(mainLibFile, RTLD_NOW | RTLD_LOCAL);
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

// NOTE: This handler cannot be debugged with gdb when
void
signal_handler(int signo, siginfo_t* info, void* context)
{
    ssize_t len;
    while ((len = read(fd, notifyBuffer, BUF_LEN)) > 0)
    {
        u8* ptr = notifyBuffer;
        while (ptr < notifyBuffer + len)
        {
            struct inotify_event* event = (struct inotify_event*)ptr;
            if (event->len > 0)
            {
                if (event->mask & IN_CLOSE_WRITE)
                {
                    libChanged = true;
                }

                fflush(stdout);
            }
            ptr += sizeof(struct inotify_event) + event->len;
        }
    }
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
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = signal_handler;

    if (sigaction(SIGIO, &sa, NULL) == -1)
    {
        perror("sigaction SIGIO");
        exit(EXIT_FAILURE);
    }

    fd = inotify_init1(IN_NONBLOCK);
    if (fd < 0)
    {
        printf("Failed to initialize inotify: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (fcntl(fd, F_SETOWN, getpid()) == -1)
    {
        perror("fcntl F_SETOWN");
        exit(EXIT_FAILURE);
    }

    if (fcntl(fd, F_SETFL, O_ASYNC | O_NONBLOCK) == -1)
    {
        perror("fcntl F_SETFL");
        exit(EXIT_FAILURE);
    }
    int wd = inotify_add_watch(fd, MAIN_LIB,
                               IN_CLOSE_WRITE); // IN_CLOSE_WRITE instead of IN_MODIFY or IN_CREATE
                                                // as the file might not be fully written to disk
    if (wd < 0)
    {
        printf("Failed to add watch to inotify: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

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

        if (libChanged)
        {
            libChanged = false;
            if (dlclose(entryHandle))
            {
                printf("Failed to close entrypoint.so: %s", dlerror());
                exit(EXIT_FAILURE);
            };

            entryHandle = nullptr;

            entryHandle = loadLibrary();
        }

#endif
        drawFrameLib(&context);
    }
#ifndef PROFILING_ENABLE
    inotify_rm_watch(fd, wd);
#endif
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
