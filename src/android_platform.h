//========================================================================
// GLFW 3.4 - www.glfw.org
//------------------------------------------------------------------------
// Copyright (c) 2016 Google Inc.
// Copyright (c) 2016-2017 Camilla Löwy <elmindreda@glfw.org>
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would
//    be appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such, and must not
//    be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source
//    distribution.
//
//========================================================================

#include <jni.h>

#define GLFW_ANDROID_WINDOW_STATE          _GLFWwindowAndroid android;
#define GLFW_ANDROID_LIBRARY_WINDOW_STATE  _GLFWlibraryAndroid android;
#define GLFW_ANDROID_MONITOR_STATE         _GLFWmonitorAndroid android;

#define GLFW_ANDROID_CONTEXT_STATE
#define GLFW_ANDROID_CURSOR_STATE _GLFWcursorAndroid android;
#define GLFW_ANDROID_LIBRARY_CONTEXT_STATE


typedef VkFlags VkAndroidSurfaceCreateFlagsKHR;

typedef struct VkAndroidSurfaceCreateInfoKHR {
    VkStructureType                   sType;
    const void*                       pNext;
    VkAndroidSurfaceCreateFlagsKHR    flags;
    struct ANativeWindow*                    window;
} VkAndroidSurfaceCreateInfoKHR;

typedef enum VkQueueFlagBits {
    VK_QUEUE_GRAPHICS_BIT = 0x00000001,
    VK_QUEUE_COMPUTE_BIT = 0x00000002,
    VK_QUEUE_TRANSFER_BIT = 0x00000004,
    VK_QUEUE_SPARSE_BINDING_BIT = 0x00000008,
    VK_QUEUE_PROTECTED_BIT = 0x00000010,
    VK_QUEUE_VIDEO_DECODE_BIT_KHR = 0x00000020,
    VK_QUEUE_VIDEO_ENCODE_BIT_KHR = 0x00000040,
    VK_QUEUE_OPTICAL_FLOW_BIT_NV = 0x00000100,
    VK_QUEUE_FLAG_BITS_MAX_ENUM = 0x7FFFFFFF
} VkQueueFlagBits;

typedef struct VkExtent3D {
    uint32_t    width;
    uint32_t    height;
    uint32_t    depth;
} VkExtent3D;

typedef VkFlags VkQueueFlags;

typedef struct VkQueueFamilyProperties {
    VkQueueFlags    queueFlags;
    uint32_t        queueCount;
    uint32_t        timestampValidBits;
    VkExtent3D      minImageTransferGranularity;
} VkQueueFamilyProperties;

typedef VkResult (APIENTRY *PFN_vkCreateAndroidSurfaceKHR)(VkInstance,const VkAndroidSurfaceCreateInfoKHR*,const VkAllocationCallbacks*,VkSurfaceKHR*);
typedef void (APIENTRY *PFN_vkGetPhysicalDeviceQueueFamilyProperties)(VkPhysicalDevice, uint32_t*, VkQueueFamilyProperties*);

typedef int32_t (*ANativeWindow_setBuffersTransform_t)(struct ANativeWindow *_Nonnull window,int32_t transform);

typedef void* (*acquire_egl_handle_t)(const char*);

typedef struct {
    acquire_egl_handle_t egl_acquire;
    const char* egl_path;
    int force_gles_context;
    int override_major_version;
    bool force_recreate_on_resize;
    int disp_width;
    int disp_height;
    int disp_hz;
} pojavexec_renderspec_t;


// Android-specific per-window data
//
typedef struct _GLFWwindowAndroid
{
    int             xpos;
    int             ypos;
    int             width;
    int             height;
    GLFWbool        visible;
    GLFWbool        iconified;
    GLFWbool        maximized;
    GLFWbool        resizable;
    GLFWbool        decorated;
    GLFWbool        floating;
    GLFWbool        transparent;
    float           opacity;
    int             mode;
    EGLint          visualId;
} _GLFWwindowAndroid;

// Android-specific per-monitor data
//
typedef struct _GLFWmonitorAndroid
{
    GLFWgammaramp   ramp;
} _GLFWmonitorAndroid;

// Android-specific global data
//
typedef struct _GLFWlibraryAndroid
{
    double          xcursor;
    double          ycursor;
    _GLFWwindow*    focusedWindow;
    void* pojavexec_handle;
    const pojavexec_renderspec_t* renderspec;
} _GLFWlibraryAndroid;

typedef struct _GLFWcursorAndroid
{
    jobject cursorRef;
} _GLFWcursorAndroid;

void _glfwPollMonitorsAndroid(void);

GLFWbool _glfwConnectAndroid(int platformID, _GLFWplatform* platform);
int _glfwInitAndroid(void);
void _glfwTerminateAndroid(void);

void _glfwFreeMonitorAndroid(_GLFWmonitor* monitor);
void _glfwGetMonitorPosAndroid(_GLFWmonitor* monitor, int* xpos, int* ypos);
void _glfwGetMonitorContentScaleAndroid(_GLFWmonitor* monitor, float* xscale, float* yscale);
void _glfwGetMonitorWorkareaAndroid(_GLFWmonitor* monitor, int* xpos, int* ypos, int* width, int* height);
GLFWvidmode* _glfwGetVideoModesAndroid(_GLFWmonitor* monitor, int* found);
GLFWbool _glfwGetVideoModeAndroid(_GLFWmonitor* monitor, GLFWvidmode* mode);
GLFWbool _glfwGetGammaRampAndroid(_GLFWmonitor* monitor, GLFWgammaramp* ramp);
void _glfwSetGammaRampAndroid(_GLFWmonitor* monitor, const GLFWgammaramp* ramp);

GLFWbool _glfwCreateWindowAndroid(_GLFWwindow* window, const _GLFWwndconfig* wndconfig, const _GLFWctxconfig* ctxconfig, const _GLFWfbconfig* fbconfig);
void _glfwDestroyWindowAndroid(_GLFWwindow* window);
void _glfwSetWindowTitleAndroid(_GLFWwindow* window, const char* title);
void _glfwSetWindowIconAndroid(_GLFWwindow* window, int count, const GLFWimage* images);
void _glfwSetWindowMonitorAndroid(_GLFWwindow* window, _GLFWmonitor* monitor, int xpos, int ypos, int width, int height, int refreshRate);
void _glfwGetWindowPosAndroid(_GLFWwindow* window, int* xpos, int* ypos);
void _glfwSetWindowPosAndroid(_GLFWwindow* window, int xpos, int ypos);
void _glfwGetWindowSizeAndroid(_GLFWwindow* window, int* width, int* height);
void _glfwSetWindowSizeAndroid(_GLFWwindow* window, int width, int height);
void _glfwSetWindowSizeLimitsAndroid(_GLFWwindow* window, int minwidth, int minheight, int maxwidth, int maxheight);
void _glfwSetWindowAspectRatioAndroid(_GLFWwindow* window, int n, int d);
void _glfwGetFramebufferSizeAndroid(_GLFWwindow* window, int* width, int* height);
void _glfwGetWindowFrameSizeAndroid(_GLFWwindow* window, int* left, int* top, int* right, int* bottom);
void _glfwGetWindowContentScaleAndroid(_GLFWwindow* window, float* xscale, float* yscale);
void _glfwIconifyWindowAndroid(_GLFWwindow* window);
void _glfwRestoreWindowAndroid(_GLFWwindow* window);
void _glfwMaximizeWindowAndroid(_GLFWwindow* window);
GLFWbool _glfwWindowMaximizedAndroid(_GLFWwindow* window);
GLFWbool _glfwWindowHoveredAndroid(_GLFWwindow* window);
GLFWbool _glfwFramebufferTransparentAndroid(_GLFWwindow* window);
void _glfwSetWindowResizableAndroid(_GLFWwindow* window, GLFWbool enabled);
void _glfwSetWindowDecoratedAndroid(_GLFWwindow* window, GLFWbool enabled);
void _glfwSetWindowFloatingAndroid(_GLFWwindow* window, GLFWbool enabled);
void _glfwSetWindowMousePassthroughAndroid(_GLFWwindow* window, GLFWbool enabled);
float _glfwGetWindowOpacityAndroid(_GLFWwindow* window);
void _glfwSetWindowOpacityAndroid(_GLFWwindow* window, float opacity);
void _glfwSetRawMouseMotionAndroid(_GLFWwindow *window, GLFWbool enabled);
GLFWbool _glfwRawMouseMotionSupportedAndroid(void);
void _glfwShowWindowAndroid(_GLFWwindow* window);
void _glfwRequestWindowAttentionAndroid(_GLFWwindow* window);
void _glfwHideWindowAndroid(_GLFWwindow* window);
void _glfwFocusWindowAndroid(_GLFWwindow* window);
GLFWbool _glfwWindowFocusedAndroid(_GLFWwindow* window);
GLFWbool _glfwWindowIconifiedAndroid(_GLFWwindow* window);
GLFWbool _glfwWindowVisibleAndroid(_GLFWwindow* window);
void _glfwPollEventsAndroid(void);
void _glfwWaitEventsAndroid(void);
void _glfwWaitEventsTimeoutAndroid(double timeout);
void _glfwPostEmptyEventAndroid(void);
void _glfwGetCursorPosAndroid(_GLFWwindow* window, double* xpos, double* ypos);
void _glfwSetCursorPosAndroid(_GLFWwindow* window, double x, double y);
void _glfwSetCursorModeAndroid(_GLFWwindow* window, int mode);
GLFWbool _glfwCreateCursorAndroid(_GLFWcursor* cursor, const GLFWimage* image, int xhot, int yhot);
GLFWbool _glfwCreateStandardCursorAndroid(_GLFWcursor* cursor, int shape);
void _glfwDestroyCursorAndroid(_GLFWcursor* cursor);
void _glfwSetCursorAndroid(_GLFWwindow* window, _GLFWcursor* cursor);
void _glfwSetClipboardStringAndroid(const char* string);
const char* _glfwGetClipboardStringAndroid(void);
const char* _glfwGetScancodeNameAndroid(int scancode);
int _glfwGetKeyScancodeAndroid(int key);

void _glfwUpdatePreeditCursorRectangleAndroid(_GLFWwindow* window);
void _glfwResetPreeditTextAndroid(_GLFWwindow* window);
void _glfwSetIMEStatusAndroid(_GLFWwindow* window, int active);
int _glfwGetIMEStatusAndroid(_GLFWwindow* window);

EGLenum _glfwGetEGLPlatformAndroid(EGLint** attribs);
EGLNativeDisplayType _glfwGetEGLNativeDisplayAndroid(void);
EGLNativeWindowType _glfwGetEGLNativeWindowAndroid(_GLFWwindow* window);

void _glfwGetRequiredInstanceExtensionsAndroid(char** extensions);
GLFWbool _glfwGetPhysicalDevicePresentationSupportAndroid(VkInstance instance, VkPhysicalDevice device, uint32_t queuefamily);
VkResult _glfwCreateWindowSurfaceAndroid(VkInstance instance, _GLFWwindow* window, const VkAllocationCallbacks* allocator, VkSurfaceKHR* surface);

void _glfwPollMonitorsAndroid(void);
// Note: for a single window ONLY!
int _glfwDisablePrerotationAndroid(struct ANativeWindow* nativeWindow);
void* _glfwLoadVulkanDriverAndroid(void);
