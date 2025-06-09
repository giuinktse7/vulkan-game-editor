#version 460


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
//
layout(location = 0) in ivec2 inPosition;

layout(binding = 0) uniform UBO {
    mat4 projectionMatrix;
    mat4 inverseProjectionMatrix;
}
ubo;

layout(push_constant) uniform LightBlock {
    vec4 color;
    vec2 position;
    float intensity;
    int z;
}
light;

out gl_PerVertex { vec4 gl_Position; };

void main() {
    const int TILE_SIZE = 32;

    const float radiusInPixels = TILE_SIZE * light.intensity;
    // ({0, 1}, {0, 1})
    const vec2 corner = inPosition;

    // vec2 pos = light.position - (radiusInPixels - 1) * (1 - corner) + (radiusInPixels + 1) * corner;
    // TODO Remove this, just for debugging
    float EXTRA_DEBUG_AREA = 0.0f;
    vec2 pos = light.position - (radiusInPixels - 1 + EXTRA_DEBUG_AREA) * (1 - corner) + (radiusInPixels + 1 + EXTRA_DEBUG_AREA) * corner;

    gl_Position = ubo.projectionMatrix * vec4(pos, 0.0f, 1.0f);
}
