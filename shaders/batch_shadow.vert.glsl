#version 460 core

layout(location = 0) in vec3 position;
layout(location = 4) in mat4 modelMatrix;

void main() {
  gl_Position = modelMatrix * vec4(position, 1.0);
}
