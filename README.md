# Lumin - Live Wallpaper SDK 
![Logo](images/Logo.png)

This library allows you to create dynamic wallpapers on Windows, macOS, and Linux   
using the game or rendering engine of your choice.

## Previews
![Matrix](images/matrix.webp)
![Wave](images/wave.webp)

## Features

- Supports Windows 11 24H2 and prior.
- Provides mouse input replacements for interactive desktops.
- Supports multi‑monitor setups and is DPI aware.
- Doesn't render if the wallpaper or monitor is occluded.

## Getting Started

### Installation

Simply add the library to your CMake config
```cmake
# Pin a tag or branch
set(LWP_GIT_TAG main CACHE STRING "Git tag/branch for LuminWallpaper")

# Fetch LuminWallpaper
FetchContent_Declare(
  lumin
  GIT_REPOSITORY https://github.com/jensroth-git/LuminWallpaper.git
  GIT_TAG ${LWP_GIT_TAG}
)
FetchContent_MakeAvailable(lumin)

# Add to target
target_link_libraries(myApp PRIVATE lumin)
```

### General integration
The only requirement is that you get a handle to the window the engine is using to render and pass it into 
`lumin::ConfigureWallpaperWindow(engineWindowHandle, monitorInfo);`  
along with the info for which monitors desktop should be replaced.

- `HWND` on Windows 
- `NSWindow*` (Objective‑C `id`) on macOS
- `Window` (X11) on Linux

### raylib integration
```cpp
#include <iostream>

#include <lumin.h>
#include <raylib.h>

int main()
{
	// Initializes desktop replacement magic
	lumin::Initialize();

	// Sets up the desktop (-1 is the entire desktop spanning all monitors)
	lumin::MonitorInfo monitorInfo = lumin::GetWallpaperTarget(-1);

	// Initialize the raylib window.
	InitWindow(monitorInfo.width, monitorInfo.height, "Raylib Desktop Demo");

	// Retrieve the handle for the raylib-created window.
	void *raylibWindowHandle = GetWindowHandle();

	// Reparent the raylib window to the window behind the desktop icons.
	lumin::ConfigureWallpaperWindow(raylibWindowHandle, monitorInfo);

	// Now, enter the raylib render loop.
	SetTargetFPS(60);

	// Main render loop.
	while (!WindowShouldClose()) {
		// Update the mouse state of the replacement api.
		lumin::UpdateMouseState();

		// Skip rendering if the wallpaper is occluded more than 95%.
		if (lumin::IsMonitorOccluded(monitorInfo, 0.95)) {
			std::cout << "Wallpaper is occluded" << std::endl;
			WaitTime(0.1);
			continue;
		}

		if (lumin::IsDesktopLocked() ) {
			std::cout << "Desktop is locked" << std::endl;
			// If the desktop is locked, we can skip rendering.
			// This is useful to avoid unnecessary rendering when the user is not interacting with the desktop.
			WaitTime(0.1);
			continue;
		}

		// Begin the drawing phase.
		BeginDrawing();
		ClearBackground(RAYWHITE);

		DrawText(TextFormat("Mouse: %d, %d", lumin::GetMouseX(), lumin::GetMouseY()), 10, 10, 30, DARKGRAY);

		EndDrawing();
	}

	// Close the window and unload resources.
	CloseWindow();

	// Clean up the desktop window.
	// Restores the original wallpaper.
	lumin::Cleanup();

	return 0;
}
```

### GLFW integration
```cpp
#if defined(__APPLE__)
#define GLFW_EXPOSE_NATIVE_COCOA
#elif defined(_WIN32) || defined(_WIN64)
#define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(__linux__)
#define GLFW_EXPOSE_NATIVE_X11
//TODO: Test on Linux
#endif

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

// use std::max and std::min from algorithm instead of max and min macros
#undef max
#undef min

#include <lumin.h>

#include <iostream>
#include <algorithm>
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
	#if defined(GLFW_SCALE_TO_MONITOR)
	glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
	#endif
	#if defined(__APPLE__)
		#if defined(GLFW_COCOA_RETINA_FRAMEBUFFER)
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
	#if defined(__APPLE__)
	id nsWindow = glfwGetCocoaWindow(window);
	lumin::ConfigureWallpaperWindow(nsWindow, monitorInfo);
	#elif defined(_WIN32) || defined(_WIN64)
	HWND hwnd = glfwGetWin32Window(window);
	lumin::ConfigureWallpaperWindow(hwnd, monitorInfo);
	#elif defined(__linux__)
	Window x11Window = glfwGetX11Window(window);
	lumin::ConfigureWallpaperWindow(x11Window, monitorInfo);
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

		// Clear the screen with a dark blue color.
		glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// Draw here ... 

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
```

### Unity integration
TODO...

### Unreal integration
TODO...

## Future Plans
Currently, there are no replacements for keyboard input, which may be added in the future.

## License

This project is licensed under the MIT License.
