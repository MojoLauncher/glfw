//========================================================================
// GLFW 3.4 - www.glfw.org
//------------------------------------------------------------------------
// Copyright (c) 2016 Google Inc.
// Copyright (c) 2016-2019 Camilla Löwy <elmindreda@glfw.org>
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

#include "internal.h"
#include "android_egl_context_hook.h"
#include "android_input_queue.h"

#include <stdlib.h>

#include <poll.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <jni.h>
#include <android/input.h>
#include <android/log.h>
#include <android/native_window.h>
#include <math.h>
#include <android/native_window_jni.h>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "android_window", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "android_window", __VA_ARGS__)

#define GLFW_ANDROID_WINDOW_MODE_UNDEFINED 0
#define GLFW_ANDROID_WINDOW_MODE_PBUFFER 1
#define GLFW_ANDROID_WINDOW_MODE_SURFACE 2

#define GLFW_ANDROID_EVENT_TYPE_EMPTY 0
#define GLFW_ANDROID_EVENT_TYPE_MOUSE_BUTTONS 2
#define GLFW_ANDROID_EVENT_TYPE_KEYBOARD_KEY 3
#define GLFW_ANDROID_EVENT_TYPE_UNICODE_CHARS 4
#define GLFW_ANDROID_EVENT_TYPE_MOUSE_SCROLL 5
#define GLFW_ANDROID_EVENT_TYPE_JOYSTICK_STATE 6

#define FLAG_MOUSE_POS (1 >> 0)
#define FLAG_APP_FOCUS (1 >> 1)

static _GLFWwindow *surfaceOwner = NULL;
static _Atomic GLFWbool surfaceDestroyed = true;
static _Atomic GLFWbool surfaceUpdated = false;
static _Atomic GLFWbool ownedByVulkan = false;
static _Atomic GLFWbool surfaceInUse = false;
static struct ANativeWindow* nativeWindow = NULL;
static _Atomic uint32_t update_flags = 0;

const char* clipboard_string = NULL;
jobject clipboard_string_ref = NULL;

static pthread_mutex_t nw_egl_mutex;
static pthread_cond_t nw_egl_cond;
static pthread_mutex_t nw_vulkan_mutex;
static pthread_cond_t nw_vulkan_cond;

static struct {
    double x, y;
} cursor_unscaled;

static struct {
    JavaVM *vm;
    jclass glfw_class;
    jmethodID method_receiveGrabState;
    jmethodID method_receiveCursorPos;
    jmethodID method_loadCursor;
    jmethodID method_useCursor;
    jmethodID method_getClipboardString;
    jmethodID method_setClipboardString;
    jmethodID method_enableDirectGamepad;
} jni;

static _Thread_local struct {
    JNIEnv *env;
    bool attached;
} jni_tl = {
        .attached = false
};

static queue_top_t input_queue;

static void ensure_comm_connected() {
    if(jni_tl.attached) return;
    JNIEnv *env;
    jint error = (*jni.vm)->GetEnv(jni.vm, (void**)&env, JNI_VERSION_1_6);
    if(error == JNI_EDETACHED) {
        error = (*jni.vm)->AttachCurrentThreadAsDaemon(jni.vm, &env, NULL);
    }
    if(error != JNI_OK) {
        LOGE("JNI connection failed: %i", error);
        abort();
    }
    jni_tl.env = env;
    jni_tl.attached = true;
}

static int translate_android_key(int32_t androidKeyCode) {
    switch (androidKeyCode) {
        case AKEYCODE_SPACE: return GLFW_KEY_SPACE;
        case AKEYCODE_APOSTROPHE: return GLFW_KEY_APOSTROPHE;
        case AKEYCODE_COMMA: return GLFW_KEY_COMMA;
        case AKEYCODE_MINUS: return GLFW_KEY_MINUS;
        case AKEYCODE_PERIOD: return GLFW_KEY_PERIOD;
        case AKEYCODE_SLASH: return GLFW_KEY_SLASH;
        case AKEYCODE_0: return GLFW_KEY_0;
        case AKEYCODE_1: return GLFW_KEY_1;
        case AKEYCODE_2: return GLFW_KEY_2;
        case AKEYCODE_3: return GLFW_KEY_3;
        case AKEYCODE_4: return GLFW_KEY_4;
        case AKEYCODE_5: return GLFW_KEY_5;
        case AKEYCODE_6: return GLFW_KEY_6;
        case AKEYCODE_7: return GLFW_KEY_7;
        case AKEYCODE_8: return GLFW_KEY_8;
        case AKEYCODE_9: return GLFW_KEY_9;
        case AKEYCODE_SEMICOLON: return GLFW_KEY_SEMICOLON;
        case AKEYCODE_EQUALS: return GLFW_KEY_EQUAL;
        case AKEYCODE_A: return GLFW_KEY_A;
        case AKEYCODE_B: return GLFW_KEY_B;
        case AKEYCODE_C: return GLFW_KEY_C;
        case AKEYCODE_D: return GLFW_KEY_D;
        case AKEYCODE_E: return GLFW_KEY_E;
        case AKEYCODE_F: return GLFW_KEY_F;
        case AKEYCODE_G: return GLFW_KEY_G;
        case AKEYCODE_H: return GLFW_KEY_H;
        case AKEYCODE_I: return GLFW_KEY_I;
        case AKEYCODE_J: return GLFW_KEY_J;
        case AKEYCODE_K: return GLFW_KEY_K;
        case AKEYCODE_L: return GLFW_KEY_L;
        case AKEYCODE_M: return GLFW_KEY_M;
        case AKEYCODE_N: return GLFW_KEY_N;
        case AKEYCODE_O: return GLFW_KEY_O;
        case AKEYCODE_P: return GLFW_KEY_P;
        case AKEYCODE_Q: return GLFW_KEY_Q;
        case AKEYCODE_R: return GLFW_KEY_R;
        case AKEYCODE_S: return GLFW_KEY_S;
        case AKEYCODE_T: return GLFW_KEY_T;
        case AKEYCODE_U: return GLFW_KEY_U;
        case AKEYCODE_V: return GLFW_KEY_V;
        case AKEYCODE_W: return GLFW_KEY_W;
        case AKEYCODE_X: return GLFW_KEY_X;
        case AKEYCODE_Y: return GLFW_KEY_Y;
        case AKEYCODE_Z: return GLFW_KEY_Z;
        case AKEYCODE_LEFT_BRACKET: return GLFW_KEY_LEFT_BRACKET;
        case AKEYCODE_BACKSLASH: return GLFW_KEY_BACKSLASH;
        case AKEYCODE_RIGHT_BRACKET: return GLFW_KEY_RIGHT_BRACKET;
        case AKEYCODE_GRAVE: return GLFW_KEY_GRAVE_ACCENT;
        case AKEYCODE_ESCAPE: return GLFW_KEY_ESCAPE;
        case AKEYCODE_ENTER: return GLFW_KEY_ENTER;
        case AKEYCODE_TAB: return GLFW_KEY_TAB;
        case AKEYCODE_DEL: return GLFW_KEY_BACKSPACE;
        case AKEYCODE_INSERT: return GLFW_KEY_INSERT;
        case AKEYCODE_FORWARD_DEL: return GLFW_KEY_DELETE;
        case AKEYCODE_DPAD_RIGHT: return GLFW_KEY_RIGHT;
        case AKEYCODE_DPAD_LEFT: return GLFW_KEY_LEFT;
        case AKEYCODE_DPAD_UP: return GLFW_KEY_UP;
        case AKEYCODE_DPAD_DOWN: return GLFW_KEY_DOWN;
        case AKEYCODE_PAGE_UP: return GLFW_KEY_PAGE_UP;
        case AKEYCODE_PAGE_DOWN: return GLFW_KEY_PAGE_DOWN;
        case AKEYCODE_MOVE_HOME: return GLFW_KEY_HOME;
        case AKEYCODE_MOVE_END: return GLFW_KEY_END;
        case AKEYCODE_CAPS_LOCK: return GLFW_KEY_CAPS_LOCK;
        case AKEYCODE_SCROLL_LOCK: return GLFW_KEY_SCROLL_LOCK;
        case AKEYCODE_NUM_LOCK: return GLFW_KEY_NUM_LOCK;
        case AKEYCODE_SYSRQ: return GLFW_KEY_PRINT_SCREEN;
        case AKEYCODE_BREAK: return GLFW_KEY_PAUSE;
        case AKEYCODE_F1: return GLFW_KEY_F1;
        case AKEYCODE_F2: return GLFW_KEY_F2;
        case AKEYCODE_F3: return GLFW_KEY_F3;
        case AKEYCODE_F4: return GLFW_KEY_F4;
        case AKEYCODE_F5: return GLFW_KEY_F5;
        case AKEYCODE_F6: return GLFW_KEY_F6;
        case AKEYCODE_F7: return GLFW_KEY_F7;
        case AKEYCODE_F8: return GLFW_KEY_F8;
        case AKEYCODE_F9: return GLFW_KEY_F9;
        case AKEYCODE_F10: return GLFW_KEY_F10;
        case AKEYCODE_F11: return GLFW_KEY_F11;
        case AKEYCODE_F12: return GLFW_KEY_F12;
        case AKEYCODE_NUMPAD_0: return GLFW_KEY_KP_0;
        case AKEYCODE_NUMPAD_1: return GLFW_KEY_KP_1;
        case AKEYCODE_NUMPAD_2: return GLFW_KEY_KP_2;
        case AKEYCODE_NUMPAD_3: return GLFW_KEY_KP_3;
        case AKEYCODE_NUMPAD_4: return GLFW_KEY_KP_4;
        case AKEYCODE_NUMPAD_5: return GLFW_KEY_KP_5;
        case AKEYCODE_NUMPAD_6: return GLFW_KEY_KP_6;
        case AKEYCODE_NUMPAD_7: return GLFW_KEY_KP_7;
        case AKEYCODE_NUMPAD_8: return GLFW_KEY_KP_8;
        case AKEYCODE_NUMPAD_9: return GLFW_KEY_KP_9;
        case AKEYCODE_NUMPAD_DOT: return GLFW_KEY_KP_DECIMAL;
        case AKEYCODE_NUMPAD_DIVIDE: return GLFW_KEY_KP_DIVIDE;
        case AKEYCODE_NUMPAD_SUBTRACT: return GLFW_KEY_KP_SUBTRACT;
        case AKEYCODE_NUMPAD_ADD: return GLFW_KEY_KP_ADD;
        case AKEYCODE_NUMPAD_ENTER: return GLFW_KEY_KP_ENTER;
        case AKEYCODE_NUMPAD_EQUALS: return GLFW_KEY_KP_EQUAL;
        case AKEYCODE_SHIFT_LEFT: return GLFW_KEY_LEFT_SHIFT;
        case AKEYCODE_CTRL_LEFT: return GLFW_KEY_LEFT_CONTROL;
        case AKEYCODE_ALT_LEFT: return GLFW_KEY_LEFT_ALT;
        case AKEYCODE_SHIFT_RIGHT: return GLFW_KEY_RIGHT_SHIFT;
        case AKEYCODE_CTRL_RIGHT: return GLFW_KEY_RIGHT_CONTROL;
        case AKEYCODE_ALT_RIGHT: return GLFW_KEY_RIGHT_ALT;
        default:
            return -1;
    }
}

static int translate_android_action_button(int32_t actionButton) {
    switch (actionButton) {
        case AMOTION_EVENT_BUTTON_PRIMARY: return GLFW_MOUSE_BUTTON_LEFT;
        case AMOTION_EVENT_BUTTON_SECONDARY: return GLFW_MOUSE_BUTTON_RIGHT;
        case AMOTION_EVENT_BUTTON_TERTIARY: return GLFW_MOUSE_BUTTON_MIDDLE;
        default:
            return -1;
    }
}

extern void _glfwUpdateJoystickConnectSate(void);

static void android_dequeue_event(input_event_t* evp) {
    input_event_t event = *evp;
    if(surfaceOwner == NULL) return;
    switch (event.type) {
        case GLFW_ANDROID_EVENT_TYPE_EMPTY:
            break;
        case GLFW_ANDROID_EVENT_TYPE_KEYBOARD_KEY:
            if(event.k.glfw_code != 0) _glfwInputKey(surfaceOwner, event.k.glfw_code, event.k.code, event.k.state, event.k.mods);
            if(event.k.codepoint != 0) _glfwInputChar(surfaceOwner, event.k.codepoint, event.k.mods, true);
            break;
        case GLFW_ANDROID_EVENT_TYPE_MOUSE_BUTTONS:
            _glfwInputMouseClick(surfaceOwner, event.m.button, event.m.state, event.m.mods);
            break;
        case GLFW_ANDROID_EVENT_TYPE_UNICODE_CHARS:
            for(int32_t i = 0; i < event.u.length; i++) {
                _glfwInputChar(surfaceOwner, event.u.codepoints[i], event.u.mods, true);
            }
            free(event.u.codepoints);
            break;
        case GLFW_ANDROID_EVENT_TYPE_MOUSE_SCROLL:
            _glfwInputScroll(surfaceOwner, event.s.xscroll, event.s.yscroll);
            break;
        case GLFW_ANDROID_EVENT_TYPE_JOYSTICK_STATE:
            _glfwUpdateJoystickConnectSate();
            break;
    }
}

static inline void android_send_event(input_event_t *ev) {
    _input_queue_push(&input_queue, ev);
}

static void computeCursorPos() {
    int width = surfaceOwner->android.width;
    int height = surfaceOwner->android.height;
    _glfw.android.xcursor = cursor_unscaled.x * width;
    _glfw.android.ycursor = cursor_unscaled.y *  height;
}

static void push_flag_events() {
    if((update_flags & FLAG_MOUSE_POS) != 0) {
        computeCursorPos();
        _glfwInputCursorPos(surfaceOwner, _glfw.android.xcursor, _glfw.android.ycursor);
    }
    update_flags = 0;
}

static inline void process_flag_bits() {
    if(update_flags == 0) return;
    push_flag_events();
}

static void applySizeLimits(_GLFWwindow* window, int* width, int* height)
{
    if (window->numer != GLFW_DONT_CARE && window->denom != GLFW_DONT_CARE)
    {
        const float ratio = (float) window->numer / (float) window->denom;
        *height = (int) (*width / ratio);
    }

    if (window->minwidth != GLFW_DONT_CARE)
        *width = _glfw_max(*width, window->minwidth);
    else if (window->maxwidth != GLFW_DONT_CARE)
        *width = _glfw_min(*width, window->maxwidth);

    if (window->minheight != GLFW_DONT_CARE)
        *height = _glfw_min(*height, window->minheight);
    else if (window->maxheight != GLFW_DONT_CARE)
        *height = _glfw_max(*height, window->maxheight);
}

static void fitToMonitor(_GLFWwindow* window)
{
    GLFWvidmode mode;
    _glfwGetVideoModeAndroid(window->monitor, &mode);
    _glfwGetMonitorPosAndroid(window->monitor,
                           &window->android.xpos,
                           &window->android.ypos);
    window->android.width = mode.width;
    window->android.height = mode.height;
}

static void acquireMonitor(_GLFWwindow* window)
{
    _glfwInputMonitorWindow(window->monitor, window);
}

static void releaseMonitor(_GLFWwindow* window)
{
    if (window->monitor->window != window)
        return;

    _glfwInputMonitorWindow(window->monitor, NULL);
}

static int createNativeWindow(_GLFWwindow* window,
                              const _GLFWwndconfig* wndconfig,
                              const _GLFWfbconfig* fbconfig)
{
    if(surfaceOwner == NULL) {
        _glfw.android.focusedWindow = surfaceOwner = window;
    }
    if (window->monitor)
        fitToMonitor(window);
    else
    {
        if (wndconfig->xpos == GLFW_ANY_POSITION && wndconfig->ypos == GLFW_ANY_POSITION)
        {
            window->android.xpos = 0;
            window->android.ypos = 0;
        }
        else
        {
            window->android.xpos = wndconfig->xpos;
            window->android.ypos = wndconfig->ypos;
        }

        window->android.width = wndconfig->width;
        window->android.height = wndconfig->height;
    }

    window->android.visible = wndconfig->visible;
    window->android.decorated = wndconfig->decorated;
    window->android.maximized = wndconfig->maximized;
    window->android.floating = wndconfig->floating;
    window->android.transparent = fbconfig->transparent;
    window->android.opacity = 1.f;

    return GLFW_TRUE;
}


GLFWbool android_init_window(void) {
    if(pthread_mutex_init(&nw_egl_mutex, NULL) != 0) return GLFW_FALSE;
    if(pthread_cond_init(&nw_egl_cond, NULL) != 0) goto fail1;
    if(pthread_mutex_init(&nw_vulkan_mutex, NULL) != 0) goto fail2;
    if(pthread_cond_init(&nw_vulkan_cond, NULL) != 0) goto fail3;
    if(!_input_queue_init(&input_queue)) goto fail4;
    return GLFW_TRUE;

    fail4:
    pthread_cond_destroy(&nw_vulkan_cond);
    fail3:
    pthread_mutex_destroy(&nw_vulkan_mutex);
    fail2:
    pthread_cond_destroy(&nw_egl_cond);
    fail1:
    pthread_mutex_destroy(&nw_egl_mutex);
    return GLFW_FALSE;
}

void android_destroy_window(void) {
    pthread_mutex_destroy(&nw_egl_mutex);
    pthread_cond_destroy(&nw_egl_cond);
    pthread_mutex_destroy(&nw_vulkan_mutex);
    pthread_cond_destroy(&nw_vulkan_cond);
    _input_queue_destroy(&input_queue);
}

static void android_reconfigure_context(const _GLFWctxconfig *ctxconfig, _GLFWctxconfig *target) {
    memcpy(target, ctxconfig, sizeof (_GLFWctxconfig));
    if(target->client == GLFW_NO_API) return;

    if(_glfw.android.renderspec->force_gles_context) {
        target->client = GLFW_OPENGL_ES_API;
        target->major = _glfw.android.renderspec->override_major_version;
        target->minor = 0;
    }
}

//////////////////////////////////////////////////////////////////////////
//////                       GLFW platform API                      //////
//////////////////////////////////////////////////////////////////////////

GLFWbool _glfwCreateWindowAndroid(_GLFWwindow* window,
                               const _GLFWwndconfig* wndconfig,
                               const _GLFWctxconfig* old_config,
                               const _GLFWfbconfig* fbconfig)
{
    if (!createNativeWindow(window, wndconfig, fbconfig))
        return GLFW_FALSE;

    _GLFWctxconfig new_config;
    android_reconfigure_context(old_config, &new_config);
    const _GLFWctxconfig *ctxconfig = &new_config;

    if (ctxconfig->client != GLFW_NO_API)
    {
        if (ctxconfig->source == GLFW_OSMESA_CONTEXT_API)
        {
            if (!_glfwInitOSMesa())
                return GLFW_FALSE;
            if (!_glfwCreateContextOSMesa(window, ctxconfig, fbconfig))
                return GLFW_FALSE;
        }
        else if (ctxconfig->source == GLFW_EGL_CONTEXT_API ||
                ctxconfig->source == GLFW_NATIVE_CONTEXT_API)
        {
            if (!_glfwInitEGL())
                return GLFW_FALSE;
            if (!_glfwCreateContextEGL(window, ctxconfig, fbconfig))
                return GLFW_FALSE;

            EGLDisplay  display = _glfw.egl.display;
            EGLConfig  config = window->context.egl.config;
            EGLBoolean res = eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &window->android.visualId);
            if(!res) _glfwInputError(GLFW_PLATFORM_ERROR, "Failed to query the default visual ID: %x", eglGetError());
        }

        if (!_glfwRefreshContextAttribs(window, ctxconfig))
            return GLFW_FALSE;
    }

    if (wndconfig->mousePassthrough)
        _glfwSetWindowMousePassthroughAndroid(window, GLFW_TRUE);

    if (window->monitor)
    {
        _glfwShowWindowAndroid(window);
        _glfwFocusWindowAndroid(window);
        acquireMonitor(window);

        if (wndconfig->centerCursor)
            _glfwCenterCursorInContentArea(window);
    }
    else
    {
        if (wndconfig->visible)
        {
            _glfwShowWindowAndroid(window);
            if (wndconfig->focused)
                _glfwFocusWindowAndroid(window);
        }
    }

    return GLFW_TRUE;
}

void _glfwDestroyWindowAndroid(_GLFWwindow* window)
{
    if (window->monitor)
        releaseMonitor(window);

    if (_glfw.android.focusedWindow == window)
        _glfw.android.focusedWindow = NULL;

    if (window->context.destroy)
        window->context.destroy(window);
}

void _glfwSetWindowTitleAndroid(_GLFWwindow* window, const char* title)
{
}

void _glfwSetWindowIconAndroid(_GLFWwindow* window, int count, const GLFWimage* images)
{
}

void _glfwSetWindowMonitorAndroid(_GLFWwindow* window,
                               _GLFWmonitor* monitor,
                               int xpos, int ypos,
                               int width, int height,
                               int refreshRate)
{
    if (window->monitor == monitor)
    {
        if (!monitor)
        {
            _glfwSetWindowPosAndroid(window, xpos, ypos);
            _glfwSetWindowSizeAndroid(window, width, height);
        }

        return;
    }

    if (window->monitor)
        releaseMonitor(window);

    _glfwInputWindowMonitor(window, monitor);

    if (window->monitor)
    {
        window->android.visible = GLFW_TRUE;
        acquireMonitor(window);
        fitToMonitor(window);
    }
    else
    {
        _glfwSetWindowPosAndroid(window, xpos, ypos);
        _glfwSetWindowSizeAndroid(window, width, height);
    }
}

void _glfwGetWindowPosAndroid(_GLFWwindow* window, int* xpos, int* ypos)
{
    if (xpos)
        *xpos = window->android.xpos;
    if (ypos)
        *ypos = window->android.ypos;
}

void _glfwSetWindowPosAndroid(_GLFWwindow* window, int xpos, int ypos)
{
    if (window->monitor)
        return;

    if (window->android.xpos != xpos || window->android.ypos != ypos)
    {
        window->android.xpos = xpos;
        window->android.ypos = ypos;
        _glfwInputWindowPos(window, xpos, ypos);
    }
}

void _glfwGetWindowSizeAndroid(_GLFWwindow* window, int* width, int* height)
{
    if (width)
        *width = window->android.width;
    if (height)
        *height = window->android.height;
}

void _glfwSetWindowSizeAndroid(_GLFWwindow* window, int width, int height)
{
    if (window->monitor)
        return;

    if (window->android.width != width || window->android.height != height)
    {
        // Actually, we don't let the window resize itself, so we just put the old size back in
        // The only potentially scary thing here is that this may cause a resize tug-of-war,
        // but hopefully that won't happen...
        width = window->android.width;
        height = window->android.height;
        _glfwInputFramebufferSize(window, width, height);
        _glfwInputWindowDamage(window);
        _glfwInputWindowSize(window, width, height);
    }
}

void _glfwSetWindowSizeLimitsAndroid(_GLFWwindow* window,
                                  int minwidth, int minheight,
                                  int maxwidth, int maxheight)
{
    int width = window->android.width;
    int height = window->android.height;
    applySizeLimits(window, &width, &height);
    //_glfwSetWindowSizeAndroid(window, width, height);
}

void _glfwSetWindowAspectRatioAndroid(_GLFWwindow* window, int n, int d)
{
    int width = window->android.width;
    int height = window->android.height;
    applySizeLimits(window, &width, &height);
    //_glfwSetWindowSizeAndroid(window, width, height);
}

void _glfwGetFramebufferSizeAndroid(_GLFWwindow* window, int* width, int* height)
{
    if (width)
        *width = window->android.width;
    if (height)
        *height = window->android.height;
}

void _glfwGetWindowFrameSizeAndroid(_GLFWwindow* window,
                                 int* left, int* top,
                                 int* right, int* bottom)
{
    if (window->android.decorated && !window->monitor)
    {
        if (left)
            *left = 1;
        if (top)
            *top = 10;
        if (right)
            *right = 1;
        if (bottom)
            *bottom = 1;
    }
    else
    {
        if (left)
            *left = 0;
        if (top)
            *top = 0;
        if (right)
            *right = 0;
        if (bottom)
            *bottom = 0;
    }
}

void _glfwGetWindowContentScaleAndroid(_GLFWwindow* window, float* xscale, float* yscale)
{
    if (xscale)
        *xscale = 1.f;
    if (yscale)
        *yscale = 1.f;
}

void _glfwIconifyWindowAndroid(_GLFWwindow* window)
{
    if (_glfw.android.focusedWindow == window)
    {
        _glfw.android.focusedWindow = NULL;
        _glfwInputWindowFocus(window, GLFW_FALSE);
    }

    if (!window->android.iconified)
    {
        window->android.iconified = GLFW_TRUE;
        _glfwInputWindowIconify(window, GLFW_TRUE);

        if (window->monitor)
            releaseMonitor(window);
    }
}

void _glfwRestoreWindowAndroid(_GLFWwindow* window)
{
    if (window->android.iconified)
    {
        window->android.iconified = GLFW_FALSE;
        _glfwInputWindowIconify(window, GLFW_FALSE);

        if (window->monitor)
            acquireMonitor(window);
    }
    else if (window->android.maximized)
    {
        window->android.maximized = GLFW_FALSE;
        _glfwInputWindowMaximize(window, GLFW_FALSE);
    }
}

void _glfwMaximizeWindowAndroid(_GLFWwindow* window)
{
    if (!window->android.maximized)
    {
        window->android.maximized = GLFW_TRUE;
        _glfwInputWindowMaximize(window, GLFW_TRUE);
    }
}

GLFWbool _glfwWindowMaximizedAndroid(_GLFWwindow* window)
{
    return GLFW_TRUE;
}

GLFWbool _glfwWindowHoveredAndroid(_GLFWwindow* window)
{
    return GLFW_TRUE;
}

GLFWbool _glfwFramebufferTransparentAndroid(_GLFWwindow* window)
{
    return window->android.transparent;
}

void _glfwSetWindowResizableAndroid(_GLFWwindow* window, GLFWbool enabled)
{
    window->android.resizable = enabled;
}

void _glfwSetWindowDecoratedAndroid(_GLFWwindow* window, GLFWbool enabled)
{
    window->android.decorated = enabled;
}

void _glfwSetWindowFloatingAndroid(_GLFWwindow* window, GLFWbool enabled)
{
    window->android.floating = enabled;
}

void _glfwSetWindowMousePassthroughAndroid(_GLFWwindow* window, GLFWbool enabled)
{
}

float _glfwGetWindowOpacityAndroid(_GLFWwindow* window)
{
    return window->android.opacity;
}

void _glfwSetWindowOpacityAndroid(_GLFWwindow* window, float opacity)
{
    window->android.opacity = opacity;
}

void _glfwSetRawMouseMotionAndroid(_GLFWwindow *window, GLFWbool enabled)
{
}

GLFWbool _glfwRawMouseMotionSupportedAndroid(void)
{
    return GLFW_TRUE;
}

void _glfwShowWindowAndroid(_GLFWwindow* window)
{
    window->android.visible = GLFW_TRUE;
}

void _glfwRequestWindowAttentionAndroid(_GLFWwindow* window)
{
}

void _glfwHideWindowAndroid(_GLFWwindow* window)
{
    if (_glfw.android.focusedWindow == window)
    {
        _glfw.android.focusedWindow = NULL;
        _glfwInputWindowFocus(window, GLFW_FALSE);
    }

    window->android.visible = GLFW_FALSE;
}

void _glfwFocusWindowAndroid(_GLFWwindow* window)
{
    _GLFWwindow* previous;

    if (_glfw.android.focusedWindow == window)
        return;

    if (!window->android.visible)
        return;

    previous = _glfw.android.focusedWindow;
    _glfw.android.focusedWindow = window;

    if (previous)
    {
        _glfwInputWindowFocus(previous, GLFW_FALSE);
        if (previous->monitor && previous->autoIconify)
            _glfwIconifyWindowAndroid(previous);
    }

    _glfwInputWindowFocus(window, GLFW_TRUE);
}

GLFWbool _glfwWindowFocusedAndroid(_GLFWwindow* window)
{
    return _glfw.android.focusedWindow == window;
}

GLFWbool _glfwWindowIconifiedAndroid(_GLFWwindow* window)
{
    return window->android.iconified;
}

GLFWbool _glfwWindowVisibleAndroid(_GLFWwindow* window)
{
    return window->android.visible;
}

void updateNativeWindowDimensions(_GLFWwindow* window) {
    // This is incredibly cringe, but...
    // When we set W/H in the buffer geometry to 0,0, we reset the ANW to its default dimensions
    // Therefore, we also don't change its dimensions (which breaks vulkan swapchains)
    ANativeWindow_setBuffersGeometry(nativeWindow, 0, 0, window->android.visualId);
    int width = ANativeWindow_getWidth(nativeWindow);
    int height = ANativeWindow_getHeight(nativeWindow);
    LOGI("Update window dimensions: %i %i", width, height);
    window->android.width = width;
    window->android.height = height;
    surfaceUpdated = false;

    _glfwInputWindowSize(window, width, height);
    _glfwInputFramebufferSize(window, width, height);
}

void _glfwPollEventsAndroid(void)
{
    process_flag_bits();
    _input_queue_dequeue(&input_queue, android_dequeue_event);

    if((surfaceOwner->android.mode == GLFW_ANDROID_WINDOW_MODE_SURFACE || ownedByVulkan) && surfaceUpdated) {
        updateNativeWindowDimensions(surfaceOwner);
    }
}

static inline void poll_with_flags() {
    push_flag_events();
    _input_queue_dequeue(&input_queue, android_dequeue_event);
}

void _glfwWaitEventsAndroid(void)
{
    if(update_flags != 0) {
        poll_with_flags();
        return;
    }
    _input_queue_wait(&input_queue, android_dequeue_event);
    process_flag_bits();
}

void _glfwWaitEventsTimeoutAndroid(double timeout)
{
    if(update_flags != 0) {
        poll_with_flags();
        return;
    }

    double norm, rem;
    rem = modf(timeout, &norm);
    struct timespec ts_timeout;
    if(clock_gettime(CLOCK_MONOTONIC, &ts_timeout) == 0) {
        long total_wait_nsec = ts_timeout.tv_nsec + (long) (rem * 1000000000.0);
        long extra_wait_sec = total_wait_nsec / 1000000000L;
        ts_timeout.tv_nsec = total_wait_nsec % 1000000000L;
        ts_timeout.tv_sec += extra_wait_sec + (long) norm;
        _input_queue_timedwait(&input_queue, android_dequeue_event, &ts_timeout);
    }else {
        _input_queue_dequeue(&input_queue, android_dequeue_event);
    }

    process_flag_bits();
}

void _glfwPostEmptyEventAndroid(void)
{
    input_event_t event;
    event.type = GLFW_ANDROID_EVENT_TYPE_EMPTY;
    android_send_event(&event);
}

void _glfwGetCursorPosAndroid(_GLFWwindow* window, double* xpos, double* ypos)
{
    if (xpos)
        *xpos = _glfw.android.xcursor;
    if (ypos)
        *ypos = _glfw.android.ycursor;
}

void _glfwSetCursorPosAndroid(_GLFWwindow* window, double x, double y)
{
    ensure_comm_connected();
    double scaled_cursor_x = x / window->android.width;
    double scaled_cursor_y = y / window->android.height;
    (*jni_tl.env)->CallStaticVoidMethod(jni_tl.env, jni.glfw_class,
                                        jni.method_receiveCursorPos,
                                        scaled_cursor_x, scaled_cursor_y);

    cursor_unscaled.x = scaled_cursor_x;
    cursor_unscaled.y = scaled_cursor_y;
    if(window == surfaceOwner) {
        _glfw.android.xcursor = x;
        _glfw.android.ycursor = y;
    } else {
        computeCursorPos();
    }
}

void _glfwSetCursorModeAndroid(_GLFWwindow* window, int mode)
{
    ensure_comm_connected();
    (*jni_tl.env)->CallStaticVoidMethod(jni_tl.env, jni.glfw_class,
                                        jni.method_receiveGrabState,
                                        mode == GLFW_CURSOR_DISABLED);
}

GLFWbool _glfwCreateCursorAndroid(_GLFWcursor* cursor,
                               const GLFWimage* image,
                               int xhot, int yhot)
{
    cursor->android.cursorRef = NULL;
    ensure_comm_connected();
    jobject imageBuffer = (*jni_tl.env)->NewDirectByteBuffer(jni_tl.env, image->pixels, image->width * image->height * 4);
    if((*jni_tl.env)->ExceptionCheck(jni_tl.env)) {
        (*jni_tl.env)->ExceptionClear(jni_tl.env);
        return GLFW_FALSE;
    }
    jobject cursorRef = (*jni_tl.env)->CallStaticObjectMethod(jni_tl.env, jni.glfw_class, jni.method_loadCursor,
                                                            imageBuffer, image->width, image->height, xhot, yhot);
    if(cursorRef == NULL) return GLFW_FALSE;
    jobject globalRef = (*jni_tl.env)->NewGlobalRef(jni_tl.env, cursorRef);
    if(globalRef == NULL) {
        (*jni_tl.env)->ExceptionClear(jni_tl.env);
        return GLFW_FALSE;
    }
    cursor->android.cursorRef = globalRef;
    return GLFW_TRUE;
}

GLFWbool _glfwCreateStandardCursorAndroid(_GLFWcursor* cursor, int shape)
{
    cursor->android.cursorRef = NULL;
    return GLFW_TRUE;
}

void _glfwDestroyCursorAndroid(_GLFWcursor* cursor)
{
    ensure_comm_connected();
    jobject cursorRef = cursor->android.cursorRef;
    if(cursorRef != NULL) {
        (*jni_tl.env)->DeleteGlobalRef(jni_tl.env, cursorRef);
    }
}

void _glfwSetCursorAndroid(_GLFWwindow* window, _GLFWcursor* cursor)
{
    ensure_comm_connected();
    jobject cursorRef = cursor ? cursor->android.cursorRef : NULL;
    (*jni_tl.env)->CallStaticVoidMethod(jni_tl.env, jni.glfw_class, jni.method_useCursor, cursorRef);
}

static void free_old_clip() {
    if(clipboard_string != NULL && clipboard_string_ref != NULL) {
        (*jni_tl.env)->ReleaseStringUTFChars(jni_tl.env, clipboard_string_ref, clipboard_string);
        (*jni_tl.env)->DeleteGlobalRef(jni_tl.env, clipboard_string_ref);
        clipboard_string_ref = NULL;
        clipboard_string = NULL;
    }

    if(clipboard_string_ref != NULL) {
        (*jni_tl.env)->DeleteGlobalRef(jni_tl.env, clipboard_string_ref);
        clipboard_string_ref = NULL;
    }
}

static void set_new_clip(jstring clip_string) {
    clipboard_string_ref = (*jni_tl.env)->NewGlobalRef(jni_tl.env, clip_string);
    clipboard_string = (*jni_tl.env)->GetStringUTFChars(jni_tl.env, clipboard_string_ref, NULL);
}

void _glfwSetClipboardStringAndroid(const char* string)
{
    ensure_comm_connected();

    free_old_clip();

    jstring clip_string = (*jni_tl.env)->NewStringUTF(jni_tl.env, string);

    (*jni_tl.env)->CallStaticVoidMethod(jni_tl.env, jni.glfw_class, jni.method_setClipboardString, clip_string);
}

const char* _glfwGetClipboardStringAndroid(void)
{
    ensure_comm_connected();

    free_old_clip();

    jstring clip_string = (*jni_tl.env)->CallStaticObjectMethod(jni_tl.env, jni.glfw_class, jni.method_getClipboardString);
    if(clip_string == NULL) {
        return "";
    }

    set_new_clip(clip_string);
    return clipboard_string;
}

void _glfwEnableGamepadAndroid(unsigned char* buttons, int buttonCount, float* axes, int axisCount) {
    ensure_comm_connected();
    jobject buttonBuffer = (*jni_tl.env)->NewDirectByteBuffer(jni_tl.env, buttons, sizeof(char) * buttonCount);
    jobject axisBuffer = (*jni_tl.env)->NewDirectByteBuffer(jni_tl.env, axes, sizeof(float) * axisCount);
    (*jni_tl.env)->CallStaticVoidMethod(jni_tl.env, jni.glfw_class, jni.method_enableDirectGamepad, buttonBuffer, axisBuffer);
}

EGLenum _glfwGetEGLPlatformAndroid(EGLint** attribs)
{
    return 0;
}

EGLNativeDisplayType _glfwGetEGLNativeDisplayAndroid(void)
{
    return EGL_DEFAULT_DISPLAY;
}

EGLNativeWindowType _glfwGetEGLNativeWindowAndroid(_GLFWwindow* window)
{
    if(window == surfaceOwner) return nativeWindow;
    return 0;
}

void _glfwUpdatePreeditCursorRectangleAndroid(_GLFWwindow* window)
{
}

void _glfwResetPreeditTextAndroid(_GLFWwindow* window)
{
}

void _glfwSetIMEStatusAndroid(_GLFWwindow* window, int active)
{
}

int _glfwGetIMEStatusAndroid(_GLFWwindow* window)
{
    return GLFW_FALSE;
}

// Select a new EGLSurface
EGLSurface _glfwManageEglSurfaceAndroid(_GLFWwindow* window) {
    int wantedMode = GLFW_ANDROID_WINDOW_MODE_UNDEFINED;
    int currentMode = window->android.mode;
    if (window != surfaceOwner || surfaceDestroyed) wantedMode = GLFW_ANDROID_WINDOW_MODE_PBUFFER;
    else wantedMode = GLFW_ANDROID_WINDOW_MODE_SURFACE;

    if (currentMode != wantedMode) {
        EGLSurface oldSurface = window->context.egl.surface;

        eglMakeCurrent(_glfw.egl.display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (oldSurface != EGL_NO_SURFACE) {
            eglDestroySurface(_glfw.egl.display, oldSurface);
        }

        if (currentMode == GLFW_ANDROID_WINDOW_MODE_SURFACE) {
            pthread_mutex_lock(&nw_egl_mutex);
            pthread_cond_broadcast(&nw_egl_cond);
            pthread_mutex_unlock(&nw_egl_mutex);
            surfaceInUse = false;
        }
    } else {
        return window->context.egl.surface;
    }

    EGLDisplay  display = _glfw.egl.display;
    EGLConfig  config = window->context.egl.config;
    EGLSurface newSurface;

    switch (wantedMode) {
        case GLFW_ANDROID_WINDOW_MODE_PBUFFER: {
            const EGLint attribs[] = {
                    EGL_WIDTH, window->android.width,
                    EGL_HEIGHT, window->android.height,
                    EGL_NONE
            };
            LOGI("Configure pbuffer for inactive window");
            newSurface = eglCreatePbufferSurface(display, config, attribs);
        } break;
        case GLFW_ANDROID_WINDOW_MODE_SURFACE: {
            LOGI("Configure native window: %p", nativeWindow);
            updateNativeWindowDimensions(window);
            surfaceInUse = true;
            newSurface = eglCreateWindowSurface(display, config, nativeWindow, NULL);
        } break;
        default:
            abort();
    }

    window->android.mode = wantedMode;
    window->context.egl.surface = newSurface;
    return newSurface;
}

// Check if the surface has been changed before attempting to swap buffers
GLFWbool _glfwSwapBuffersAttentionEglAndroid(_GLFWwindow* window) {
    if(window == surfaceOwner && !ownedByVulkan) {
        switch (window->android.mode) {
            case GLFW_ANDROID_WINDOW_MODE_UNDEFINED: return true;
            case GLFW_ANDROID_WINDOW_MODE_SURFACE: return surfaceDestroyed || surfaceUpdated;
            case GLFW_ANDROID_WINDOW_MODE_PBUFFER: return !surfaceDestroyed;
        }
    } else {
        switch (window->android.mode) {
            case GLFW_ANDROID_WINDOW_MODE_UNDEFINED:
            case GLFW_ANDROID_WINDOW_MODE_SURFACE:
                return true;
        }
    }
    return false;
}

const char* _glfwGetScancodeNameAndroid(int scancode)
{
    if (scancode > AKEYCODE_MACRO_4)
    {
        _glfwInputError(GLFW_INVALID_VALUE, "Invalid scancode %i", scancode);
        return NULL;
    }

    // TODO: query KeyCharacterMap for the keycode display labels

    return NULL;
}

int _glfwGetKeyScancodeAndroid(int key)
{
    switch (key) {
        case GLFW_KEY_SPACE: return AKEYCODE_SPACE;
        case GLFW_KEY_APOSTROPHE: return AKEYCODE_APOSTROPHE;
        case GLFW_KEY_COMMA: return AKEYCODE_COMMA;
        case GLFW_KEY_MINUS: return AKEYCODE_MINUS;
        case GLFW_KEY_PERIOD: return AKEYCODE_PERIOD;
        case GLFW_KEY_SLASH: return AKEYCODE_SLASH;
        case GLFW_KEY_0: return AKEYCODE_0;
        case GLFW_KEY_1: return AKEYCODE_1;
        case GLFW_KEY_2: return AKEYCODE_2;
        case GLFW_KEY_3: return AKEYCODE_3;
        case GLFW_KEY_4: return AKEYCODE_4;
        case GLFW_KEY_5: return AKEYCODE_5;
        case GLFW_KEY_6: return AKEYCODE_6;
        case GLFW_KEY_7: return AKEYCODE_7;
        case GLFW_KEY_8: return AKEYCODE_8;
        case GLFW_KEY_9: return AKEYCODE_9;
        case GLFW_KEY_SEMICOLON: return AKEYCODE_SEMICOLON;
        case GLFW_KEY_EQUAL: return AKEYCODE_EQUALS;
        case GLFW_KEY_A: return AKEYCODE_A;
        case GLFW_KEY_B: return AKEYCODE_B;
        case GLFW_KEY_C: return AKEYCODE_C;
        case GLFW_KEY_D: return AKEYCODE_D;
        case GLFW_KEY_E: return AKEYCODE_E;
        case GLFW_KEY_F: return AKEYCODE_F;
        case GLFW_KEY_G: return AKEYCODE_G;
        case GLFW_KEY_H: return AKEYCODE_H;
        case GLFW_KEY_I: return AKEYCODE_I;
        case GLFW_KEY_J: return AKEYCODE_J;
        case GLFW_KEY_K: return AKEYCODE_K;
        case GLFW_KEY_L: return AKEYCODE_L;
        case GLFW_KEY_M: return AKEYCODE_M;
        case GLFW_KEY_N: return AKEYCODE_N;
        case GLFW_KEY_O: return AKEYCODE_O;
        case GLFW_KEY_P: return AKEYCODE_P;
        case GLFW_KEY_Q: return AKEYCODE_Q;
        case GLFW_KEY_R: return AKEYCODE_R;
        case GLFW_KEY_S: return AKEYCODE_S;
        case GLFW_KEY_T: return AKEYCODE_T;
        case GLFW_KEY_U: return AKEYCODE_U;
        case GLFW_KEY_V: return AKEYCODE_V;
        case GLFW_KEY_W: return AKEYCODE_W;
        case GLFW_KEY_X: return AKEYCODE_X;
        case GLFW_KEY_Y: return AKEYCODE_Y;
        case GLFW_KEY_Z: return AKEYCODE_Z;
        case GLFW_KEY_LEFT_BRACKET: return AKEYCODE_LEFT_BRACKET;
        case GLFW_KEY_BACKSLASH: return AKEYCODE_BACKSLASH;
        case GLFW_KEY_RIGHT_BRACKET: return AKEYCODE_RIGHT_BRACKET;
        case GLFW_KEY_GRAVE_ACCENT: return AKEYCODE_GRAVE;
        case GLFW_KEY_ESCAPE: return AKEYCODE_ESCAPE;
        case GLFW_KEY_ENTER: return AKEYCODE_ENTER;
        case GLFW_KEY_TAB: return AKEYCODE_TAB;
        case GLFW_KEY_BACKSPACE: return AKEYCODE_DEL;
        case GLFW_KEY_INSERT: return AKEYCODE_INSERT;
        case GLFW_KEY_DELETE: return AKEYCODE_FORWARD_DEL;
        case GLFW_KEY_RIGHT: return AKEYCODE_DPAD_RIGHT;
        case GLFW_KEY_LEFT: return AKEYCODE_DPAD_LEFT;
        case GLFW_KEY_UP: return AKEYCODE_DPAD_UP;
        case GLFW_KEY_DOWN: return AKEYCODE_DPAD_DOWN;
        case GLFW_KEY_PAGE_UP: return AKEYCODE_PAGE_UP;
        case GLFW_KEY_PAGE_DOWN: return AKEYCODE_PAGE_DOWN;
        case GLFW_KEY_HOME: return AKEYCODE_MOVE_HOME;
        case GLFW_KEY_END: return AKEYCODE_MOVE_END;
        case GLFW_KEY_CAPS_LOCK: return AKEYCODE_CAPS_LOCK;
        case GLFW_KEY_SCROLL_LOCK: return AKEYCODE_SCROLL_LOCK;
        case GLFW_KEY_NUM_LOCK: return AKEYCODE_NUM_LOCK;
        case GLFW_KEY_PRINT_SCREEN: return AKEYCODE_SYSRQ;
        case GLFW_KEY_PAUSE: return AKEYCODE_BREAK;
        case GLFW_KEY_F1: return AKEYCODE_F1;
        case GLFW_KEY_F2: return AKEYCODE_F2;
        case GLFW_KEY_F3: return AKEYCODE_F3;
        case GLFW_KEY_F4: return AKEYCODE_F4;
        case GLFW_KEY_F5: return AKEYCODE_F5;
        case GLFW_KEY_F6: return AKEYCODE_F6;
        case GLFW_KEY_F7: return AKEYCODE_F7;
        case GLFW_KEY_F8: return AKEYCODE_F8;
        case GLFW_KEY_F9: return AKEYCODE_F9;
        case GLFW_KEY_F10: return AKEYCODE_F10;
        case GLFW_KEY_F11: return AKEYCODE_F11;
        case GLFW_KEY_F12: return AKEYCODE_F12;
        case GLFW_KEY_KP_0: return AKEYCODE_NUMPAD_0;
        case GLFW_KEY_KP_1: return AKEYCODE_NUMPAD_1;
        case GLFW_KEY_KP_2: return AKEYCODE_NUMPAD_2;
        case GLFW_KEY_KP_3: return AKEYCODE_NUMPAD_3;
        case GLFW_KEY_KP_4: return AKEYCODE_NUMPAD_4;
        case GLFW_KEY_KP_5: return AKEYCODE_NUMPAD_5;
        case GLFW_KEY_KP_6: return AKEYCODE_NUMPAD_6;
        case GLFW_KEY_KP_7: return AKEYCODE_NUMPAD_7;
        case GLFW_KEY_KP_8: return AKEYCODE_NUMPAD_8;
        case GLFW_KEY_KP_9: return AKEYCODE_NUMPAD_9;
        case GLFW_KEY_KP_DECIMAL: return AKEYCODE_NUMPAD_DOT;
        case GLFW_KEY_KP_DIVIDE: return AKEYCODE_NUMPAD_DIVIDE;
        case GLFW_KEY_KP_SUBTRACT: return AKEYCODE_NUMPAD_SUBTRACT;
        case GLFW_KEY_KP_ADD: return AKEYCODE_NUMPAD_ADD;
        case GLFW_KEY_KP_ENTER: return AKEYCODE_NUMPAD_ENTER;
        case GLFW_KEY_KP_EQUAL: return AKEYCODE_NUMPAD_EQUALS;
        case GLFW_KEY_LEFT_SHIFT: return AKEYCODE_SHIFT_LEFT;
        case GLFW_KEY_LEFT_CONTROL: return AKEYCODE_CTRL_LEFT;
        case GLFW_KEY_LEFT_ALT: return AKEYCODE_ALT_LEFT;
        case GLFW_KEY_RIGHT_SHIFT: return AKEYCODE_SHIFT_RIGHT;
        case GLFW_KEY_RIGHT_CONTROL: return AKEYCODE_CTRL_RIGHT;
        case GLFW_KEY_RIGHT_ALT: return AKEYCODE_ALT_RIGHT;
        default:
            //_glfwInputError(GLFW_INVALID_VALUE, "Invalid or unknown keycode %i", key);
            return -1;
    }
}

void _glfwGetRequiredInstanceExtensionsAndroid(char** extensions)
{
    if (!_glfw.vk.KHR_surface || !_glfw.vk.KHR_android_surface)
        return;
    extensions[0] = "VK_KHR_surface";
    extensions[1] = "VK_KHR_android_surface";
}

GLFWbool _glfwGetPhysicalDevicePresentationSupportAndroid(VkInstance instance,
                                                       VkPhysicalDevice device,
                                                       uint32_t queuefamily)
{
    if(!_glfw.vk.KHR_surface || !_glfw.vk.KHR_android_surface) return false;
    PFN_vkGetPhysicalDeviceQueueFamilyProperties getPhysicalDeviceQueueFamilyProperties =
            (PFN_vkGetPhysicalDeviceQueueFamilyProperties) vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceQueueFamilyProperties");

    if(getPhysicalDeviceQueueFamilyProperties == NULL) {
        _glfwInputError(GLFW_API_UNAVAILABLE,
                        "Android: Vulkan instance missing vkGetPhysicalDeviceQueueFamilyProperties");
        return false;
    }
    uint32_t maxfamilies = queuefamily + 1;
    VkQueueFamilyProperties properties[maxfamilies];
    properties[queuefamily].queueFlags = 0; // reset the flag in case the function below doesn't write to it
    getPhysicalDeviceQueueFamilyProperties(device, &maxfamilies, properties);

    return (properties[queuefamily].queueFlags & VK_QUEUE_GRAPHICS_BIT) == VK_QUEUE_GRAPHICS_BIT;
}

VkResult _glfwCreateWindowSurfaceAndroid(VkInstance instance,
                                      _GLFWwindow* window,
                                      const VkAllocationCallbacks* allocator,
                                      VkSurfaceKHR* surface)
{
    VkResult err;
    VkAndroidSurfaceCreateInfoKHR sci;
    PFN_vkCreateAndroidSurfaceKHR vkCreateAndroidSurfaceKHR;

    vkCreateAndroidSurfaceKHR = (PFN_vkCreateAndroidSurfaceKHR)
            vkGetInstanceProcAddr(instance, "vkCreateAndroidSurfaceKHR");
    if (!vkCreateAndroidSurfaceKHR)
    {
        _glfwInputError(GLFW_API_UNAVAILABLE,
                        "Android: Vulkan instance missing VK_KHR_android_surface extension");
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }

    ownedByVulkan = true;

    // Wait for a new window to become available
    if(nativeWindow == NULL) {
        pthread_mutex_lock(&nw_vulkan_mutex);
        pthread_cond_wait(&nw_vulkan_cond, &nw_vulkan_mutex);
        pthread_mutex_unlock(&nw_vulkan_mutex);
    }

    _glfwDisablePrerotationAndroid(nativeWindow);

    memset(&sci, 0, sizeof(sci));
    sci.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;

    //TODO vulkan native window
    sci.window = nativeWindow;

    err = vkCreateAndroidSurfaceKHR(instance, &sci, allocator, surface);
    if (err)
    {
        _glfwInputError(GLFW_PLATFORM_ERROR,
                        "Android: Failed to create Vulkan surface: %s",
                        _glfwGetVulkanResultString(err));
    }

    return err;
}


JNIEXPORT void JNICALL
Java_git_artdeell_dnbootstrap_glfw_GLFW_nativeSurfaceCreated(JNIEnv *env, jclass clazz,
                                                                         jobject surface) {
    ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
    ANativeWindow_acquire(window);
    LOGI("Acquired native window: %p", window);
    nativeWindow = window;
    surfaceDestroyed = false;
    if(ownedByVulkan) {
        pthread_mutex_lock(&nw_vulkan_mutex);
        pthread_cond_broadcast(&nw_vulkan_cond);
        pthread_mutex_unlock(&nw_vulkan_mutex);
    }
}

JNIEXPORT void JNICALL
Java_git_artdeell_dnbootstrap_glfw_GLFW_nativeSurfaceUpdated(JNIEnv *env, jclass clazz) {
    surfaceUpdated = true;
}

JNIEXPORT void JNICALL
Java_git_artdeell_dnbootstrap_glfw_GLFW_nativeSurfaceDestroyed(JNIEnv *env,
                                                                           jclass clazz) {
    surfaceDestroyed = true;
    if(!ownedByVulkan && surfaceInUse) {
        pthread_mutex_lock(&nw_egl_mutex);
        pthread_cond_wait(&nw_egl_cond, &nw_egl_mutex);
        LOGI("Unhalted after window destruction");
        ANativeWindow_release(nativeWindow);
        nativeWindow = NULL;
        pthread_mutex_unlock(&nw_egl_mutex);
    }else {
        ANativeWindow_release(nativeWindow);
        nativeWindow = NULL;
    }

}


JNIEXPORT void JNICALL
Java_git_artdeell_dnbootstrap_glfw_GLFW_sendMousePosition0__DD(JNIEnv *env, jclass clazz,
                                                          jdouble v1, jdouble v2) {
    if(cursor_unscaled.x == v1 && cursor_unscaled.y == v2) return;
    cursor_unscaled.x = v1;
    cursor_unscaled.y = v2;
    update_flags |= FLAG_MOUSE_POS;
    _input_queue_wait_unlock(&input_queue);
}

JNIEXPORT void JNICALL
Java_git_artdeell_dnbootstrap_glfw_GLFW_initialize(JNIEnv *env, jclass clazz) {
    (*env)->GetJavaVM(env, &jni.vm);
    jni.glfw_class = (*env)->NewGlobalRef(env, clazz);
    jni.method_receiveGrabState = (*env)->GetStaticMethodID(env, clazz, "receiveGrabState", "(Z)V");
    jni.method_receiveCursorPos = (*env)->GetStaticMethodID(env, clazz, "receiveCursorPos", "(DD)V");
    jni.method_loadCursor = (*env)->GetStaticMethodID(env, clazz, "loadCursor","(Ljava/nio/ByteBuffer;IIII)Lgit/artdeell/dnbootstrap/glfw/GLFWCursor;");
    jni.method_useCursor = (*env)->GetStaticMethodID(env, clazz, "useCursor","(Lgit/artdeell/dnbootstrap/glfw/GLFWCursor;)V");
    jni.method_getClipboardString = (*env)->GetStaticMethodID(env, clazz, "getClipboardString", "()Ljava/lang/String;");
    jni.method_setClipboardString = (*env)->GetStaticMethodID(env, clazz, "setClipboardString", "(Ljava/lang/String;)V");
    jni.method_enableDirectGamepad = (*env)->GetStaticMethodID(env, clazz, "enableDirectGamepad", "(Ljava/nio/ByteBuffer;Ljava/nio/ByteBuffer;)V");
}

JNIEXPORT void JNICALL
Java_git_artdeell_dnbootstrap_glfw_GLFW_sendKeyEvent(JNIEnv *env, jclass clazz, jint glfw_code,
                                                     jint state, jint mods) {
    input_event_t event = {
            .type = GLFW_ANDROID_EVENT_TYPE_KEYBOARD_KEY,
            .k.glfw_code = glfw_code,
            .k.code = _glfwGetKeyScancodeAndroid(glfw_code),
            .k.state = state,
            .k.mods = mods,
            .k.codepoint = 0
    };
    android_send_event(&event);
}

JNIEXPORT void JNICALL
Java_git_artdeell_dnbootstrap_glfw_GLFW_sendRawKeyEvent(JNIEnv *env, jclass clazz,
                                                        jint android_code, jint state, jint mods, jchar codepoint) {
    input_event_t event = {
            .type = GLFW_ANDROID_EVENT_TYPE_KEYBOARD_KEY,
            .k.glfw_code = translate_android_key(android_code),
            .k.code = android_code,
            .k.state = state,
            .k.mods = mods,
            .k.codepoint = codepoint
    };
    android_send_event(&event);
}

JNIEXPORT void JNICALL
Java_git_artdeell_dnbootstrap_glfw_GLFW_sendMouseEvent(JNIEnv *env, jclass clazz,
                                                       jint glfw_mouse_key, jint state, jint mods) {
    input_event_t event = {
            .type = GLFW_ANDROID_EVENT_TYPE_MOUSE_BUTTONS,
            .m.button = glfw_mouse_key,
            .m.state = state,
            .m.mods = mods
    };
    android_send_event(&event);
}

JNIEXPORT void JNICALL
Java_git_artdeell_dnbootstrap_glfw_GLFW_sendBulkUnicodeEvent(JNIEnv *env, jclass clazz,
                                                             jstring input, jint mods) {
    jsize length = (*env)->GetStringLength(env, input);
    jchar* codepoints = malloc(length * sizeof(codepoints));
    (*env)->GetStringRegion(env, input, 0, length, codepoints);

    input_event_t event = {
            .type = GLFW_ANDROID_EVENT_TYPE_UNICODE_CHARS,
            .u.length = length,
            .u.mods = mods,
            .u.codepoints = codepoints
    };
    android_send_event(&event);
}

JNIEXPORT void JNICALL
Java_git_artdeell_dnbootstrap_glfw_GLFW_sendScrollEvent(JNIEnv *env, jclass clazz, jdouble xoffset,
                                                        jdouble yoffset) {
    input_event_t event = {
            .type = GLFW_ANDROID_EVENT_TYPE_MOUSE_SCROLL,
            .s.xscroll = xoffset,
            .s.yscroll = yoffset
    };
    android_send_event(&event);
}

void _glfwSendJoystickConnectEvent(void) {
    input_event_t event = {
            .type = GLFW_ANDROID_EVENT_TYPE_JOYSTICK_STATE
    };
    android_send_event(&event);
}