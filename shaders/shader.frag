#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 1, binding = 0) uniform sampler2D texSampler;

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec4 rect;
layout(location = 3) in float fragOpacity;

layout(location = 0) out vec4 outColor;

void main() {
    vec2 sampleLocation = vec2(clamp(fragTexCoord.x, rect.x, rect.z), clamp(fragTexCoord.y, rect.w, rect.y));
    // vec2 sampleLocation = fragTexCoord;
    // vec4 color = texture(texSampler, sampleLocation) * fragColor;
    vec4 color = texture(texSampler, sampleLocation);
    // color[3] = fragOpacity;
    outColor = color;
    // outColor = vec4(fragOpacity, fragOpacity, fragOpacity, 1.0f);
}
