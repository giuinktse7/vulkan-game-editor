#version 460
// #extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UBO {
  mat4 projection;
  mat4 inverseProjectionMatrix;
} ubo;


layout(binding = 1) buffer SSBO {
    int width;
    int height;
    int bufferWidth;
    int bufferHeight;

    int bufferIndexOffsetX;
    int bufferIndexOffsetY;
    // Z used to compute the buffer index offsets
    int z0;

    int _padding[1];

    ivec4 occlusion[];
}
occlusionData;

layout(set = 1, binding = 0) uniform sampler2D texSampler;

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

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec4 fragTexBoundary;
layout(location = 3) in float fragOpacity;
layout(location = 4) in float writeToLightMask;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outMapLightMask;
layout(location = 2) out vec4 outIndoorShadow;

/*
See:
Original thread:
https://community.khronos.org/t/custom-bilinear-filtering-w-texturegather-problem/67051
Solution:
https://community.khronos.org/t/custom-bilinear-filtering-w-texturegather-problem/76178
*/


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// >>>>>>>>INDOOR SHADOW SYSTEM>>>>>>>>>>>>>
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


const vec4 GREEN = vec4(0.0f, 1.0f, 0.0f, 0.5f);
const vec4 RED = vec4(1.0f, 0.0f, 0.0f, 0.5f);
const vec4 BLUE = vec4(0.0f, 0.0f, 1.0f, 0.5f);
const vec4 MAGENTA = vec4(1.0f, 0.0f, 1.0f, 0.5f);

const float INDOOR_SHADOW_STRENGTH = 0.5f;

const vec4 COLOR_NO_INDOOR_SHADOW = vec4(1.0f);
// const vec4 COLOR_NO_INDOOR_SHADOW = vec4(0.0f);
const vec4 COLOR_FULL_INDOOR_SHADOW = vec4(vec3(INDOOR_SHADOW_STRENGTH), 1.0f);


const uint OCCLUDED = 1 << 0;
const uint INDOOR = 1 << 1;
const uint GROUND = 1 << 2;
const uint INDOOR_BORDER = 1 << 3;
const uint INDOOR_WALL = 1 << 4;
const uint ELEVATED_GROUND = 1 << 5;

// Indoor shadow condition flags
const uint CONDITION_FLAG_CLEAR_SHADOW_NORTH = 1 << 0;
const uint CONDITION_FLAG_INDOOR_SHADOW_CLEAR_SHADOW_WEST = 1 << 1;
const uint CONDITION_FLAG_INDOOR_SHADOW_CLEAR_SHADOW_EAST = 1 << 2;
const uint CONDITION_FLAG_INDOOR_SHADOW_CLEAR_SHADOW_SOUTH = 1 << 3;
const uint CONDITION_FLAG_INDOOR_SHADOW_CLEAR_SHADOW_NORTH_EAST = 1 << 4;
const uint CONDITION_FLAG_IS_OUTDOOR_WALL = 1 << 5;
const uint CONDITION_FLAG_FORCE_FULL_SHADOW = 1 << 6;
const uint CONDITION_FLAG_IS_INDOOR_WALL = 1 << 7;
const uint CONDITION_FLAG_ONLY_CONSIDER_FULLY_INDOOR_SHADOW = 1 << 8;
const uint CONDITION_FLAG_IGNORE_INDOOR_BORDERS_EAST_AND_SOUTH = 1 << 9;

const int TILE_SIZE = 32;
const int HALF_TILE_SIZE = TILE_SIZE / 2;
const float TILE_SIZE_F = float(TILE_SIZE);

uint get_occlusion(int index)
{
    int vec_index = int(floor((index) / 4));
    // Same thing as index % 4, but faster
    // int i_0_r = index % 4;
    int i_0_r = index & 3;
    return int(occlusionData.occlusion[vec_index][i_0_r]);
}

uint get_occlusion_v2(int width, int height, ivec3 pos)
{
    int index = pos.z * width * height + pos.y * width + pos.x;
    int vec_index = int(floor((index) / 4));
    // Same thing as index % 4, but faster
    // int i_0_r = index % 4;
    int i_0_r = index & 3;

    uint result = int(occlusionData.occlusion[vec_index][i_0_r]);

    // A tile with elevated ground at the border of indoor is a tile that is not indoors
    // for the purposes of shadow rendering
    // Multiplication variant:
    // int is_outdoor = int((result & (INDOOR_BORDER | ELEVATED_GROUND)) == (INDOOR_BORDER | ELEVATED_GROUND));
    // result &= ~((INDOOR | INDOOR_BORDER | INDOOR_WALL) * is_outdoor);
    //
    // Naive profiling suggest that this is faster than using the multiplication variant above.
    if ((result & (INDOOR_BORDER | ELEVATED_GROUND)) == (INDOOR_BORDER | ELEVATED_GROUND)) {
        result &= ~(INDOOR | INDOOR_BORDER | INDOOR_WALL);
    }

    // Clear indoor border if CONDITION_FLAG_ONLY_CONSIDER_FULLY_INDOOR_SHADOW is set
    if ((pc.flags & CONDITION_FLAG_ONLY_CONSIDER_FULLY_INDOOR_SHADOW) == CONDITION_FLAG_ONLY_CONSIDER_FULLY_INDOOR_SHADOW) {
        result &= ~INDOOR_BORDER;
    }

    return result;
}

// https://stackoverflow.com/a/30545544
float distanceToRectangle(vec2 uv, vec2 tl, vec2 br)
{
    vec2 d = max(tl - uv, uv - br);
    return length(max(vec2(0.0), d)) + min(0.0, max(d.x, d.y));
}

bool is_occluded(uint k) {
    return (k & 1) == 1;
}

bool is_indoor(uint k) {
    return ((k & INDOOR) == INDOOR) || ((k & INDOOR_BORDER) == INDOOR_BORDER);
}

bool is_indoor_border(uint k) {
    return (k & INDOOR_BORDER) == INDOOR_BORDER;
}

bool is_indoor_wall(uint k) {
    return (k & INDOOR_WALL) == INDOOR_WALL;
}

bool has_elevated_ground(uint k) {
    return (k & ELEVATED_GROUND) == ELEVATED_GROUND;
}

bool is_occluded_or_indoor_wall(uint k) {
    return (k & (INDOOR_WALL | OCCLUDED)) != 0;
}

bool is_fully_indoor(uint k) {
    return (k & INDOOR) == INDOOR;
}


bool is_occluded_and_indoor(uint k) {
    return ((k & 3) == 3) || ((k & 9) == 9);
}

bool is_occluded_or_indoor(uint k) {
    return is_indoor(k) || is_occluded(k);
}

bool has_flags(uint k, uint flags) {
    return (k & flags) == flags;
}

bool non_occluded_indoor(uint k)
{
    return ((k & OCCLUDED) == 0) && (((k & INDOOR) == INDOOR) || ((k & INDOOR_BORDER) == INDOOR_BORDER));
}


float computeDistanceToTargetForIndoor(vec2 worldPos, vec2 center, vec2 our_quadrant_top_left, vec2 tile_top_left, ivec2 s, uint k_0, uint k_x, uint k_y, uint k_diagonal) {
    ivec2 quadrant = clamp(s, 0, 1);

    bool indoor_x = is_indoor(k_x) || is_occluded(k_x);
    bool indoor_y = is_indoor(k_y) || is_occluded(k_y);


    int int_indoor_x = int(indoor_x);
    int int_indoor_y = int(indoor_y);

    uint empty_x = uint(!(is_indoor(k_x) || is_occluded(k_x)));
    uint empty_y = uint(!(is_indoor(k_y) || is_occluded(k_y)));

    bool circle;
    vec2 target;

    bool is_vertical_corner = ((1 - (int_indoor_x | int_indoor_y)) & quadrant.y) == 1;

    // Top-left quadrant of shadow on (991, 996, 7) should count as horizontal. It doesn't because 
    // there is a (fully) indoor tile above it.
    // TODO: Implement partial shadows to handle this case

    bool horizontal = !(indoor_y || is_vertical_corner) || (indoor_x && (quadrant.y == 0));
    if (horizontal) {
        int invert = int_indoor_y;
     
        int k_1 = int_indoor_y | (int_indoor_x & (quadrant.x ^ int_indoor_x));
        int sgn = quadrant.x * 2 - 1;
        int dx = k_1 * sgn;

        k_1 = int_indoor_y | (int_indoor_x & quadrant.y);
        int dy = -k_1;

        target = center + HALF_TILE_SIZE * vec2(dx, dy);
        circle = indoor_x == indoor_y;

        if (circle) {
            float d = abs(length(worldPos - target));
            return HALF_TILE_SIZE * invert + d * (1 - 2 * invert);
        } else {
            vec2 target_quadrant_top_left = target;
            vec2 target_quadrant_bottom_right = target_quadrant_top_left + vec2(HALF_TILE_SIZE);

            return abs(distanceToRectangle(worldPos, target_quadrant_top_left, target_quadrant_bottom_right));
        }
    }

    bool vertical = (!indoor_x) || (indoor_x && (quadrant.y == 1));
    if (vertical) {
        int k_1 = int_indoor_x | (int_indoor_y & quadrant.x);
        int sgn = (quadrant.x & int_indoor_x) * 2 - 1;
        int dx = k_1 * sgn;
        
        k_1 = int_indoor_y & (int_indoor_x | (quadrant.y ^ int_indoor_y));
        sgn = int_indoor_x * 2 - 1;
        int dy = k_1 * sgn;

        int invert = int_indoor_x;


        target = center + HALF_TILE_SIZE * vec2(dx, dy);
        circle = indoor_x == indoor_y;

        if (circle) {
            // if (is_indoor(k_x) && is_indoor(k_y)) {
            //     return 100;
            // }
            float d = abs(length(worldPos - target));
            return HALF_TILE_SIZE * invert + d * (1 - 2 * invert);
        } else {
            vec2 target_quadrant_top_left = target;
            vec2 target_quadrant_bottom_right = target_quadrant_top_left + vec2(HALF_TILE_SIZE);

            return abs(distanceToRectangle(worldPos, target_quadrant_top_left, target_quadrant_bottom_right));
        }
    }

    // Method using an if statement - Faster at least in extreme test cases
    if (indoor_x && indoor_y) {
        // COR1
        return 100;

        if (horizontal) {
            return 100;
            // dx = indoor_x * (1 - s_x) * (1 - invert * s_x * 2)
            int invert = int(indoor_y);
            int dx = int(indoor_x) * (1 - s.x) * (1 - invert * s.x * 2);
            int dy = int(indoor_y) * (1 - s.x * 2) - s.y;
            target = center + HALF_TILE_SIZE * vec2(dx, dy);
            circle = !(indoor_x && !indoor_y);
        } else {
            circle = true;
            target = center + HALF_TILE_SIZE * s;
        }

        if (circle) {
            return HALF_TILE_SIZE - abs(length(worldPos - target));
        } else {
            vec2 target_quadrant_top_left = target;
            vec2 target_quadrant_bottom_right = target_quadrant_top_left + vec2(HALF_TILE_SIZE);

            return abs(distanceToRectangle(worldPos, target_quadrant_top_left, target_quadrant_bottom_right));
        }
    } else {
        // COR2
        return 50;
        // This must be ivec2, doesn't work with vec2 for some reason
        // Negated because we want to move in a direction opposite to the quadrant sign (s.x, s.y)
        ivec2 delta = -HALF_TILE_SIZE * s * ivec2(empty_x, empty_y);

        vec2 target_quadrant_top_left = our_quadrant_top_left + delta;
        vec2 target_quadrant_bottom_right = target_quadrant_top_left + vec2(HALF_TILE_SIZE);

        return abs(distanceToRectangle(worldPos, target_quadrant_top_left, target_quadrant_bottom_right));
    }
}

ivec4 get_z(uint k_x, uint k_y, uint k_diagonal, uint k_0) {
    uint z_x = k_x >> 27;
    uint z_y = k_y >> 27;
    uint z_diagonal = k_diagonal >> 27;

    uint z_0 = k_0 >> 27;

    return ivec4(z_x, z_y, z_diagonal, z_0);
}

uint getOcclusionForDebug(vec2 worldPos) {
    const int width = occlusionData.width;
    const int height = occlusionData.height;
    
    worldPos += 0.5f;

    
    ivec2 gamePos = ivec2(worldPos / TILE_SIZE_F);
    int z = int(pc.position.z);

    // Compensate for the fact that each time we move up in z, x and y shifts by (1, 1)
    int perspective_correction = occlusionData.z0 - z;
    
    int x0 = occlusionData.bufferIndexOffsetX + gamePos.x + perspective_correction;
    int y0 = occlusionData.bufferIndexOffsetY + gamePos.y + perspective_correction;

    ivec3 pos_0 = ivec3(x0 % width, y0 % height, z);
    uint k_0 = get_occlusion_v2(width, height, pos_0);

    return k_0;
}

vec4 computeIndoorShadow(vec2 worldPos) {
    // Width of the viewport (in game space, 1 = 1 tile)

    if ((pc.flags & CONDITION_FLAG_FORCE_FULL_SHADOW) != 0) {
        return COLOR_FULL_INDOOR_SHADOW;
    }

    const int width = occlusionData.width;
    const int height = occlusionData.height;
    
    worldPos += 0.5f;

    
    ivec2 gamePos = ivec2(worldPos / TILE_SIZE_F);
    int z = int(pc.position.z);

    // Compensate for the fact that each time we move up in z, x and y shifts by (1, 1)
    int perspective_correction = occlusionData.z0 - z;
    
    int x0 = occlusionData.bufferIndexOffsetX + gamePos.x + perspective_correction;
    int y0 = occlusionData.bufferIndexOffsetY + gamePos.y + perspective_correction;

    // Draws a tile grid
    // if ((int(worldPos.x) & 31) == 0 || (int(worldPos.y) & 31) == 0) {
    //     return vec4(vec3(1.0f), 0.25f);
    // }

    ivec3 pos_0 = ivec3(x0 % width, y0 % height, z);
    uint k_0 = get_occlusion_v2(width, height, pos_0);

    // if (is_indoor_border(k_0)) {
    //     return RED;
    // } else if (is_fully_indoor(k_0)) {
    //     return GREEN;
    // }
    
    // if (is_indoor(k_0)) {
    //     if (is_occluded(k_0)) {
    //         return BLUE;
    //     }

    //     return GREEN;
    // }

    // if (is_occluded(k_0)) {
    //     return BLUE;
    // }

    // if ((k_0 & 0xFF) == 0) {
    //     return vec4(0.0f, 0.0f, 1.0f, 1.0f);
    // }

    // if (is_indoor(k_0)) {
    //     return vec4(0.0f, 1.0f, 0.0f, 1.0f);
    // }

    if (!is_indoor(k_0) || is_occluded(k_0)) {
        return COLOR_NO_INDOOR_SHADOW;
    }


    ivec2 center = ivec2(floor(worldPos / TILE_SIZE_F) * TILE_SIZE_F + HALF_TILE_SIZE);
    ivec2 s = ivec2(sign(worldPos - center));


    ivec3 pos_kx = ivec3((x0 + s.x) % width, y0 % height, z);
    ivec3 pos_ky = ivec3(x0 % width, (y0 + s.y) % height, z);
    ivec3 pos_kdiag = ivec3((x0 + s.x) % width, (y0 + s.y) % height, z);

    uint k_x = get_occlusion_v2(width, height, pos_kx); // Horizontal occlusion
    uint k_y = get_occlusion_v2(width, height, pos_ky); // Vertical occlusion
    uint k_diagonal = get_occlusion_v2(width, height, pos_kdiag); // Diagonal occlusion


    // Treat indoor wall as indoor if we are an indoor wall
    if ((pc.flags & CONDITION_FLAG_IS_INDOOR_WALL) != 0 && pc.position.z == 7) {
        k_x |= INDOOR * int(is_occluded_or_indoor_wall(k_x));
        k_y |= INDOOR * int(is_occluded_or_indoor_wall(k_y));
        k_diagonal |= INDOOR * int(is_occluded_or_indoor_wall(k_diagonal));
    }


    // Conditional logic for indoor shadows
    int clear_north = int(s.y == -1 && (pc.flags & CONDITION_FLAG_CLEAR_SHADOW_NORTH) == CONDITION_FLAG_CLEAR_SHADOW_NORTH);
    int clear_west = int(s.x == -1 && (pc.flags & CONDITION_FLAG_INDOOR_SHADOW_CLEAR_SHADOW_WEST) == CONDITION_FLAG_INDOOR_SHADOW_CLEAR_SHADOW_WEST);
    int clear_east = int(s.x == 1 && (pc.flags & CONDITION_FLAG_INDOOR_SHADOW_CLEAR_SHADOW_EAST) == CONDITION_FLAG_INDOOR_SHADOW_CLEAR_SHADOW_EAST);
    int clear_south = int(s.y == 1 && (pc.flags & CONDITION_FLAG_INDOOR_SHADOW_CLEAR_SHADOW_SOUTH) == CONDITION_FLAG_INDOOR_SHADOW_CLEAR_SHADOW_SOUTH);
    int clear_north_east = int(s.x == 1 && s.y == -1 && (pc.flags & CONDITION_FLAG_INDOOR_SHADOW_CLEAR_SHADOW_NORTH_EAST) == CONDITION_FLAG_INDOOR_SHADOW_CLEAR_SHADOW_NORTH_EAST);

    k_y &= ~((INDOOR | INDOOR_BORDER) * clear_north);
    k_y &= ~((INDOOR | INDOOR_BORDER) * clear_south);
    k_x &= ~((INDOOR | INDOOR_BORDER) * clear_west);
    k_x &= ~((INDOOR | INDOOR_BORDER) * clear_east);
    k_diagonal &= ~((INDOOR|INDOOR_BORDER) * clear_north_east);

    // // Horizontal occlusion
    // ivec2 pos_kx = ivec2((x0 + s.x) % width, y0 % height);
    // uint k_x = get_occlusion(pos_kx.y * width + pos_kx.x);

    // // Vertical occlusion
    // ivec2 pos_ky = ivec2(x0 % width, (y0 + s.y) % height);
    // uint k_y = get_occlusion(pos_ky.y * width + pos_ky.x);


    // // Diagonal occlusion
    // ivec2 pos_kdiag = ivec2((x0 + s.x) % width, (y0 + s.y) % height);
    // uint k_diagonal = get_occlusion(pos_kdiag.y * width + pos_kdiag.x);

    // ivec4 z = get_z(k_x, k_y, k_diagonal, k_0);


    // return vec4(int(is_indoor(k_x)), int(is_indoor(k_y)), int(is_indoor(k_diagonal)), 1.0f);

    

    // k_y & 11 ==> not occluded nor indoor
    if (is_occluded_and_indoor(k_0) && is_occluded_and_indoor(k_x) && ((k_y & 11) == 0)) {
        return COLOR_NO_INDOOR_SHADOW;
    }

    // if (s.x == -1 && is_occluded_and_indoor(k_x)) {
    //     if (non_occluded_indoor(k_y)) {
    //         return COLOR_FULL_INDOOR_SHADOW;
    //     } else {
    
    //     }
    // }
    // else if (s.y == -1 && is_occluded_and_indoor(k_y)) {
    //     return COLOR_FULL_INDOOR_SHADOW;
    // }
    // else if (s.x == 1 && s.y == -1 && is_occluded_and_indoor(k_y)) {
    //     return COLOR_FULL_INDOOR_SHADOW;
    // }
    // else if (s.x == -1 && s.y == -1 && is_occluded_and_indoor(k_diagonal)) {
    //     return COLOR_FULL_INDOOR_SHADOW;
    // }
    // else if (s.y == -1 && is_occluded(k_y) && is_occluded_or_indoor(k_x)) {
    //     return COLOR_FULL_INDOOR_SHADOW;
    // }
    // else if (s.x == -1 && s.y == 1 && is_indoor(k_y) && is_occluded(k_x)) {
    //     return COLOR_FULL_INDOOR_SHADOW;
    // }

    // Always full indoor shadow if we are surrounded by indoor or occluded tiles
    bool x_is_covered = is_occluded_or_indoor(k_x);
    bool y_is_covered = is_occluded_or_indoor(k_y);
    bool diagonal_is_covered = is_occluded_or_indoor(k_diagonal);

    if (x_is_covered && y_is_covered && diagonal_is_covered) {
        return COLOR_FULL_INDOOR_SHADOW;
    }
    
    if (is_occluded(k_0 & k_x & k_y & k_diagonal)) {
        return COLOR_NO_INDOOR_SHADOW;
    }
    else if (!is_occluded(k_0) && non_occluded_indoor(k_x) && non_occluded_indoor(k_y) && non_occluded_indoor(k_diagonal)) {
        return COLOR_FULL_INDOOR_SHADOW;
    }

    vec2 our_quadrant_top_left = floor(worldPos / HALF_TILE_SIZE) * HALF_TILE_SIZE;

    vec2 tile_top_left = floor(worldPos / TILE_SIZE) * TILE_SIZE;
    float dist_to_target = computeDistanceToTargetForIndoor(worldPos, center, our_quadrant_top_left, tile_top_left, s, k_0, k_x, k_y, k_diagonal);

    if (dist_to_target == 100) {
        return GREEN;
    }
    if (dist_to_target == 50) {
        return BLUE;
    }

    float f_0 = clamp(min(dist_to_target / HALF_TILE_SIZE, 1), 0, 1);
    // f_0: [0, 1]

    // Rescale f_0 to [INDOOR_SHADOW_STRENGTH, 1]
    f_0 = (f_0 * (1 - INDOOR_SHADOW_STRENGTH)) + INDOOR_SHADOW_STRENGTH;
    // float i = 1 - clamp(interpolation_value, 0, 1);
    // i = (i * (1 - INDOOR_SHADOW_STRENGTH)) + INDOOR_SHADOW_STRENGTH;

    // 0 ==> Full shadow
    // 1 ==> No shadow
    
    vec4 indoorColor = vec4(vec3(f_0), 1.0f);

    return indoorColor;
}

// Returns the position in Vulkan clip space
vec2 getClipSpacePos(int screenWidth, int screenHeight) {
    // gl_FragCoord is in [0, screenWidth] x [0, screenHeight]
    return gl_FragCoord.xy / vec2(screenWidth, screenHeight) * 2 - 1;
}


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// >>>>>>>>END INDOOR SHADOW SYSTEM>>>>>>>>>
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

// NOTE: This was taken from this thread: https://community.khronos.org/t/custom-bilinear-filtering-w-texturegather-problem/76178
//
// The function "textureBilinear" has a bug where it creates pixel line strips due to sampling from a neighbouring sprite.
// The function "textureBilinearV2" fixes this issue for default zoom (zoom = 1.0). However, it has not been tested for
// other zoom levels.
vec4 textureBilinearV2(in sampler2D tex, in vec2 coord) {
    vec2 colorTextureSize = vec2(textureSize(tex, 0));

     // Convert UV coordinates to pixel coordinates and get pixel index of top left pixel (assuming UVs are relative to top left corner of texture)
    vec2 pixCoord = coord * colorTextureSize - 0.5f;    // First pixel goes from -0.5 to +0.4999 (0.0 is center) last pixel goes from (size - 1.5) to (size - 0.5000001)
    vec2 originPixCoord = floor(pixCoord);              // Pixel index coordinates of bottom left pixel of set of 4 we will be blending

    // For Gather we want UV coordinates of bottom right corner of top left pixel
    vec2 gatherUV = (originPixCoord + 1.0f) / colorTextureSize;

    // Gather from all surounding texels:
    vec4 red   = textureGather(tex, gatherUV, 0);
    vec4 green = textureGather(tex, gatherUV, 1);
    vec4 blue  = textureGather(tex, gatherUV, 2);
    vec4 alpha = textureGather(tex, gatherUV, 3);
 
    // Swizzle the gathered components to create four colours
    vec4 c00 = vec4(red.w, green.w, blue.w, alpha.w);
    vec4 c01 = vec4(red.x, green.x, blue.x, alpha.x);
    vec4 c11 = vec4(red.y, green.y, blue.y, alpha.y);
    vec4 c10 = vec4(red.z, green.z, blue.z, alpha.z);

    // Filter weight is fract(coord * colorTextureSize - 0.5f) = (coord * colorTextureSize - 0.5f) - floor(coord * colorTextureSize - 0.5f)
    vec2 filterWeight = pixCoord - originPixCoord;
 
    // Bi-linear mixing:
    vec4 temp0 = mix(c01, c11, filterWeight.x);
    vec4 temp1 = mix(c00, c10, filterWeight.x);
    return mix(temp1, temp0, filterWeight.y);
}

vec4 textureBilinear(in sampler2D texSampler, in vec2 textureCoordinate)
{
    // Get texture size in pixels:
    vec2 colorTextureSize = vec2(textureSize(texSampler, 0));

    // Convert UV coordinates to pixel coordinates and get pixel index of top left
    // pixel (assuming UVs are relative to top left corner of texture)
    vec2 firstPixelCoordinate =
        textureCoordinate * colorTextureSize -
        0.5f; // First pixel goes from -0.5 to +0.4999 (0.0 is center) last pixel
              // goes from (size - 1.5) to (size - 0.5000001)
    vec2 originPixelCoordinate =
        floor(firstPixelCoordinate); // Pixel index coordinates of bottom left
                                     // pixel of set of 4 we will be blending

    vec2 sampleUV = (originPixelCoordinate + 0.5f) / colorTextureSize;

    // Sample from all surounding texels
    vec4 c00 = texture(texSampler, sampleUV);
    vec4 c01 = textureOffset(texSampler, sampleUV, ivec2(0, 1));
    vec4 c11 = textureOffset(texSampler, sampleUV, ivec2(1, 1));
    vec4 c10 = textureOffset(texSampler, sampleUV, ivec2(1, 0));

    // vec3 black = vec3(0.0f);
    // vec3 magenta = vec3(1.0f, 0.0f, 1.0f);
    // vec4 transparent = vec4(0.0f);

    // if (c00.rgb == magenta)
    //     c00.rgb = black;
    // if (c01.rgb == magenta)
    //     c01.rgb = black;
    // if (c11.rgb == magenta)
    //     c11.rgb = black;
    // if (c10.rgb == magenta)
    //     c10.rgb = black;

    // if (c00.rgb == magenta)
    //     c00 = transparent;
    // if (c01.rgb == magenta)
    //     c01 = transparent;
    // if (c11.rgb == magenta)
    //     c11 = transparent;
    // if (c10.rgb == magenta)
    //     c10 = transparent;

    // Filter weight is fract(coord * colorTextureSize - 0.5f) = (coord *
    // colorTextureSize - 0.5f) - floor(coord * colorTextureSize - 0.5f)
    vec2 filterWeight = firstPixelCoordinate - originPixelCoordinate;

    // Bi-linear mixing:
    vec4 temp0 = mix(c01, c11, filterWeight.x);
    vec4 temp1 = mix(c00, c10, filterWeight.x);
    return mix(temp1, temp0, filterWeight.y);
}

void main() {
    // TODO Check whether this clamp is necessary
    vec2 sampleLocation =
        vec2(clamp(fragTexCoord.x, fragTexBoundary.x, fragTexBoundary.z),
             clamp(fragTexCoord.y, fragTexBoundary.y, fragTexBoundary.w));
    // vec2 sampleLocation = fragTexCoord;

    // outColor = textureBilinear(texSampler, sampleLocation) * fragColor;
    outColor = textureBilinearV2(texSampler, sampleLocation) * fragColor;
    // outColor = texture(texSampler, sampleLocation) * fragColor;

    if (outColor.a > 0.01f) {
        if ((pc.flags & CONDITION_FLAG_IS_OUTDOOR_WALL) == CONDITION_FLAG_IS_OUTDOOR_WALL) {
            outMapLightMask = vec4(1.0f);
        }
        else {
            outMapLightMask = vec4(vec3(0.0f), 1.0f);
        }

        if (writeToLightMask > 0.1f) {
            vec2 vulkanClipSpacePos = getClipSpacePos(occlusionData.bufferWidth, occlusionData.bufferHeight);
            ivec2 worldPos = ivec2((ubo.inverseProjectionMatrix * vec4(vulkanClipSpacePos, 0.0, 1.0)).xy);

            vec4 shadow = computeIndoorShadow(vec2(worldPos));
            outIndoorShadow = shadow;
        } else {
            outIndoorShadow = vec4(1.0f);
        }
    } else {
        outIndoorShadow = vec4(0.0f);
        outMapLightMask = vec4(0.0f);
    }
}
