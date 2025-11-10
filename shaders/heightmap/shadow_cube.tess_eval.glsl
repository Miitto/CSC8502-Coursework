#version 460 core

layout (quads, fractional_odd_spacing, cw) in;

layout(binding = 0) uniform sampler2D heightmap;

layout(std140, binding = 5) uniform LightUniforms {
  mat4 shadowMatrix[6];
  vec3 lightPos;
  float radius;
} U;


in Vertex {
  vec2 uv;
  float dist;
} IN[];

out Vertex {
    vec4 fragPos;
    flat vec3 lightPos;
    flat float radius;
} OUT;

void main() {
  vec2 inUv = gl_TessCoord.xy;

  vec2 t0 = mix(IN[0].uv, IN[1].uv, inUv.x);
  vec2 t1 = mix(IN[2].uv, IN[3].uv, inUv.x);
  vec2 uv = mix(t0, t1, inUv.y);

  float d0 = mix(IN[0].dist, IN[1].dist, inUv.x);
  float d1 = mix(IN[2].dist, IN[3].dist, inUv.x);
  float dist = mix(d0, d1, inUv.y);

  float height = texture(heightmap, uv).r * 500.0;

  vec4 p00 = gl_in[0].gl_Position;
  vec4 p01 = gl_in[1].gl_Position;
  vec4 p10 = gl_in[2].gl_Position;
  vec4 p11 = gl_in[3].gl_Position;

  vec4 pos0 = mix(p00, p01, inUv.x);
  vec4 pos1 = mix(p10, p11, inUv.x);
  vec4 pos = mix(pos0, pos1, inUv.y);

  pos.y += height;

  OUT.fragPos = pos;
  OUT.lightPos = U.lightPos;
  OUT.radius = U.radius;

  gl_Position = pos;
}
