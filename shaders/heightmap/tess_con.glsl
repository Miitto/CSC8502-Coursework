#version 460 core

layout(vertices = 4) out;

in Vertex {
    vec2 uv;
} IN[];

out Vertex {
    vec2 uv;
} OUT[];

in gl_PerVertex {
    vec4 gl_Position;
    float gl_PointSize;
    float gl_ClipDistance[];
} gl_in[gl_MaxPatchVertices];

out gl_PerVertex {
    vec4 gl_Position;
    float gl_PointSize;
    float gl_ClipDistance[];
} gl_out[gl_MaxPatchVertices];

void main() {
  gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
  OUT[gl_InvocationID].uv = IN[gl_InvocationID].uv;

  if (gl_InvocationID == 0) {
    gl_TessLevelOuter[0] = 512.0;
    gl_TessLevelOuter[1] = 512.0;
    gl_TessLevelOuter[2] = 512.0;
    gl_TessLevelOuter[3] = 512.0;

    gl_TessLevelInner[0] = 512.0;
    gl_TessLevelInner[1] = 512.0;
  }
}