#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <lumin.h>

#include <iostream>
#include <chrono>
#include <thread>

int main()
{
	// Initializes desktop replacement magic.
	lumin::Initialize();

	// Sets up the desktop. (-1 is the entire desktop spanning all monitors)
	lumin::MonitorInfo monitorInfo = lumin::GetWallpaperTarget(-1);

	// Initialize GLFW
	if (!glfwInit()) {
		std::cout << "Failed to initialize GLFW" << std::endl;
		return -1;
	}

	// Create GLFW window with the size of the monitor.
	GLFWwindow *window = glfwCreateWindow(
		monitorInfo.width, monitorInfo.height, "GLFW Wallpaper", nullptr, nullptr
	);
	if (!window) {
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}

	// Make the window's context current.
	glfwMakeContextCurrent(window);

	// Retrieve the handle for the GLFW-created window and configure it with lumin.
	HWND hwnd = glfwGetWin32Window(window);
	lumin::ConfigureWallpaperWindow(hwnd, monitorInfo);

	// Enable vsync. (caps fps to monitor refresh rate)
	glfwSwapInterval(1);

	// Main render loop.
	while (!glfwWindowShouldClose(window)) {
		// Update the mouse state of the replacement api.
		lumin::UpdateMouseState();

		// Skip rendering if the wallpaper is occluded more than 95%.
		if (lumin::IsMonitorOccluded(monitorInfo, 0.95)) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}

		// Skip rendering if the desktop is locked.
		// This is useful to avoid unnecessary rendering when the user is not interacting with the desktop.
		if (lumin::IsDesktopLocked()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}

		// Clear the screen with a dark blue color.
		glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// Draw here

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// Cleanup GLFW resources.
	glfwDestroyWindow(window);
	glfwTerminate();

	// Clean up the desktop window.
	// Restores the original wallpaper.
	lumin::Cleanup();

	return 0;
}
