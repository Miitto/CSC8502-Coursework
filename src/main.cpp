#include "logger/logger.hpp"
#include "renderer.hpp"

int main() {
  Logger::info("Starting application...");
  Renderer app(800, 600, "CSC8502 FLOAT");

  int r = engine::run(app);

  if (r != 0) {
    Logger::error("Application exited with error code {}", r);
  } else {
    Logger::info("Application exited successfully");
  }

  return r;
}
