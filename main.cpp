#include <complex.h>
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
#include <sys/prctl.h>
#include <sys/signal.h>

// user defined: [hpp]
#include "base/base.hpp"
#include "entrypoint.hpp"

// user defined: [cpp]

void (*drawFrameLib)(Context*);
void (*cleanupLib)(Context*);
void (*initVulkanLib)(Context*);
ThreadCtx (*InitContextLib)(Context*);
void (*DeleteContextLib)(Context*);
void (*InitThreadContextLib)(ThreadCtx*);

#define BUF_LEN (10 * (sizeof(struct inotify_event) + NAME_MAX + 1))
#define CONCAT(a, b) a b
#define MAIN_LIB "./build/debug/"
#define MAIN_LIB_FILE "entrypoint.so"

const char* mainLibFile = CONCAT(MAIN_LIB, MAIN_LIB_FILE);
const char* sourceDir = "./";
const char* buildScript = "./build.sh";
int fd;
bool libChanged = false;
u8 notifyBuffer[BUF_LEN] __attribute__((aligned(__alignof__(struct inotify_event))));

void
setupChildDeathSignal()
{
    if (prctl(PR_SET_PDEATHSIG, SIGTERM) == -1)
    {
        std::cerr << "prctl failed: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }

    // Method 2: Check if parent died during prctl setup
    if (getppid() == 1)
    {
        std::cerr << "Parent died before prctl setup" << std::endl;
        exit(EXIT_FAILURE);
    }
}

// NOTE: A bug in system might cause the read function to fail in debug mode for some reason
void
listenForFileChanges(const char* srcDir)
{
    printf("Listening for file changes in %s\n", srcDir);
    int srcDirFd = {0};
    int srcDirWd = {0};
    u8 buffer[BUF_LEN] __attribute__((aligned(__alignof__(struct inotify_event))));
    ssize_t len;

    srcDirFd = inotify_init();
    if (srcDirFd < 0)
    {
        printf("Failed to initialize inotify: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    srcDirWd = inotify_add_watch(srcDirFd, srcDir, IN_CLOSE_WRITE);
    if (srcDirWd < 0)
    {
        printf("Failed to add watch to inotify: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    while ((len = read(srcDirFd, buffer, BUF_LEN)) > 0)
    {
        u8* buf = &buffer[0];
        struct inotify_event* event;
        while (buf < buffer + len)
        {
            event = (struct inotify_event*)buf;
            if (event->mask & IN_CLOSE_WRITE)
            {
                int procRes = system(buildScript);
                printf("Process id is: %d", getpid());
                if (procRes == EXIT_FAILURE)
                {
                    printf("Failed to run build script: %s", strerror(errno));
                    exit(EXIT_FAILURE);
                }
            }
            buf += sizeof(struct inotify_event) + event->len;
        }
    }
}

void*
loadLibrary()
{
    void* entryHandle = dlopen(mainLibFile, RTLD_NOW | RTLD_LOCAL);
    if (!entryHandle)
    {
        printf("Failed to load entrypoint.so: %s", dlerror());
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

    DeleteContextLib = (void (*)(Context*))dlsym(entryHandle, "DeleteContext");
    if (!DeleteContextLib)
    {
        printf("Failed to load DeleteContext function: %s", dlerror());
        exit(EXIT_FAILURE);
    }

    InitThreadContextLib = (void (*)(ThreadCtx*))dlsym(entryHandle, "InitThreadContext");
    if (!InitThreadContextLib)
    {
        printf("Failed to load SetThreadContext function: %s", dlerror());
        exit(EXIT_FAILURE);
    }

    cleanupLib = (void (*)(Context*))dlsym(entryHandle, "cleanup");
    if (!cleanupLib)
    {
        printf("Failed to load cleanup: %s", dlerror());
        exit(EXIT_FAILURE);
    }

    {
        InitContextLib = (ThreadCtx(*)(Context*))dlsym(entryHandle, "InitContext");
        if (!InitContextLib)
        {
            printf("Failed to load InitContext: %s", dlerror());
            exit(EXIT_FAILURE);
        }
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
    (void)signo;
    (void)info;
    (void)context;

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
framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
    (void)width;
    (void)height;

    auto context = reinterpret_cast<Context*>(glfwGetWindowUserPointer(window));
    context->vulkanContext->framebufferResized = 1;
}

void
initWindow(Context* context)
{
    VulkanContext* vulkanContext = context->vulkanContext;

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    vulkanContext->window = glfwCreateWindow(800, 600, "Vulkan", nullptr, nullptr);
    glfwSetWindowUserPointer(vulkanContext->window, context);
    glfwSetFramebufferSizeCallback(vulkanContext->window, framebufferResizeCallback);
}

void
run()
{
    VulkanContext vulkanContext = {};
    ProfilingContext profilingContext = {};
    GlyphAtlas glyphAtlas = {};
    BoxContext rect = {};
    UI_IO input = {};
    Context context = {&vulkanContext, &profilingContext, &glyphAtlas, &rect, &input, 0, 0, 0};

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

    int childPid = fork();
    switch (childPid)
    {
    case -1:
        perror("fork");
        exit(EXIT_FAILURE);
    case 0:
        setupChildDeathSignal();
        listenForFileChanges(sourceDir);
        break;
    default:
        setpgid(childPid, childPid);
        break;
    }

#else
    initVulkanLib = initVulkan;
    drawFrameLib = drawFrame;
    cleanupLib = cleanup;
    InitContextLib = InitContext;
    InitThreadContextLib = InitThreadContext;
    DeleteContextLib = DeleteContext;

#endif
    ThreadCtx ctx = InitContextLib(&context);
    InitThreadContextLib(&ctx);
    initWindow(&context);
    initVulkanLib(&context);
    while (!glfwWindowShouldClose(vulkanContext.window))
    {
        glfwPollEvents();

        glfwGetCursorPos(vulkanContext.window, &input.mousePosition.x, &input.mousePosition.y);
        input.leftClicked =
            glfwGetMouseButton(vulkanContext.window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

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

    DeleteContextLib(&context);
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
