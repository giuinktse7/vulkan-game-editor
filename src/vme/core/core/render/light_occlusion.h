#pragma once

#include "../camera.h"
#include "../graphics/buffer.h"
#include "../position.h"
#include "light_constants.h"
#include <array>

class OcclusionData
{
  private:
    static const uint32_t SIZE = MAX_SUPPORTED_NUM_TILES_FOR_LIGHT_RENDERING * MAX_SUPPORTED_NUM_FLOORS_FOR_LIGHT;
    static const uint32_t EDGE_PADDING = 1;

    static const uint32_t OCCLUDED = 1 << 0;
    static const uint32_t INDOOR = 1 << 1;
    static const uint32_t GROUND = 1 << 2;
    static const uint32_t INDOOR_BORDER = 1 << 3;

    // Whether the tile blocks light for walls
    // This is used e.g. in cases where a vertical wall is north of a vertical archway whose ground is outdoors (but one tile east of the archway is indoors)
    // In this case, the wall should consider the archway as blocking light, but the ground should not
    static const uint32_t INDOOR_WALL = 1 << 4;

    /**
     * Whether the tile contains elevated ground (e.g. a mountain)
     */
    static const uint32_t ELEVATED_GROUND = 1 << 5;

    /* Flags to help with shadow rendering logic. These flags are not used in the shader code.
       NOTE: Flags must NOT exceed 32 - 6 = 26, because the last 6 bits are used for floor information.
       Flags start at 20 and "grow" down (decreasing) to leave possibility for other types of usage for higher and lower bits.
    */
    static const uint32_t FLAG_WALL_IGNORES_NORTH_INDOOR_SHADOW = 1 << 20;

    struct OcclusionBufferObject
    {
        uint32_t width;
        // We must include height here (or some other 4-byte value) even though we don't use it to get the correct
        // alignment of the occlusion buffer
        uint32_t height;
        // Framebuffer width in pixels
        uint32_t bufferWidth;
        // Framebuffer height in pixels
        uint32_t bufferHeight;

        uint32_t bufferIndexOffsetX;
        uint32_t bufferIndexOffsetY;

        // Z used to compute the buffer index offsets
        uint32_t z0;

        // Required to get correct alignment for the occlusion buffer
        uint32_t __padding1;

        // Used to occlude lights
        /**
         * Last 6 bits are for floor (2^6 - 1 = 63 possible floors)
         */
        std::array<uint32_t, SIZE> occlusion;
    } occlusionUbo;

  public:
    OcclusionData();
    void reset();
    void setOcclusionBelow(Position pos);
    void setIndoorWallBelow(Position pos);

    void setIndoor(Position pos);
    void setIndoorBorder(Position pos);
    void setIndoorWall(Position pos);
    void setElevatedGround(Position pos);

    void clearIndoor(Position pos);

    bool isFullyIndoor(Position pos) const;
    bool isIndoor(Position pos, bool includeBorder) const;
    bool isIndoorBorder(Position pos) const;
    bool isIndoorWall(Position pos) const;
    bool isNonOccludedIndoor(Position pos) const;
    bool isInsideBounds(Position pos) const;

    void setGround(Position pos, bool occludeBelow = true);

    void setViewport(Camera::Viewport viewport);

    OcclusionBufferObject *data()
    {
        return &occlusionUbo;
    }

    [[nodiscard]] static constexpr uint32_t size()
    {
        return sizeof(OcclusionBufferObject);
    }

    BoundBuffer buffer;

  private:
    friend class LightRenderer;
    Camera::Viewport viewport;

    // Top-left and bottom-right game position that can use the occlusion buffer
    // These positions MUST be honored. If they are not, the occlusion buffer will be incorrect due to index wrapping.
    Position topLeftGamePos;
    Position bottomRightGamePos;

    [[nodiscard]] int getOcclusionIndex(const Position &pos) const noexcept;
    [[nodiscard]] int getIndoorShadowOcclusionIndex(Position pos) const noexcept;
};