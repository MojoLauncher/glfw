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

#include "internal.h"

#include <stdlib.h>
#include <string.h>
#include <mojoexec.h>


//////////////////////////////////////////////////////////////////////////
//////                       GLFW platform API                      //////
//////////////////////////////////////////////////////////////////////////

GLFWbool _glfwConnectAndroid(int platformID, _GLFWplatform* platform)
{
    const _GLFWplatform null =
    {
        .platformID = GLFW_PLATFORM_ANDROID,
        .init = _glfwInitAndroid,
        .terminate = _glfwTerminateAndroid,
        .getCursorPos = _glfwGetCursorPosAndroid,
        .setCursorPos = _glfwSetCursorPosAndroid,
        .setCursorMode = _glfwSetCursorModeAndroid,
        .setRawMouseMotion = _glfwSetRawMouseMotionAndroid,
        .rawMouseMotionSupported = _glfwRawMouseMotionSupportedAndroid,
        .createCursor = _glfwCreateCursorAndroid,
        .createStandardCursor = _glfwCreateStandardCursorAndroid,
        .destroyCursor = _glfwDestroyCursorAndroid,
        .setCursor = _glfwSetCursorAndroid,
        .getScancodeName = _glfwGetScancodeNameAndroid,
        .getKeyScancode = _glfwGetKeyScancodeAndroid,
        .setClipboardString = _glfwSetClipboardStringAndroid,
        .getClipboardString = _glfwGetClipboardStringAndroid,
        .initJoysticks = _glfwInitJoysticksAndroid,
        .terminateJoysticks = _glfwTerminateJoysticksAndroid,
        .pollJoystick = _glfwPollJoystickAndroid,
        .getMappingName = _glfwGetMappingNameAndroid,
        .updateGamepadGUID = _glfwUpdateGamepadGUIDAndroid,
        .freeMonitor = _glfwFreeMonitorAndroid,
        .getMonitorPos = _glfwGetMonitorPosAndroid,
        .getMonitorContentScale = _glfwGetMonitorContentScaleAndroid,
        .getMonitorWorkarea = _glfwGetMonitorWorkareaAndroid,
        .getVideoModes = _glfwGetVideoModesAndroid,
        .getVideoMode = _glfwGetVideoModeAndroid,
        .getGammaRamp = _glfwGetGammaRampAndroid,
        .setGammaRamp = _glfwSetGammaRampAndroid,
        .createWindow = _glfwCreateWindowAndroid,
        .destroyWindow = _glfwDestroyWindowAndroid,
        .setWindowTitle = _glfwSetWindowTitleAndroid,
        .setWindowIcon = _glfwSetWindowIconAndroid,
        .getWindowPos = _glfwGetWindowPosAndroid,
        .setWindowPos = _glfwSetWindowPosAndroid,
        .getWindowSize = _glfwGetWindowSizeAndroid,
        .setWindowSize = _glfwSetWindowSizeAndroid,
        .setWindowSizeLimits = _glfwSetWindowSizeLimitsAndroid,
        .setWindowAspectRatio = _glfwSetWindowAspectRatioAndroid,
        .getFramebufferSize = _glfwGetFramebufferSizeAndroid,
        .getWindowFrameSize = _glfwGetWindowFrameSizeAndroid,
        .getWindowContentScale = _glfwGetWindowContentScaleAndroid,
        .iconifyWindow = _glfwIconifyWindowAndroid,
        .restoreWindow = _glfwRestoreWindowAndroid,
        .maximizeWindow = _glfwMaximizeWindowAndroid,
        .showWindow = _glfwShowWindowAndroid,
        .hideWindow = _glfwHideWindowAndroid,
        .requestWindowAttention = _glfwRequestWindowAttentionAndroid,
        .focusWindow = _glfwFocusWindowAndroid,
        .setWindowMonitor = _glfwSetWindowMonitorAndroid,
        .windowFocused = _glfwWindowFocusedAndroid,
        .windowIconified = _glfwWindowIconifiedAndroid,
        .windowVisible = _glfwWindowVisibleAndroid,
        .windowMaximized = _glfwWindowMaximizedAndroid,
        .windowHovered = _glfwWindowHoveredAndroid,
        .framebufferTransparent = _glfwFramebufferTransparentAndroid,
        .getWindowOpacity = _glfwGetWindowOpacityAndroid,
        .setWindowResizable = _glfwSetWindowResizableAndroid,
        .setWindowDecorated = _glfwSetWindowDecoratedAndroid,
        .setWindowFloating = _glfwSetWindowFloatingAndroid,
        .setWindowOpacity = _glfwSetWindowOpacityAndroid,
        .setWindowMousePassthrough = _glfwSetWindowMousePassthroughAndroid,
        .pollEvents = _glfwPollEventsAndroid,
        .waitEvents = _glfwWaitEventsAndroid,
        .waitEventsTimeout = _glfwWaitEventsTimeoutAndroid,
        .postEmptyEvent = _glfwPostEmptyEventAndroid,
        .getEGLPlatform = _glfwGetEGLPlatformAndroid,
        .getEGLNativeDisplay = _glfwGetEGLNativeDisplayAndroid,
        .getEGLNativeWindow = _glfwGetEGLNativeWindowAndroid,
        .getRequiredInstanceExtensions = _glfwGetRequiredInstanceExtensionsAndroid,
        .getPhysicalDevicePresentationSupport = _glfwGetPhysicalDevicePresentationSupportAndroid,
        .createWindowSurface = _glfwCreateWindowSurfaceAndroid,
        .setIMEStatus = _glfwSetIMEStatusAndroid,
        .getIMEStatus = _glfwGetIMEStatusAndroid,
        .updatePreeditCursorRectangle = _glfwUpdatePreeditCursorRectangleAndroid,
        .resetPreeditText = _glfwResetPreeditTextAndroid
    };

    *platform = null;
    return GLFW_TRUE;
}

extern GLFWbool android_init_window(void);
extern void android_destroy_window(void);
extern void android_notify_init();

void* _glfwLoadVulkanDriverAndroid(void) {

    return mojoexec_acq_vulkan_handle();
}

void* _glfwLoadEglAndroid(void) {
    return mojoexec_acq_egl_handle();
}

int _glfwInitAndroid(void)
{

    _glfwPollMonitorsAndroid();
    android_notify_init();
    return android_init_window();
}

void _glfwTerminateAndroid(void)
{
    android_destroy_window();
    free(_glfw.null.clipboardString);
    _glfwTerminateOSMesa();
    _glfwTerminateEGL();
}

