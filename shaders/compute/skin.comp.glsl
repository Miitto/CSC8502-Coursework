#version 460 core

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

struct InVertex {
  vec3 position;
  vec2 uv;
  vec3 normal;
  vec4 tangent;
  vec4 weights;
  ivec4 weightIndices;
};

layout(std430, binding = 1) readonly buffer InputVertices {
  InVertex vertices[];
} IN;

struct OutVertex {
  vec3 position;
  vec2 uv;
  vec3 normal;
  vec4 tangent;
};

layout(std430, binding = 2) buffer OutputVertices {
  OutVertex vertices[];
} OUT;

layout(std430, binding = 3) readonly buffer JointMats {
    mat4 joints[];
} JOINTS;

layout(location = 0) uniform uvec4 uInfo;
layout(location = 1) uniform uint frame;

void main() {
  uint inVertexStart = uInfo.x;
  uint jointStartOffset = uInfo.y;
  uint jointCount = uInfo.z;
  uint outVertexStart = uInfo.w;

  uint vIndex = gl_GlobalInvocationID.x + inVertexStart;
  uint vOutIndex = gl_GlobalInvocationID.x + outVertexStart;

  InVertex v = IN.vertices[vIndex];

  vec4 local = vec4(v.position, 1.0);

  vec4 skelPos = vec4(0.0);

  if (jointCount != 0) {
    uint frameIndex = (frame * jointCount) + jointStartOffset;

    for (int i = 0; i < 4; ++i) {
      int jointIndex = v.weightIndices[i];
      float weight = v.weights[i];

      mat4 jointOne = JOINTS.joints[frameIndex + jointIndex];

      skelPos += jointOne * local * weight;
    }
  } else {
    skelPos = local;
  }

  OUT.vertices[vOutIndex].position = skelPos.xyz;
  OUT.vertices[vOutIndex].uv = v.uv;
  OUT.vertices[vOutIndex].normal = v.normal;
  OUT.vertices[vOutIndex].tangent = v.tangent;
}