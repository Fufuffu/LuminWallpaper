#include <Windows.h>

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#pragma comment(lib, "opengl32.lib")

#include <lumin.h>

#include <iostream>
#include <chrono>
#include <thread>

int main()
{
	lumin::Initialize();

	lumin::MonitorInfo monitor = lumin::GetWallpaperTarget(-1);

	if (!glfwInit()) {
		std::cout << "Failed to initialize GLFW" << std::endl;
		return -1;
	}

	GLFWwindow *window = glfwCreateWindow(monitor.width, monitor.height, "OpenGL Wallpaper", nullptr, nullptr);

	if (!window) {
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);

	HWND hwnd = glfwGetWin32Window(window);
	lumin::ConfigureWallpaperWindow(hwnd, monitor);

	while (!glfwWindowShouldClose(window)) {
		lumin::UpdateMouseState();

		if (lumin::IsMonitorOccluded(monitor, 0.95) || lumin::IsDesktopLocked()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}

		glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// Draw here

		glfwSwapBuffers(window);
		glfwPollEvents();

		std::this_thread::sleep_for(std::chrono::milliseconds(4)); // ~240 FPS
	}

	glfwDestroyWindow(window);
	glfwTerminate();
	lumin::Cleanup();

	return 0;
}
