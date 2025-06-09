#version 460
// #extension GL_ARB_separate_shader_objects : enable

// The position of this vertex.
// One of:
// A = (0, 0)
// B = (0, 1)
// C = (1, 1)
// D = (1, 0)
//
// A--------D
// |        |
// |        |
// |        |
// B--------C
layout(location = 0) in ivec2 inLocation;

layout(binding = 0) uniform UBO {
  mat4 projection;
  mat4 inverseProjectionMatrix;
} ubo;

layout(push_constant) uniform PushConstants {
  vec4 textureQuad;
  vec4 fragQuad;
  vec4 color;
  vec4 position;
  vec4 size;
  vec4 lightMaskQuad;

  float writeToLightMask;
  int flags;
}
pc;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec4 fragTexBoundary;
layout(location = 3) out float fragOpacity;
layout(location = 4) out float writeToLightMask;
layout(location = 5) out vec4 fragLightMaskBoundary;


out gl_PerVertex { vec4 gl_Position; };

void main() {
  float opacity = pc.color.w;
  vec4 color = pc.color;

  vec2 pos = vec2(pc.position.x, pc.position.y);
  pos.x += inLocation.x * pc.size.x;
  pos.y += inLocation.y * pc.size.y;

  // Note: OpenGL uses inverted y axis while Vulkan does not. This difference
  // is corrected by the projection.
  gl_Position = ubo.projection * vec4(pos.x, pos.y, 0.0, 1.0);
  // gl_Position = vec4(inLocation * 2 - 1, 0.0f, 1.0f);
  /*
  gl_Position = ubo.projection * vec4(0, 0, 0.0, 1.0);

  if (inLocation.x == 0 && inLocation.y == 0) {
    color = vec4(1.0f, 0.0f, 0.0f, 1.0f);
  }
  if (inLocation.x == 0 && inLocation.y == 1) {
    color = vec4(0.0f, 1.0f, 0.0f, 1.0f);
  }
  if (inLocation.x == 1 && inLocation.y == 1) {
    color = vec4(0.0f, 0.0f, 1.0f, 1.0f);
  }
  if (inLocation.x == 1 && inLocation.y == 0) {
    color = vec4(0.0f, 0.0f, 0.0f, 1.0f);
  }
  */

  vec2 texCoord;
  texCoord.x = inLocation.x == 0 ? pc.textureQuad.x : pc.textureQuad.z;
  // y=0 uses the larger y component because the texture atlases are saved as
  // BMP, and BMP images are stored "upside down", i.e. y grows upwards instead
  // of downwards.
  texCoord.y = inLocation.y == 0 ? pc.textureQuad.w : pc.textureQuad.y;

  fragColor = pc.color;
  fragTexCoord = texCoord;
  fragTexBoundary = pc.fragQuad;
  fragOpacity = opacity;
  writeToLightMask = pc.writeToLightMask;
  fragLightMaskBoundary = pc.lightMaskQuad;
}