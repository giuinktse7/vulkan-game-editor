#pragma once

#include <array>
#include <vulkan/vulkan.h>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include <memory>
#include <vector>

#include <unordered_map>
#include <unordered_set>
#include <vulkan/vulkan_core.h>

#include "brushes/brush.h"
#include "editor_action.h"
#include "graphics/buffer.h"
#include "graphics/texture.h"
#include "graphics/texture_atlas.h"
#include "graphics/vulkan_helpers.h"
#include "item.h"
#include "items.h"
#include "map.h"
#include "render/quad_mesh.h"
#include "util.h"

class MapView;

namespace colors
{
    constexpr glm::vec4 Default{1.0f, 1.0f, 1.0f, 1.0f};
    constexpr glm::vec4 Selected{0.45f, 0.45f, 0.45f, 1.0f};
    constexpr glm::vec4 Shade = Selected;
    constexpr glm::vec4 Red{1.0f, 0.0f, 0.0f, 1.0f};
    constexpr glm::vec4 SeeThrough{1.0f, 1.0f, 1.0f, 0.35f};
    constexpr glm::vec4 ItemPreview{0.6f, 0.6f, 0.6f, 0.7f};

    glm::vec4 opacity(float value);

} // namespace colors

struct RectangleDrawInfo
{
    RectangleDrawInfo(SolidColor color, SolidColor borderColor, WorldPosition position, int width, int height, float opacity = 1.0f);

    static RectangleDrawInfo border(SolidColor color, WorldPosition position, int width, int height, float opacity = 1.0f);
    static RectangleDrawInfo border(SolidColor color, WorldPosition position, int size, float opacity = 1.0f);

    static RectangleDrawInfo solid(SolidColor color, WorldPosition position, int width, int height, float opacity = 1.0f);
    static RectangleDrawInfo solid(SolidColor color, WorldPosition position, int size, float opacity = 1.0f);

    std::optional<SolidColor> color;
    std::optional<SolidColor> borderColor;
    WorldPosition position = {0, 0};

    // Width in pixels
    int width = 0;

    // Height in pixels
    int height = 0;

    float opacity;

  private:
    RectangleDrawInfo();
};

namespace DrawInfo
{
    struct Base
    {
        TextureInfo textureInfo;
        glm::vec4 color = colors::Default;
        VkDescriptorSet descriptorSet;
        uint32_t width;
        uint32_t height;
    };

    /**
     * Describes an overlay object.
     * NOTE: Do not use this to draw a map item. For that, use DrawInfo::Object;
     * it takes a Position instead of a WorldPosition.
     */
    struct OverlayObject : Base
    {
        WorldPosition position;
    };

    struct Object : Base
    {
        Position position;
        DrawOffset drawOffset = {0, 0};
    };

    struct ObjectQuadrant : Base
    {
        Position position;
        DrawOffset drawOffset = {0, 0};
    };

    struct Creature : Base
    {
        Position position;
        DrawOffset drawOffset = {0, 0};
    };

    struct Vertex
    {
        TextureInfo textureInfo;
        WorldPosition position;
        glm::vec4 color{};
        VkDescriptorSet descriptorSet;
    };

    struct Rectangle
    {
        WorldPosition from;
        WorldPosition to;
        glm::vec4 color{};
        std::variant<const Texture *, TextureInfo> texture;
        VkDescriptorSet descriptorSet;
    };

}; // namespace DrawInfo

namespace ItemDrawFlags
{
    constexpr uint32_t None = 0;
    constexpr uint32_t DrawNonSelected = 1 << 0;
    constexpr uint32_t DrawSelected = 1 << 1;
    constexpr uint32_t Ghost = 1 << 2;
    constexpr uint32_t ActiveSelectionArea = 1 << 3;
    constexpr uint32_t Shade = 1 << 4;
    constexpr uint32_t ForceDraw = 1 << 5;
} // namespace ItemDrawFlags

struct TextureOffset
{
    float x;
    float y;
};

struct ItemUniformBufferObject
{
    glm::mat4 projection;
};

struct ItemDrawInfo
{
    const Item *item = nullptr;
    Position position;
    uint32_t drawFlags = 0;
    DrawOffset worldPosOffset = {0, 0};
};

struct ItemTypeDrawInfo
{
    uint32_t spriteId;
    glm::vec4 color = colors::Default;
    const ItemType *itemType = nullptr;
    WorldPosition worldPos;
    DrawOffset worldPosOffset = {0, 0};
};

enum BlendMode
{
    BM_NONE,
    BM_BLEND,
    BM_ADD,
    BM_ADDX2,

    NUM_BLENDMODES
};

enum class FrameDataFlag : uint16_t
{
    None = 0,
    MouseHover = 1,
    Dragging = 1 << 1,
    DraggingWithSubtract = 1 << 2,
    MovingSelection = 1 << 3,
};
VME_ENUM_OPERATORS(FrameDataFlag);

struct FrameData
{
    int index = 0;

    VkFramebuffer frameBuffer = VK_NULL_HANDLE;
    BoundBuffer uniformBuffer;
    VkDescriptorSet uboDescriptorSet = VK_NULL_HANDLE;

    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

    glm::mat4 projectionMatrix{};

    MouseAction_t mouseAction = MouseAction::None{};
    Position mouseGamePos = PositionConstants::Zero;
    FrameDataFlag flags = FrameDataFlag::None;
    std::optional<std::pair<WorldPosition, WorldPosition>> dragPoints = std::nullopt;
};

/*
        A VulkanTexture contains Vulkan resources that are used to draw a Texture
        to the screen.
*/
class VulkanTexture
{
  public:
    struct Descriptor
    {
        VkDescriptorSetLayout layout;
        VkDescriptorPool pool;
    };

    VulkanTexture();
    ~VulkanTexture();

    VulkanTexture(const VulkanTexture &other) = delete;
    VulkanTexture &operator=(const VulkanTexture &other) = delete;

    VulkanTexture(VulkanTexture &&other) noexcept = default;
    VulkanTexture &operator=(VulkanTexture &&other) noexcept = default;

    bool unused = true;

    void initResources(const Texture &texture, std::shared_ptr<VulkanInfo> &vulkanInfo, const VulkanTexture::Descriptor descriptor);
    void releaseResources();

    [[nodiscard]] bool hasResources() const
    {
        return textureImage != VK_NULL_HANDLE || textureImageMemory != VK_NULL_HANDLE || _descriptorSet != VK_NULL_HANDLE;
    }

    [[nodiscard]] VkDescriptorSet descriptorSet() const
    {
        return _descriptorSet;
    }

  private:
    uint32_t width;
    uint32_t height;

    std::shared_ptr<VulkanInfo> vulkanInfo;
    VkImage textureImage = VK_NULL_HANDLE;
    VkDeviceMemory textureImageMemory = VK_NULL_HANDLE;
    VkDescriptorSet _descriptorSet = VK_NULL_HANDLE;

    VkDescriptorSet createDescriptorSet(VulkanTexture::Descriptor descriptor);
    void copyStagingBufferToImage(VkBuffer stagingBuffer);
    VkImageView createImageView(VkImage image, VkFormat format);
    void createImage(VkFormat format,
                     VkImageTiling tiling,
                     VkImageUsageFlags usage,
                     VkMemoryPropertyFlags properties);
    VkSampler createSampler();
};

class MapRenderer
{
  public:
    MapRenderer(std::shared_ptr<VulkanInfo> &vulkanInfo, std::shared_ptr<MapView> &mapView);
    ~MapRenderer();

    static constexpr int MAP_ATTACHMENT_INDEX = 0;
    static constexpr int LIGHT_MASK_ATTACHMENT_INDEX = 1;
    static constexpr int INDOOR_SHADOW_ATTACHMENT_INDEX = 2;

    // 0: Map attachment
    // 1: Light mask attachment
    // 2: Indoor shadow attachment
    static constexpr int RENDER_ATTACHMENT_COUNT = 3;

    static const int TILE_SIZE = 32;
    static const uint32_t MAX_VERTICES = 64 * 1024;

    void createResources();
    void destroyResources();

    void setRenderTargetSize(int width, int height);

    void render(VkCommandBuffer commandBuffer, int frameIndex);

    VkDescriptorSetLayout &getTextureDescriptorSetLayout()
    {
        return textureDescriptorSetLayout;
    }

    inline void setCurrentFrame(int frameIndex)
    {
        _currentFrame = &frames[frameIndex];
    }

    FrameData *currentFrame() const noexcept
    {
        return _currentFrame;
    }

    VkRenderPass getRenderPass() const
    {
        return renderPass;
    }

    std::shared_ptr<VulkanInfo> getVulkanInfo() const
    {
        return vulkanInfo;
    }

    bool containsAnimation() const
    {
        return _containsAnimation;
    }

  private:
    using ItemPredicate = std::function<bool(const Position, const Item &item)>;

    struct TextureWindowEqual
    {
        size_t operator()(const TextureWindow &l, const TextureWindow &r) const
        {
            return l.x0 == r.x0 && l.y0 == r.y0 && l.x1 == r.x1 && l.y1 == r.y1;
        };
    };
    struct TextureWindowHasher
    {
        size_t operator()(const TextureWindow &window) const
        {
            size_t hash = 0;
            util::combineHash(hash, window.x0);
            util::combineHash(hash, window.y0);
            util::combineHash(hash, window.x1);
            util::combineHash(hash, window.y1);
            return hash;
        }
    };

    void createRenderPass();
    void createGraphicsPipeline();
    VkShaderModule createShaderModule(const std::vector<uint8_t> &code);
    void createUniformBuffers();
    void createDescriptorSetLayouts();
    void createDescriptorSets();
    void createIndexBuffer();
    void createVertexBuffer();

    static auto createColorBlendAttachmentStates() -> std::array<VkPipelineColorBlendAttachmentState, RENDER_ATTACHMENT_COUNT>;

    bool insideMap(const Position &position);

    void updateUniformBuffer(glm::mat4 projection);

    void setupFrame(VkCommandBuffer cb);

    VkCommandBuffer beginSingleTimeCommands(VulkanInfo *info);
    uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

    void drawMap();
    void drawCurrentAction();
    void drawPreviewItem(uint32_t serverId, Position pos);
    void drawMovingSelection();
    void drawMapOverlay();

    void drawRectangle(const RectangleDrawInfo &info);

    void drawRectangle(const Texture &texture, WorldPosition from, WorldPosition to, float opacity = 1.0f);
    void drawRectangle(const Texture &texture, WorldPosition from, int width, int height, float opacity = 1.0f);

    DrawInfo::Object getItemDrawInfo(const Item &item, const Position &position, uint32_t drawFlags);
    DrawInfo::Object itemTypeDrawInfo(const ItemType &itemType, const Position &position, uint32_t drawFlags);
    DrawInfo::Creature creatureDrawInfo(const Creature &creature, const Position &position, uint32_t drawFlags);

    glm::vec4 getItemDrawColor(const Item &item, const Position &position, uint32_t drawFlags);
    glm::vec4 getCreatureDrawColor(const Creature &creature, const Position &position, uint32_t drawFlags) const;
    glm::vec4 getItemTypeDrawColor(uint32_t drawFlags);

    VkDescriptorSet objectDescriptorSet(const Texture &texture);
    VkDescriptorSet objectDescriptorSet(TextureAtlas *atlas);

    /**
     * @predicate An Item predicate. Items for which predicate(item) is false will not be rendered.
     */
    void drawTile(const TileLocation &tileLocation,
                  uint32_t drawFlags,
                  Position offset,
                  const ItemPredicate &filter = nullptr);
    void drawTile(const TileLocation &tileLocation,
                  uint32_t drawFlags = ItemDrawFlags::DrawNonSelected,
                  const ItemPredicate &filter = nullptr);
    void drawTile(Tile *tile, uint32_t flags, Position offset, const ItemPredicate &filter = nullptr);

    WorldPosition getWorldPosForDraw(const ItemTypeDrawInfo &info, TextureAtlas *atlas) const;

    void drawItem(const ItemDrawInfo &drawInfo);
    void drawItemType(const ItemTypeDrawInfo &drawInfo, QuadrantRenderType renderType);
    void drawItemType(const ItemTypeDrawInfo &drawInfo);

    void drawOverlayItemType(uint32_t serverId, WorldPosition position, glm::vec4 color = colors::Default);

    void drawCreature(const DrawInfo::Creature &info);
    void drawCreatureType(const CreatureType &creatureType, Position position, Direction direction, glm::vec4 color, const DrawOffset &drawOffset = DrawOffset{0, 0});

    bool shouldDrawItem(Position pos, const Item &item, uint32_t flags, const ItemPredicate &filter = {}) const noexcept;
    bool shouldDrawCreature(Position pos, const Creature &creature, uint32_t flags) const noexcept;

    void drawBrushPreview(Brush *brush, const Position &position, int variation);
    void drawBrushPreviewAtWorldPos(Brush *brush, const WorldPosition &worldPos, int variation);
    void drawPreview(ThingDrawInfo drawInfo, const Position &position);

    void issueDraw(const DrawInfo::Base &info, const WorldPosition &worldPos);
    void issueRectangleDraw(DrawInfo::Rectangle &info);

    MouseAction_t mouseAction() const;

    template <typename T>
    bool isMouseAction()
    {
        return std::holds_alternative<T>(_currentFrame->mouseAction);
    }

    template <typename T>
    T *mouseActionAs()
    {
        return std::get_if<T>(&_currentFrame->mouseAction);
    }

    bool hasMovingSelection();
    bool mouseHover();
    bool isDragging();
    bool draggingWithSubtract();
    std::optional<std::pair<WorldPosition, WorldPosition>> getDragPoints();

    std::unordered_set<TextureWindow, TextureWindowHasher, TextureWindowEqual> testTextureSet;

    // std::unique_ptr<SwapChain> swapchain;

    bool _containsAnimation = false;

    bool debug = false;
    std::shared_ptr<MapView> mapView;
    std::shared_ptr<VulkanInfo> vulkanInfo;
    std::array<FrameData, 3> frames;

    // All sprites are drawn using this index buffer

    BoundBuffer indexBuffer;
    BoundBuffer vertexBuffer;

    QuadMesh quadMesh;

    FrameData *_currentFrame = nullptr;

    VkRenderPass renderPass = VK_NULL_HANDLE;

    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline = 0;

    VkDescriptorSetLayout uboDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout textureDescriptorSetLayout = VK_NULL_HANDLE;

    // Vulkan texture resources
    mutable std::vector<uint32_t> activeTextureAtlasIds;
    mutable std::vector<VulkanTexture> vulkanTexturesForAppearances;
    /*
                Resources for general textures such as solid color textures (non-sprites)

                NOTE: Textures from TextureAtlas class should be stored in 'vulkanTexturesForAppearances'.
        */
    std::unordered_map<const Texture *, VulkanTexture> vulkanTextures;

    util::Size renderTargetSize;

    bool isDefaultZoom = true;
};
