#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_debug_printf : enable

layout (binding = 0) uniform UBO 
{
	mat4 projection;
} ubo;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec4 inTexRect;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec4 rect;
layout(location = 3) out float fragOpacity;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    float opacity = inColor.w;
    vec4 color = inColor;

    // Note: OpenGL uses inverted y axis while Vulkan does not. This difference
    // is corrected by the projection.
    gl_Position = ubo.projection * vec4(inPosition.x, inPosition.y, 0.0, 1.0);

  // vec2 sampleLocation = vec2(clamp(fragTexCoord.x, rect.x, rect.z), clamp(fragTexCoord.y, rect.w, rect.y));
    
    fragColor = color;
    fragTexCoord = inTexCoord;
    rect = inTexRect;
    fragOpacity = opacity;
}


