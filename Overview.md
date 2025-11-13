# Coursework Overview

[Demo](https://youtu.be/PwhaKpD_pUQ)

## Controls

- `ESC` - Toggle mouse look, or take control of the camera if it is moving itself.
- `TAB` - Toggle which camera is controled by user input.
- `B` - Toggle bloom.
- `U` - Toggle Debug UI.
- `F11` - Toggle fullscreen.

## Features

### Core

- Terrain is rendered using diffuse, height and normal textures. Tesselation shaders are used to provide dynamic level of detail based on camera distance.
- There are multiple lights in each scene.
- The skybox and environmental reflections use a cubemap texture.
- The camera starts on a predefined path defined by keyframes containing a postion and orientation. These keyframes are interpolated using a cubic easing function. Quaternion slerp is used for orientation interpolation. The user can take over by pressing `ESC` at any time. While on track, effects are run at predetermined points.
- A scene graph is used to manage objects in the scene. Two scene graphs are used, one for the left camera and one for the right camera.
- The water is rendered using a quad, and the realistic character mesh is included in the first scene.

### Advanced

- Tone mapping, bloom, skybox rendering, reflections and FXAA post processing effects are implemented.
- Point and spot lights in a deferred rendering pipeline with PBR materials.
- Spot lights use standard shadow maps, and point lights use omnidirectional shadow maps implemented with a cubemap and geometry shader.
- The split camera contains two perspective cameras that split on the screen vertically. Each camera renders its own scene graph to allow for different objects to be rendered on each side (although they could easily render the same one). Uses `glViewport` and passes in the uvRange to shaders.
- The character mesh is skinned using a compute shader to offload the skinning calculations to the GPU. In the first scene there can be 105 independently positioned and animated characters on screen at once, running at over 140fps on my machine (with most of the processing seemingly being used to frustum cull). The skinned vertices are reused when rendering shadow maps.
- I do not belive I have any environmental effects implemented, other than the skybox and reflections.

### Other

- All none-custom meshes (such as the terrain and water) use bindless textures for material textures.
- All none-custom meshes (such as the terrain and water) have their data stored in one large buffer. (Although skinned vertices are stored in a separate buffer).
- All none-custom meshes (such as the terrain and water) are batched using `glMultiDrawElementsIndirect`.
- Skinning is done on the GPU using a compute shader. These vertices are used when rendering the main scene and for shadow maps.

## Screenshots

![Animated characters with bloom]("screenshots/Animated_Character_and_Bloom.png")

Shows some animated characters with bloom enabled. Lit by the sun and two point lights, red infront and blue behind.

![Shadows]("screenshots/Realtime_Shadows.png")

Shows the characters casting shadows from the light sources. Cascaded shadow maps are not implemented, so the sun shadows are rather pixelated.

![Skybox with environmental reflections]("screenshots/Skybox_reflections_100_animated_meshes.png")

Shows the skybox and environmental reflections on the water surface. Also in frame is 100 animated character meshes running at ~155fps.

![Split camera]("screenshots/100_animated_meshes_lights.png")

Shows the 100 animated meshes on the left side, and the right side shows a darker scene to better show the lighting. Includes some point lights and a spot light. Running >140fps even when rendering both scenes.
