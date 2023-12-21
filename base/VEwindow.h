#pragma once

#include <GLFW/glfw3.h>
#include <string>

const int WIDTH = 1200;
const int HEIGHT = 720;

const int KEYS = GLFW_KEY_MENU + 1;

static bool pressed[KEYS] {false};

class VEwindow {
protected:
	GLFWwindow* window;
public:
	void setUpWindow(const char* title, int = WIDTH, int = HEIGHT);
	void cleanUpWindow();

	void setWindowTitle(std::string);

	void keyHandle();
	static void keyProcess(GLFWwindow*, int, int, int, int);
};

void key_callback(GLFWwindow*, int, int, int, int);