# CSC8502 Coursework

> Screenshots, controls and an overview of the project can be found in [Overview.md](Overview.md).

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

To build for release, tag `release` to the preset name:
```bash
cmake -S . --preset windows-vs-x64-release
```

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

### Terrain

The terrain is defined in `src/heightmap.hpp` and uses a heightmap image along with a tesselation shader to render the terrain with a dynamic level of detail based on the camera position.

The vertex shader uses gl_VertexID to create a grid of quads around the origin based on the `chunksPerAxis` uniform.
The tesselation shaders use the distance from the camera to determine the tesselation level for each patch, and then interpolates the values appropriatly accross the generated vertices.

### Models and Meshes

Meshes are defined in `engine/include/engine/mesh/*.hpp`. The files originated from the `nclgl` library, but have been modified to use my own framework. The mesh class has been heavily modified,
as it now holds offset data to be used in creating indirect draw calls for rendering all batchable meshes in a single draw call.

The `Renderer` class holds a `staticBuffer`, `skinnedBuffer` and `dynamicBuffer`. Both `staticBuffer` and `skinnedBuffer` are not host visible, and any data is uploaded using staging buffers.
- `staticBuffer` contains unskinend vertices, indices and joints for all `Mesh`s in the scene.
- `skinnedBuffer` contains the skinned vertices, and is populated at the start of each frame by a compute shader.
- `dynamicBuffer` is persistently mapped and is used to hold indirect draw calls, instance data and texture handles for all batchable meshes in the scene. Instance and texture data is written at the start of each frame when skinning.

### Lighting

Lighting is handled directly in the `Renderer` class. A deferred rendering pipeline is used with PBR, point and spot lights are supported.
Lights are not batched with instancing, although the groundwork has been laid to support it in future (I couldn't figure out why the VAO kept creating nullptr exceptions with an instance buffer bound, and never got back to it).

The material textures use the following channels:
- Red: Reflectivity (used for reflections in post processing)
- Green: Metalness
- Blue: Roughness
- Alpha: Ambient Occlusion (Although all used textures have an alpha of 1 for no AO)

#### Shadows

Point lights use an omnidirectional shadow map implemented with a cubemap texture. To avoid multiple draw calls for each face of the cubemap, a geometry shader is used to output all 6 faces in a single pass.
Spot lights use a standard 2D shadow map.

### Post Processing

All post processing effects use a fullscreen tri that is hard coded in the vertex shader, and the engine has a global `DUMMY_VAO` which can be used when no VAO is needed (since OpenGL no longer supports using the default VAO 0).

HDR tone mapping and bloom are implemented as the first post processing step after deferred rendering. If bloom is disabled, the lighting combine pass can also perform tone mapping without an additional post processing step being required.

The skybox is implemented as a post processing step, using the depth buffer to determine where to draw the skybox to avoid overdraw.

Reflections are implemented using the skybox texture and the normal and view direction to calculate the reflection vector. The red channel of the material texture is used to determine the reflectivity of the surface.

FXAA is implemented as a final post processing step to smooth out jagged edges.
