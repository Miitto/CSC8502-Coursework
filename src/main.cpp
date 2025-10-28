#include "gl/gl.hpp"

int main() {
  auto wm = gl::WindowManager::get();

  auto window = gl::Window(800, 600, "Test Window FLOAT", true);

  while (!window.shouldClose()) {
    gl::Window::pollEvents();
    glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    window.swapBuffers();
  }

  return 0;
}
