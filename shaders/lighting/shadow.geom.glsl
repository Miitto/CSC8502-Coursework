#version 460 core

layout (triangles) in;

layout (triangle_strip, max_vertices = 18) out;

layout(std140, binding = 5) uniform LightUniforms {
  mat4 shadowMatrix[6];
  vec3 lightPos;
  float radius;
} U;

out Vertex {
    vec4 fragPos;
} OUT;

void main() {
  for (int face = 0; face < 6; ++face) {
    gl_Layer = face;

    for (int vertex = 0; vertex < 3; ++vertex) {
      OUT.fragPos = gl_in[vertex].gl_Position;
      gl_Position = U.shadowMatrix[face] * OUT.fragPos;
      EmitVertex();
    }
    EndPrimitive();
  }
}