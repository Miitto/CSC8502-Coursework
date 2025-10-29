#include "gl/gl.hpp"
#include "logger/logger.hpp"
#include <engine/gui.hpp>
#include <engine/input.hpp>
#include <engine/window.hpp>
#include <imgui/imgui.h>

int main() {
  auto& wm = engine::WindowManager::get();

  engine::Window window(800, 600, "Test Window FLOAT", true);

  engine::Input input(window);
  engine::gui::Context gui(window);

  if (!wm.isGlLoaded()) {
    Logger::critical("Failed to load OpenGL");
    return -1;
  }

  while (!window.shouldClose()) {
    engine::Window::pollEvents();

    if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0) {
      gui.sleep(10);
      continue;
    }
    gui.newFrame();
    input.imGuiWantsMouse(gui.io().WantCaptureMouse);
    input.imGuiWantsKeyboard(gui.io().WantCaptureKeyboard);

    {
      engine::gui::GuiWindow frame("Hello, ImGui!");
    }

    glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    input.frameEnd();
    gui.endFrame();
    window.swapBuffers();
  }

  return 0;
}
