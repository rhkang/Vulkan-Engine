#include "VEwindow.h"

#include <iostream>
#include <queue>

std::queue<int> KeyEventQueue{};
bool musicFlag = false;

void VEwindow::setUpWindow(const char* title, int width, int height) {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	VEwindow::window = glfwCreateWindow(width, height, title, nullptr, nullptr);
}

void VEwindow::cleanUpWindow() {
	glfwDestroyWindow(VEwindow::window);
	glfwTerminate();

    musicFlag = true;
}

void VEwindow::setWindowTitle(std::string title) {
    glfwSetWindowTitle(VEwindow::window, title.c_str());
}

void VEwindow::keyProcess(GLFWwindow* win, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_UNKNOWN) return; // Don't accept unknown keys

    if (action == GLFW_PRESS) {
        pressed[key] = true;
        KeyEventQueue.push(key);
    }
    else if (action == GLFW_RELEASE)
        pressed[key] = false;

    switch (key) {
    case GLFW_KEY_ESCAPE:
        if (action == GLFW_PRESS)
            glfwSetWindowShouldClose(win, true);
    }
}

// 메인루프에서 호출하여 키 눌렸는지 확인하는 용도
void VEwindow::keyHandle() {
    for (int i = 0; i < KEYS; i++) {
        if (!pressed[i]) continue;
        switch (i) {
        case GLFW_KEY_P:
            exit(-1);
        default:
            break;
        }
    }
}

void key_callback(GLFWwindow* win, int key, int scancode, int action, int mods) {
	auto input = glfwGetWindowUserPointer(win);
	VEwindow::keyProcess(win, key, scancode, action, mods);
}
