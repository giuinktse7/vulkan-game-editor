#version 460

layout(binding = 0) uniform UBO {
    mat4 projectionMatrix;
    mat4 inverseProjectionMatrix;
    ivec4 occlusion[1024];
}
ubo;

layout(binding = 1) buffer SSBO {
    int width;
    int height;
    int bufferWidth;
    int bufferHeight;

    int bufferIndexOffsetX;
    int bufferIndexOffsetY;
    int _padding[2];

    ivec4 occlusion[];
}
occlusionData;

layout (binding = 2) uniform sampler2D lightMask;


layout(push_constant) uniform LightBlock {
    vec4 color;
    vec2 position;
    float intensity;
    int z;
}
light;

layout(location = 0) out vec4 outColor;

const vec4 GREEN = vec4(0.0f, 1.0f, 0.0f, 0.5f);
const vec4 RED = vec4(1.0f, 0.0f, 0.0f, 0.5f);
const vec4 BLUE = vec4(0.0f, 0.0f, 1.0f, 0.5f);
const vec4 MAGENTA = vec4(1.0f, 0.0f, 1.0f, 0.5f);

const vec3 BLACK = vec3(0.0f);
const vec4 ZERO = vec4(0.0f);
const vec3 ROOF_OCCLUSION_COLOR = BLACK.xyz;

const int width = 30;
const int TILE_SIZE = 32;
const int HALF_TILE_SIZE = TILE_SIZE / 2;
const float TILE_SIZE_F = float(TILE_SIZE);
const int MAX_LIGHT_INTENSITY = 8;

uint get_occlusion(int index)
{
    int i = light.z * occlusionData.width * occlusionData.height + index;
    int vec_index = int(floor((i) / 4));
    // Same thing as index % 4, but faster
    // int i_0_r = index % 4;
    int i_0_r = i & 3;
    return int(occlusionData.occlusion[vec_index][i_0_r]);
}

uint getOcclusionFromWorldPos(vec2 worldPos) {
    const int width = occlusionData.width;
    const int height = occlusionData.height;

    ivec2 gamePos = ivec2(worldPos / TILE_SIZE_F);
    
    int x0 = occlusionData.bufferIndexOffsetX + gamePos.x;
    int y0 = occlusionData.bufferIndexOffsetY + gamePos.y;

    ivec2 localPos = ivec2(x0 % width, y0 % height);
    int index = localPos.y * width + localPos.x;
    return get_occlusion(index);
}

    


// https://stackoverflow.com/a/30545544
float distanceToRectangle(vec2 uv, vec2 tl, vec2 br)
{
    vec2 d = max(tl - uv, uv - br);
    return length(max(vec2(0.0), d)) + min(0.0, max(d.x, d.y));
}

bool has(uint k, uint i) {
    return (k & (1 << i)) != 0;
}

bool is_occluded(uint k) {
    return (k & 1) == 1;
}

float computeDistanceToTargetForOccluded(vec2 worldPos, vec2 center, vec2 our_quadrant_top_left, ivec2 s, uint k_0, uint k_x, uint k_y, uint k_diagonal) {
    uint empty_x = uint(!is_occluded(k_x));
    uint empty_y = uint(!is_occluded(k_y));

    // Method using an if statement - Faster at least in extreme test cases
    if (empty_x == 0 && empty_y == 0) {
        vec2 target_point = center + HALF_TILE_SIZE * s;
        return HALF_TILE_SIZE - abs(length(worldPos - target_point));
    } else {
        // return 100;
        // This must be ivec2, doesn't work with vec2 for some reason
        // Negated because we want to move in a direction opposite to the quadrant sign (s.x, s.y)
        ivec2 delta = -HALF_TILE_SIZE * s * ivec2(empty_x, empty_y);

        vec2 target_quadrant_top_left = our_quadrant_top_left + delta;
        vec2 target_quadrant_bottom_right = target_quadrant_top_left + vec2(HALF_TILE_SIZE);

        return abs(distanceToRectangle(worldPos, target_quadrant_top_left, target_quadrant_bottom_right));
    }
}

vec4 computeLightMask(vec2 worldPos) {
    // Width of the viewport (in game space, 1 = 1 tile)

    // Is this delta necessary?
    worldPos += 0.5f;

    const int width = occlusionData.width;
    const int height = occlusionData.height;

    ivec2 gamePos = ivec2(worldPos / TILE_SIZE_F);
    
    int x0 = occlusionData.bufferIndexOffsetX + gamePos.x;
    int y0 = occlusionData.bufferIndexOffsetY + gamePos.y;


    // Draws a tile grid
    // if ((int(worldPos.x) & 31) == 0 || (int(worldPos.y) & 31) == 0) {
    //     return vec4(vec3(1.0f), 0.25f);
    // }
  

    ivec2 pos_0 = ivec2(x0 % width, y0 % height);
    uint k_0 = get_occlusion(pos_0.y * width + pos_0.x);

    if (!is_occluded(k_0)) {
        return ZERO;
    }

    // return vec4(BLACK, 1.0f);
    // return RED;


    ivec2 center = ivec2(floor(worldPos / TILE_SIZE_F) * TILE_SIZE_F + HALF_TILE_SIZE);
    ivec2 s = ivec2(sign(worldPos - center));

    // Horizontal occlusion
    ivec2 pos_kx = ivec2((x0 + s.x) % width, y0 % height);
    uint k_x = get_occlusion(pos_kx.y * width + pos_kx.x);

    // Vertical occlusion
    ivec2 pos_ky = ivec2(x0 % width, (y0 + s.y) % height);
    uint k_y = get_occlusion(pos_ky.y * width + pos_ky.x);

    // Diagonal occlusion
    ivec2 pos_kdiag = ivec2((x0 + s.x) % width, (y0 + s.y) % height);
    uint k_diagonal = get_occlusion(pos_kdiag.y * width + pos_kdiag.x);

    if (is_occluded(k_x) && is_occluded(k_y) && is_occluded(k_diagonal)) {
        return vec4(ROOF_OCCLUSION_COLOR, 1.0f);
    }


    // For quadrant debugging - KEEP
    // if (k_0 == 0 && k_x == 0 && k_y == 0) {
    //     return vec4(0.0f, 0.0f, 0.0f, 1.0f);
    // } else {
    //     return vec4(clamp(k_0, 0, 1), clamp(k_x, 0, 1), clamp(k_y, 0, 1), 1.0f);
    // }



    vec2 our_quadrant_top_left = floor(worldPos / HALF_TILE_SIZE) * HALF_TILE_SIZE;

    float dist_to_target = computeDistanceToTargetForOccluded(worldPos, center, our_quadrant_top_left, s, k_0, k_x, k_y, k_diagonal);
    if (dist_to_target == 100) {
        return vec4(vec3(k_x & 1, 0, 0), 1.0f);
        // return vec4(vec3(k_x & 1, k_y & 1, k_diagonal & 1), 1.0f);
        return GREEN;
    }
    float f_0 = 1 - dist_to_target / HALF_TILE_SIZE;
    float interpolation_value = f_0;
    vec4 roofColor = vec4(ROOF_OCCLUSION_COLOR, clamp(interpolation_value, 0, 1));


    return roofColor;
}

// Returns the position in Vulkan clip space
vec2 getClipSpacePos(int screenWidth, int screenHeight) {
    // gl_FragCoord is in [0, screenWidth] x [0, screenHeight]
    return gl_FragCoord.xy / vec2(screenWidth, screenHeight) * 2 - 1;
}

float light_attenuation(float intensity, float distanceInTiles) {
    float k = intensity;
    float d = distanceInTiles;

    // float c1 = 0.0f;
    // float c2 = 1.0f / (k * k);
    // float c3 = 1.0f / k;

    float c1 = 0.0f;
    float c2 = k / 4.0f;
    float c3 = 1.0f / k;


    return clamp((k - d) / (k - d + c1 + c2 * d + c3 * d * d), 0.0f, 1.0f);
}

float remeresLightAttenuation(float d, float intensity) {
    if (d > MAX_LIGHT_INTENSITY) {
        return 0.f;
    }

    intensity = (-d + intensity) * 0.2f;
    if (intensity < 0.01f) {
        return 0.f;
    }

    return min(intensity, 1.f);
}

void main() {
    const int TILE_SIZE = 32;
    const int HALF_TILE_SIZE = TILE_SIZE / 2;
    const float TILE_SIZE_F = float(TILE_SIZE);

    // Temporary solution for ambience
    // if (light.intensity == 0.0f) {
    //     outColor = vec4(vec3(0.0f), 1.0f);
    //     return;
    // }

    
    vec2 vulkanClipSpacePos = getClipSpacePos(occlusionData.bufferWidth, occlusionData.bufferHeight);
    // TODO Should ivec2 be used here?
    ivec2 worldPos = ivec2((ubo.inverseProjectionMatrix * vec4(vulkanClipSpacePos, 0.0, 1.0)).xy);

    // Move the light into the exact center of the tile
    vec2 lightPosV2 = light.position + 0.5f;

    // Check if light is occluded
    uint k_0 = getOcclusionFromWorldPos(lightPosV2);
    bool isLightOccluded = is_occluded(k_0);
    // bool isLightOccluded = false;

    float distanceFromLight = (length(lightPosV2 - worldPos));
    float d = distanceFromLight / TILE_SIZE;

    float attenuation = remeresLightAttenuation(d, light.intensity);

    vec4 lightColor = vec4(light.color.xyz * attenuation, 1.0f);

    vec4 occlusionMask = computeLightMask(vec2(worldPos));

    vec4 roofColor;

    // TODO This logic can maybe be improved
    if (!isLightOccluded)
    {
        float w = occlusionData.bufferWidth;
        float h = occlusionData.bufferHeight;
        vec2 testPos = gl_FragCoord.xy / vec2(w, h);
        vec4 blockColor = texture(lightMask, testPos);
        if (blockColor.rgb == vec3(1.0f)) {
            roofColor = lightColor;
        } else {
            roofColor = mix(lightColor, occlusionMask, occlusionMask.a);
        }
    } else {
        roofColor = mix(lightColor, occlusionMask, occlusionMask.a);
    }



    // The light attenuation is used to mask indoor shadows in the combined.frag shader.
    // The light attenuation is sent to the combined.frag shader as the alpha channel of this texture.
    // This requires that the alpha channel blends mode is set to VK_BLEND_OP_MAX
    roofColor.a = attenuation;

    // V Actual value we should be returning!
    outColor = roofColor;
    
    // outColor = blockColor;
    // outColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);

    // outColor = vec4(0.0f, 1.0f, 0.0f, 1.0f);
    // outColor = vec4(0.0f, 0.0f, 0.0f, 1.0f);


    // For debugging the clip space rectangle
    // {
    //     vec4 debugColor = vec4(0.0f, 1.0f, 0.0f, 1.0f);
    //     vec4 debugResult = vec4(1.0f);
    //     if (attenuation > 0.00001) {
    //         debugResult = vec4(vec3(1.0f), attenuation);
    //     } else {
    //         debugResult = vec4(0.0f, 0.0f, 0.0f, 0.0f);
    //     }

    //     outColor = mix(debugColor, debugResult, 0.5f);
    // }
    
    

    // This is faster than the naive version:
    // outColor = lightColor * vec4(vec3(i), 1.0f);
    // const vec2 constantList = vec2(1.0, 0.0);
    // vec4 roofColor = lightColor * lightMask * constantList.xxxy + constantList.yyyx;
    // outColor = roofColor;
    
    // For debugging
    // outColor = lightColor * vec4(vec3(i), 1.0f);
    // outColor = vec4(light.color.xyz, 1.0f);
    // outColor = vec4(gl_FragCoord.x / 960, 0.0f, 0.0f, 1.0f);
    // outColor = vec4(worldPos.x / 960, gl_FragCoord.y / 800, 0.0f, 1.0f);
    // outColor = vec4(vec3(i), 1.0f);
}

// Visualizes the s values for the current tile
// vec4 visualize_s(uint k_0, uint k_x, uint k_y, ivec2 s) {
//     if (k_0 == 1 && k_x == 1 && k_y == 1) {
//         return vec4(0.0f, 0.0f, 0.0f, 1.0f);
//     }
//     else if (s.x == -1 && s.y == -1) {
//         return vec4(1.0f, 0.0f, 0.0f, 1.0f); // Red: both negative
//     }
//     else if (s.x == 1 && s.y == -1) {
//         return vec4(0.0f, 1.0f, 0.0f, 1.0f); // Green: s.x positive
//     }
//     else if (s.x == -1 && s.y == 1) {
//         return vec4(0.0f, 0.0f, 1.0f, 1.0f); // Blue: s.y positive
//     }
//     else if (s.x == 1 && s.y == 1) {
//         return vec4(1.0f, 1.0f, 0.0f, 1.0f); // Yellow: both positive
//     }
//     else {
//         return vec4(0.0f, 0.0f, 0.0f, 1.0f); 
//     }
// }

// Visualizes the occlusion values for the current tile
// vec4 visualize_k(uint k_0, uint k_x, uint k_y, uint k_diagonal) {
//     if (k_0 == 1 && k_x == 1 && k_y == 1) {
//         return vec4(0.0f, 0.0f, 0.0f, 1.0f);
//     }
//     else if (k_0 == 0) {
//         return vec4(1.0f, 0.0f, 0.0f, 1.0f); // Red
//     }
//     else if (k_x == 0) {
//         return vec4(0.0f, 1.0f, 0.0f, 1.0f); // Green
//     }
//     else if (k_y == 0) {
//         return vec4(0.0f, 0.0f, 1.0f, 1.0f); // Blue
//     }
//     else {
//         return vec4(0.0f, 0.0f, 0.0f, 1.0f); // Black
//     }
// }


// Visualizes the target quadrant for the current tile
// vec4 visualize_target_quadrant(uint halfTileSize, uint k_0, uint k_x, uint k_y, vec2 target_quadrant_top_left, vec2 tile_top_left) {
//     if (k_0 == 1 && k_x == 1 && k_y == 1) {
//         return vec4(0.0f, 0.0f, 0.0f, 1.0f);
//     } else {
//         if (target_quadrant_top_left == tile_top_left) {
//             return vec4(1.0f, 0.0f, 0.0f, 1.0f); // ◰ Red
//         }
//         else if (target_quadrant_top_left == tile_top_left + vec2(halfTileSize, 0)) {
//             return vec4(0.0f, 1.0f, 0.0f, 1.0f); // ◳ Green
//         }
//         else if (target_quadrant_top_left == tile_top_left + vec2(0, halfTileSize)) {
//             return vec4(0.0f, 0.0f, 1.0f, 1.0f); // ◱ Blue
//         }
//         else if (target_quadrant_top_left == tile_top_left + vec2(halfTileSize)) {
//             return vec4(1.0f, 1.0f, 0.0f, 1.0f); // ◲ Yellow
//         }
//         else {
//             return vec4(0.0f, 0.0f, 0.0f, 1.0f);
//         }
//     }
// }