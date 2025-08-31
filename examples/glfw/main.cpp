#ifdef __APPLE__
#define GLFW_EXPOSE_NATIVE_COCOA
#else
#define GLFW_EXPOSE_NATIVE_WIN32
#endif

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <lumin.h>

#include <iostream>
#include <chrono>
#include <thread>
#include <cmath>

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

	// Improve HiDPI behavior and per-monitor scaling where supported
	#ifdef GLFW_SCALE_TO_MONITOR
	glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
	#endif
	#ifdef __APPLE__
		#ifdef GLFW_COCOA_RETINA_FRAMEBUFFER
		glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_TRUE);
		#endif
	#endif

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
	#ifdef __APPLE__
	id nsWindow = glfwGetCocoaWindow(window);
	lumin::ConfigureWallpaperWindow(nsWindow, monitorInfo);
	#else
	HWND hwnd = glfwGetWin32Window(window);
	lumin::ConfigureWallpaperWindow(hwnd, monitorInfo);
	#endif

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

		// Animate background color to show frames are rendering.
		double timeSeconds = glfwGetTime();
		float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(timeSeconds) * 2.0f);
		glClearColor(0.08f + 0.12f * pulse, 0.15f + 0.25f * pulse, 0.25f + 0.35f * pulse, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// Ensure viewport matches framebuffer size (HiDPI aware)
		int winW = 0, winH = 0;
		int fbW = 0, fbH = 0;
		glfwGetWindowSize(window, &winW, &winH);
		glfwGetFramebufferSize(window, &fbW, &fbH);
		glViewport(0, 0, fbW, fbH);

		// Cursor debug visualization using scissor rectangles (core-profile safe).
		int mouseLogicalX = lumin::GetMouseX();
		int mouseLogicalYTopOrigin = lumin::GetMouseY();

		// Convert logical/window coordinates (top-left origin) to framebuffer coords (bottom-left origin)
		double scaleX = (winW > 0) ? static_cast<double>(fbW) / static_cast<double>(winW) : 1.0;
		double scaleY = (winH > 0) ? static_cast<double>(fbH) / static_cast<double>(winH) : 1.0;
		int mouseXFb = static_cast<int>(std::round(mouseLogicalX * scaleX));
		int mouseYFbBottomOrigin = static_cast<int>(
			std::round((static_cast<double>(winH) - static_cast<double>(mouseLogicalYTopOrigin)) * scaleY)
		);

		// Clamp to framebuffer bounds
		if (mouseXFb < 0) mouseXFb = 0;
		if (mouseXFb > fbW) mouseXFb = fbW;
		if (mouseYFbBottomOrigin < 0) mouseYFbBottomOrigin = 0;
		if (mouseYFbBottomOrigin > fbH) mouseYFbBottomOrigin = fbH;

		glEnable(GL_SCISSOR_TEST);

		// Scale line/box sizes to look consistent on HiDPI
		int lineThickness = static_cast<int>(std::round(std::max(scaleX, scaleY) * 2.0));
		if (lineThickness < 1) lineThickness = 1;

		// Draw vertical line (crosshair) at mouse X
		int vx = mouseXFb - lineThickness / 2;
		if (vx < 0) vx = 0;
		if (vx + lineThickness > fbW) vx = fbW - lineThickness;
		glScissor(vx, 0, lineThickness, fbH);
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// Draw horizontal line (crosshair) at mouse Y
		int hy = mouseYFbBottomOrigin - lineThickness / 2;
		if (hy < 0) hy = 0;
		if (hy + lineThickness > fbH) hy = fbH - lineThickness;
		glScissor(0, hy, fbW, lineThickness);
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// Draw a small box centered at the cursor that changes color with time
		int boxSize = static_cast<int>(std::round(10.0 * std::max(scaleX, scaleY)));
		if (boxSize < 6) boxSize = 6;
		int bx = mouseXFb - boxSize / 2;
		int by = mouseYFbBottomOrigin - boxSize / 2;
		if (bx < 0) bx = 0;
		if (by < 0) by = 0;
		if (bx + boxSize > fbW) bx = fbW - boxSize;
		if (by + boxSize > fbH) by = fbH - boxSize;
		glScissor(bx, by, boxSize, boxSize);
		glClearColor(1.0f * (1.0f - pulse), 0.4f + 0.6f * pulse, 0.2f + 0.3f * (1.0f - pulse), 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		glDisable(GL_SCISSOR_TEST);

		// Periodic console logging of cursor position for debugging
		static double lastLogTime = 0.0;
		if (timeSeconds - lastLogTime > 0.25) {
			lastLogTime = timeSeconds;
			float contentScaleX = 1.0f, contentScaleY = 1.0f;
			#ifdef GLFW_VERSION_MAJOR
			glfwGetWindowContentScale(window, &contentScaleX, &contentScaleY);
			#endif
			std::cout
				<< "Mouse logical(top-left): x=" << mouseLogicalX << ", y=" << mouseLogicalYTopOrigin
				<< " | framebuffer(bottom-left): x=" << mouseXFb << ", y=" << mouseYFbBottomOrigin
				<< " | win=" << winW << "x" << winH
				<< " fb=" << fbW << "x" << fbH
				<< " scale=" << scaleX << "," << scaleY
				<< " contentScale=" << contentScaleX << "," << contentScaleY
				<< std::endl;
		}

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
