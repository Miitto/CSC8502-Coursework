#version 460 core

layout(vertices = 4) out;

layout(std140, binding = 0) uniform CameraMats {
    mat4 view;
    mat4 proj;
    mat4 viewProj;
    mat4 invView;
    mat4 invProj;
    mat4 invViewProj;
} CAM;

struct LODSettings {
    float minDistance;
    float maxDistance;
    float minTessLevel;
    float maxTessLevel;
};

const LODSettings LOD = {
    20.0,    // minDistance
    2000.0,   // maxDistance
    1.0,     // minTessLevel
    64.0     // maxTessLevel
};

in Vertex {
    vec2 uv;
} IN[];

out Vertex {
    vec2 uv;
    float dist;
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
    vec3 cameraPos = vec3(CAM.invView[3]);
    vec3 dir00 = gl_in[0].gl_Position.xyz - cameraPos;
    vec3 dir01 = gl_in[1].gl_Position.xyz - cameraPos;
    vec3 dir10 = gl_in[2].gl_Position.xyz - cameraPos;
    vec3 dir11 = gl_in[3].gl_Position.xyz - cameraPos;

    // ----------------------------------------------------------------------
    // Step 3: "distance" from camera scaled between 0 and 1
    float distDelta = LOD.maxDistance - LOD.minDistance;

    float distance00 = (abs(length(dir00))-LOD.minDistance) / distDelta;
    float distance01 = (abs(length(dir01))-LOD.minDistance) / distDelta;
    float distance10 = (abs(length(dir10))-LOD.minDistance) / distDelta;
    float distance11 = (abs(length(dir11))-LOD.minDistance) / distDelta;

    switch (gl_InvocationID) {
      case 0: OUT[gl_InvocationID].dist = distance00; break;
      case 1: OUT[gl_InvocationID].dist = distance01; break;
      case 2: OUT[gl_InvocationID].dist = distance10; break;
      case 3: OUT[gl_InvocationID].dist = distance11; break;
    }

    float clamped00 = clamp(distance00, 0.0, 1.0);
    float clamped01 = clamp(distance01, 0.0, 1.0);
    float clamped10 = clamp(distance10, 0.0, 1.0);
    float clamped11 = clamp(distance11, 0.0, 1.0);

    // ----------------------------------------------------------------------
    // Step 4: interpolate edge tessellation level based on closer vertex
    float tessLevel0 = mix( LOD.maxTessLevel, LOD.minTessLevel, min(clamped10, clamped00) );
    float tessLevel1 = mix( LOD.maxTessLevel, LOD.minTessLevel, min(clamped00, clamped01) );
    float tessLevel2 = mix( LOD.maxTessLevel, LOD.minTessLevel, min(clamped01, clamped11) );
    float tessLevel3 = mix( LOD.maxTessLevel, LOD.minTessLevel, min(clamped11, clamped10) );

    // ----------------------------------------------------------------------
    // Step 5: set the corresponding outer edge tessellation levels
    gl_TessLevelOuter[0] = tessLevel0;
    gl_TessLevelOuter[1] = tessLevel1;
    gl_TessLevelOuter[2] = tessLevel2;
    gl_TessLevelOuter[3] = tessLevel3;

    // ----------------------------------------------------------------------
    // Step 6: set the inner tessellation levels to the max of the two parallel edges
    gl_TessLevelInner[0] = max(tessLevel1, tessLevel3);
    gl_TessLevelInner[1] = max(tessLevel0, tessLevel2);
  }
}