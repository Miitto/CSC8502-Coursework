#include "logger/logger.hpp"
#include "renderer.hpp"

int main() {
  Logger::info("Starting application...");

  {
    auto err = engine::loadPreInitEnginePlugins();
    if (err.has_value()) {
      Logger::error("Failed to load pre init engine plugins: {}", err.value());
      return -1;
    }
  }

  Renderer app(800, 600, "CSC8502 FLOAT");

  int r = engine::run(app);

  if (r != 0) {
    Logger::error("Application exited with error code {}", r);
  } else {
    Logger::info("Application exited successfully");
  }

  return r;
}
