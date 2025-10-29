#include "gl/gl.hpp"
#include "logger/logger.hpp"
#include "renderer.hpp"
#include <engine/gui.hpp>
#include <engine/input.hpp>
#include <engine/window.hpp>
#include <imgui/imgui.h>

int main() {
  Renderer app(800, 600, "CSC8502 FLOAT");

  return engine::run(app);
}
