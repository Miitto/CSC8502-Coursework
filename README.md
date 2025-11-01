# CSC8502 Coursework

## Cloning
This project uses git submodules to manage some dependencies. When cloning the repository, make sure to include the `--recurse-submodules` flag:
```bash
git clone --recurse-submodules https://github.com/Miitto/CSC8502-Coursework.git
```

If you already cloned the repository without the flag, you can initialize and update the submodules with:
```bash
git submodule update --init --recursive
```

## Building

This project is built with `cmake`, with presets defined for Windows in `CMakePresets.json`.

You can use the following commands to build from the CLI:
```bash
cmake -S . --preset windows-msvc-host
cmake --build --preset windows-msvc-host
```
```bash
cmake -S . --preset windows-vs-x64
cmake --build --preset windows-vs-x64
```

Only the `windows-vs` presets seems to work correctly with Visual Studio's built-in CMake support. The `windows-msvc` presets error due to some precompiled header shennanigans (why can things never just work?).

## Running

OpenGL 4.6 with the `GL_ARB_bindless_texture` extension is required to run this project.

The executable will compile to `__output/<preset-name>/src/<build-type>/CSC8502-Coursework.exe`.

E.g. for the `windows-vs-x64`, the executable will be at:
```
__output/windows-vs-x64/src/Debug/CSC8502-Coursework.exe
```
and for the `windows-vs-x64-release`, it will be at:
```
__output/windows-vs-x64/src/Release/CSC8502-Coursework.exe
```

## Project Structure

- `src/` - Source code for the project.
- `shaders/` - GLSL shader source files.
- `meshes/` - 3D model, material and animation files used in the project.
- `textures/` - Texture image files used in the project.
- `engine/` - Custom game engine code used in the project.
- `engine/gl/` - OpenGL abstraction layer for rendering.
- `engine/vendor/` - Third-party libraries used by the engine that arn't fetchable using cmake's `FetchContent`.
- `engine/logger/` - Logging utilities for debugging and information output.

The libraries are split up into `src` and `include/<lib-name>` folders to allow for easier inclusion in other projects.
- `src` - Source code for the library and private headers.
- `include/<lib-name>` - Public headers for the library. They will be available to be included by `#include <lib-name/header.h>`.

### Entry
The engine is designed to call `engine::run` on a user-defined class that inherits from `engine::App`. `engine::App` handles initialization of the window, OpenGL context and the ImGUI context, as well as the post-render control such as swapping the back buffer.
`engine::run` handles the main loop, including calculating delta time. Since `engine::run` calls the `engine::App` `update` and `render` methods, the function is defined as a template to avoid polymorhpic overhead.

The user-defined class is defined in `src/renderer.hpp` and handles the scene graph, post processing and the camera.

### Camera

Cameras are defined in `engine/include/engine/camera.hpp`. There is a base `Camera` class that handles rotation and translation (view matrix properties) and a `PerspectiveCamera` class that inherits from it that handles the perspective matrix.
The camera matrices (view, projection, view-projection) as well as their inverse are written to a uniform buffer each frame so they can be accessed from any shader easily. To help with this, the render functions have access to the current camera.

The camera also handles input for moving around the scene.

### Terrain

The terrain is defined in `src/heightmap.hpp` and uses a heightmap image along with a tesselation shader to render the terrain with a dynamic level of detail based on the camera position.
The LOD parameters and terrain scale can be adjusted in the ImGUI window.

The vertex shader uses gl_VertexID to create a grid of quads around the origin based on the `chunksPerAxis` uniform.
The tesselation shaders use the distance from the camera to determine the tesselation level for each patch, and then interpolates the values appropriatly accross the generated vertices.

### Models and Meshes

Meshes are defined in `engine/include/engine/mesh.hpp`. The file originated from the `nclgl` library, and has been translated to use `glm` and my own OpenGL abstraction layer.
The largest change is how the vertex attributes are handles, as I have combined them all into one buffer. The buffer starts with the interleaved vertex attributes, followed by the index data.
Any attributes that are not present in the mesh are not present in the buffer to avoid wasting memory.

A `BatchSubmeshes` method has been added to `Mesh` to allow for rendering multiple submeshes in a single draw call. This uses `glMultiDrawElements` to render all submeshes in one call.
This is used to render the futuristic soldier model, as all the textures are setup to be accessed bindless, allowing for all submeshes to be rendered in one call.