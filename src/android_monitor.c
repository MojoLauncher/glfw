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

#include <stdlib.h>
#include <string.h>
#include <math.h>

// The the sole (fake) video mode of our (sole) fake monitor
//
static GLFWvidmode getVideoMode(void)
{
    GLFWvidmode mode;
    mode.width = _glfw.android.renderspec->disp_width;
    mode.height = _glfw.android.renderspec->disp_height;
    mode.redBits = 8;
    mode.greenBits = 8;
    mode.blueBits = 8;
    mode.refreshRate = _glfw.android.renderspec->disp_hz;
    return mode;
}

//////////////////////////////////////////////////////////////////////////
//////                       GLFW internal API                      //////
//////////////////////////////////////////////////////////////////////////

void _glfwPollMonitorsAndroid(void)
{
    const float dpi = 141.f;
    const GLFWvidmode mode = getVideoMode();
    _GLFWmonitor* monitor = _glfwAllocMonitor("Android SuperNoop 0",
                                              (int) (mode.width * 25.4f / dpi),
                                              (int) (mode.height * 25.4f / dpi));
    _glfwInputMonitor(monitor, GLFW_CONNECTED, _GLFW_INSERT_FIRST);
}

//////////////////////////////////////////////////////////////////////////
//////                       GLFW platform API                      //////
//////////////////////////////////////////////////////////////////////////

void _glfwFreeMonitorAndroid(_GLFWmonitor* monitor)
{
}

void _glfwGetMonitorPosAndroid(_GLFWmonitor* monitor, int* xpos, int* ypos)
{
    if (xpos)
        *xpos = 0;
    if (ypos)
        *ypos = 0;
}

void _glfwGetMonitorContentScaleAndroid(_GLFWmonitor* monitor,
                                     float* xscale, float* yscale)
{
    if (xscale)
        *xscale = 1.f;
    if (yscale)
        *yscale = 1.f;
}

void _glfwGetMonitorWorkareaAndroid(_GLFWmonitor* monitor,
                                 int* xpos, int* ypos,
                                 int* width, int* height)
{
    const GLFWvidmode mode = getVideoMode();

    if (xpos)
        *xpos = 0;
    if (ypos)
        *ypos = 0;
    if (width)
        *width = mode.width;
    if (height)
        *height = mode.height;
}

GLFWvidmode* _glfwGetVideoModesAndroid(_GLFWmonitor* monitor, int* found)
{
    GLFWvidmode* mode = _glfw_calloc(1, sizeof(GLFWvidmode));
    *mode = getVideoMode();
    *found = 1;
    return mode;
}

GLFWbool _glfwGetVideoModeAndroid(_GLFWmonitor* monitor, GLFWvidmode* mode)
{
    *mode = getVideoMode();
    return GLFW_TRUE;
}

GLFWbool _glfwGetGammaRampAndroid(_GLFWmonitor* monitor, GLFWgammaramp* ramp)
{
    // No native gamma control on Android
    return GLFW_FALSE;
}

void _glfwSetGammaRampAndroid(_GLFWmonitor* monitor, const GLFWgammaramp* ramp)
{

}

