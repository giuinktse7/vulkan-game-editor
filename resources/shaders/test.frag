#version 440

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

// layout(binding = 1) uniform sampler2D source;

layout(std140, binding = 0) uniform buf
{
    mat4 qt_Matrix;
    float qt_Opacity;
}
ubuf;

// from http://www.iquilezles.org/www/articles/distfunctions/distfunctions.htm
float roundedBoxSDF(vec2 centerPos, vec2 size, float radius)
{
    return length(max(abs(centerPos) - size + radius, 0.0)) - radius;
}

void main()
{
    vec2 fragCoord = qt_TexCoord0;

    vec2 size = vec2(1.0f, 1.0f);

    // How soft the edges should be (in pixels). Higher values could be used to simulate a drop shadow.
    float edgeSoftness = 1.0f;

    // The radius of the corners (in pixels).
    float radius = 0;

    // Calculate distance to edge.
    // float dist = roundedBoxSDF(fragCoord.xy - (size / 2.0f), size / 2.0f, radius);
    float dist = distance(fragCoord, size / 2.0f);

    // Smooth the result (free antialiasing).
    float smoothedAlpha = 1.0f - smoothstep(0.0f, edgeSoftness * 2.0f, dist);

    // Return the resultant shape.
    vec4 quadColor = mix(vec4(1.0f, 1.0f, 1.0f, 1.0f), vec4(0.0f, 0.2f, 1.0f, smoothedAlpha), smoothedAlpha);

    // Apply a drop shadow effect.
    float shadowSoftness = 0.5f;
    vec2 shadowOffset = vec2(0.0f, 0.1f);
    float shadowDistance = roundedBoxSDF(fragCoord.xy + shadowOffset - (size / 2.0f), size / 2.0f, radius);
    float shadowAlpha = 1.0f - smoothstep(-shadowSoftness, shadowSoftness, shadowDistance);
    vec4 shadowColor = vec4(0.4f, 0.4f, 0.4f, 1.0f);
    fragColor = mix(quadColor, shadowColor, shadowAlpha - smoothedAlpha);
    // fragColor = vec4(dist, dist, dist, 1.0f);
}
