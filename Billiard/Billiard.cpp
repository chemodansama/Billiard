// Billiard.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <glog/logging.h>

#include <GL/glew.h>
#include <GL/GL.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <memory>

#include "Game.h"

#ifdef _DEBUG
    #include <crtdbg.h>
    #define _CRTDBG_MAP_ALLOC
    #define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif

using namespace google;

namespace {
    std::shared_ptr<billiard::Game> g;

    void error_callback(int error, const char* description)
    {
        LOG(ERROR) << description;
    }

    void key_callback(GLFWwindow* window, int key, int scancode,
            int action, int mods)
    {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GL_TRUE);
		} else {
			g->keyAction(key, action == GLFW_PRESS || action == GLFW_REPEAT);
		}
    }

    void mouse_callback(GLFWwindow *window, int button, int action, int mods) {
        if (button == GLFW_MOUSE_BUTTON_1) {
            if (action == GLFW_PRESS) {
                double x, y;
                glfwGetCursorPos(window, &x, &y);
                g->mouseDown(static_cast<float>(x), static_cast<float>(y));
            } else if (action == GLFW_RELEASE) {
                g->mouseUp();
            }
        }
    }

    void mouse_scroll_callback(GLFWwindow *, double x, double y) {
        g->mouseScrolled(static_cast<float>(y));
    }

    void mouse_move_callback(GLFWwindow *window, double x, double y) {
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS) {
            g->mouseMoved(static_cast<float>(x), static_cast<float>(y));
        }
    }

    void window_size_callback(GLFWwindow* window, int w, int h) {
        g->resize(w, h);
    }
}

int run(int argc, _TCHAR* argv[]) 
{
    //ParseCommandLineFlags(&argc, &argv, true);
    InitGoogleLogging(argv[0]);
    LOG(INFO) << "starting";

    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
        return EXIT_FAILURE;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
    auto window = glfwCreateWindow(640, 480, "Simple example", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return EXIT_FAILURE;
    }
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);

    glewExperimental = GL_TRUE;

    auto err = glewInit();
    if (GLEW_OK != err) {
        LOG(ERROR) << "glewInit failed.";
        LOG(ERROR) << "Error: " << glewGetErrorString(err);
        return EXIT_FAILURE;
    }

    if (!GLEW_VERSION_4_1) {
        LOG(ERROR) << "OpenGL 4.1 not supported.\n";
        return EXIT_FAILURE;
    }

    GLint majorVersion;
    GLint minorVersion;
    glGetIntegerv(GL_MAJOR_VERSION, &majorVersion);
    glGetIntegerv(GL_MINOR_VERSION, &minorVersion);
    if (majorVersion < 4) {
        LOG(ERROR) << "OpenGL major version is " << majorVersion;
        return EXIT_FAILURE;
    }
    if (minorVersion < 1) {
        LOG(ERROR) << "OpenGL minor version is " << minorVersion;
        return EXIT_FAILURE;
    }

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    LOG(INFO) << "ogl error on initialization: " << 123;
    if (auto err = glGetError()) {
        LOG(INFO) << "ogl error on initialization: " << err;
    }

    g = std::make_shared<billiard::Game>(width, height);

    glfwSetCursorPosCallback(window, mouse_move_callback);
    glfwSetMouseButtonCallback(window, mouse_callback);
    glfwSetScrollCallback(window, mouse_scroll_callback);
    glfwSetWindowSizeCallback(window, window_size_callback);
    
    while (!glfwWindowShouldClose(window)) {
        g->render();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    g.reset();

    glfwDestroyWindow(window);
    glfwTerminate();

    LOG(INFO) << "done.";
    ShutdownGoogleLogging();

    return 0;
}

int _tmain(int argc, _TCHAR* argv[]) 
{
    // wrap main routine with memory leak detection code.

#ifdef _DEBUG
    _CrtMemState _ms;
    _CrtMemCheckpoint(&_ms);
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
#endif

    int r = run(argc, argv);

#ifdef _DEBUG
    _CrtMemDumpAllObjectsSince(&_ms);

    std::cout << "hit any key..";
    getchar();
#endif

    return r;
}

