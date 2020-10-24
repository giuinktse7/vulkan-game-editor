#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 1, binding = 0) uniform sampler2D texSampler;

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec4 rect;
layout(location = 3) in float fragOpacity;

layout(location = 0) out vec4 outColor;

/*
See:
Original thread: https://community.khronos.org/t/custom-bilinear-filtering-w-texturegather-problem/67051
Solution: https://community.khronos.org/t/custom-bilinear-filtering-w-texturegather-problem/76178
*/

vec4 textureBilinear(in sampler2D texSampler, in vec2 textureCoordinate)
{
    // Get texture size in pixels:
    vec2 colorTextureSize = vec2(textureSize(texSampler, 0));

    // Convert UV coordinates to pixel coordinates and get pixel index of top left pixel (assuming UVs are relative to top left corner of texture)
    vec2 firstPixelCoordinate = textureCoordinate * colorTextureSize - 0.5f;    // First pixel goes from -0.5 to +0.4999 (0.0 is center) last pixel goes from (size - 1.5) to (size - 0.5000001)
    vec2 originPixelCoordinate = floor(firstPixelCoordinate);              // Pixel index coordinates of bottom left pixel of set of 4 we will be blending

    vec2 sampleUV = (originPixelCoordinate + 0.5f) / colorTextureSize;

    // Sample from all surounding texels
    vec4 c00 = texture(texSampler, sampleUV);
    vec4 c01 = textureOffset(texSampler, sampleUV, ivec2(0, 1));
    vec4 c11 = textureOffset(texSampler, sampleUV, ivec2(1, 1));
    vec4 c10 = textureOffset(texSampler, sampleUV, ivec2(1, 0));

    vec3 black = vec3(0.0f);
    vec3 magenta = vec3(1.0f, 0.0f, 1.0f);
    
    if (c00.rgb == magenta) c00.rgb = black;
    if (c01.rgb == magenta) c01.rgb = black;
    if (c11.rgb == magenta) c11.rgb = black;
    if (c10.rgb == magenta) c10.rgb = black;

    // Filter weight is fract(coord * colorTextureSize - 0.5f) = (coord * colorTextureSize - 0.5f) - floor(coord * colorTextureSize - 0.5f)
    vec2 filterWeight = firstPixelCoordinate - originPixelCoordinate;
 
    // Bi-linear mixing:
    vec4 temp0 = mix(c01, c11, filterWeight.x);
    vec4 temp1 = mix(c00, c10, filterWeight.x);
    return mix(temp1, temp0, filterWeight.y);
}

void main() {
  // vec2 sampleLocation = vec2(clamp(fragTexCoord.x, rect.x, rect.z), clamp(fragTexCoord.y, rect.w, rect.y));

  // outColor = textureBilinear(texSampler, sampleLocation) * fragColor;
  outColor = textureBilinear(texSampler, fragTexCoord) * fragColor;
  // outColor = fragColor;
}
