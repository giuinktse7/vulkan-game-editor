#include "map_renderer.h"

#include <glm/vec2.hpp>
#include <stdexcept>
#include <variant>

#include "../vendor/rollbear-visit/visit.hpp"
#include "brushes/brush.h"
#include "brushes/ground_brush.h"
#include "brushes/raw_brush.h"
#include "brushes/wall_brush.h"
#include "debug.h"
#include "file.h"
#include "graphics/appearances.h"
#include "logger.h"
#include "map_view.h"
#include "position.h"
#include "settings.h"
#include "util.h"

/** Order of members matter for this struct due to alignment requirements in the 
 * vertex shader.
 * 
 * See: Vulkan Spec: 14.5.4. Offset and Stride Assignment
 */
struct PushConstantData
{
    glm::vec4 textureQuad;
    glm::vec4 fragQuad;
    glm::vec4 color;
    glm::vec4 pos;
    glm::vec4 size;
};

struct NewVertex
{
    glm::ivec2 position;

    static VkVertexInputBindingDescription getBindingDescription()
    {
        VkVertexInputBindingDescription bindingDescription{};

        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(NewVertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 1> getAttributeDescriptions()
    {
        std::array<VkVertexInputAttributeDescription, 1> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = 0;

        // attributeDescriptions[4].binding = 0;
        // attributeDescriptions[4].location = 4;
        // attributeDescriptions[4].format = VK_FORMAT_R8_UINT;
        // attributeDescriptions[4].offset = offsetof(Vertex, useUbo);

        return attributeDescriptions;
    }
};

constexpr VkFormat ColorFormat = VK_FORMAT_B8G8R8A8_UNORM;

constexpr VkClearColorValue ClearColor = {{0.0f, 0.0f, 0.0f, 1.0f}};

// A rectangle is drawn using two triangles, each with 3 vertex indices.
constexpr uint32_t IndexBufferSize = 6 * sizeof(uint16_t);
constexpr uint32_t VertexBufferSize = 4 * sizeof(NewVertex);

constexpr int MaxDrawOffsetPixels = 24;

glm::vec4 colors::opacity(float value)
{
    DEBUG_ASSERT(0 <= value && value <= 1, "value must be in range [0.0f, 1.0f].");
    return glm::vec4(1.0f, 1.0f, 1.0f, value);
}

MapRenderer::MapRenderer(VulkanInfo &vulkanInfo, MapView *mapView)
    : mapView(mapView),
      vulkanInfo(vulkanInfo),
      vulkanTexturesForAppearances(Appearances::textureAtlasCount()),
      vulkanSwapChainImageSize(0, 0)
{
    activeTextureAtlasIds.reserve(Appearances::textureAtlasCount());

    size_t ArbitraryGeneralReserveAmount = 8;
    vulkanTextures.reserve(ArbitraryGeneralReserveAmount);
}

void MapRenderer::initResources(VkFormat colorFormat)
{
    // VME_LOG_D("[window: " << window.debugName << "] MapRenderer::initResources (device: " << window.device() << ")");

    vulkanInfo.update();
    this->colorFormat = colorFormat;

    createRenderPass();

    _currentFrame = &frames.front();

    createDescriptorSetLayouts();
    createGraphicsPipeline();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createVertexBuffer();
    createIndexBuffer();

    // VME_LOG_D("End MapRenderer::initResources");
}

void MapRenderer::initSwapChainResources(util::Size vulkanSwapChainImageSize)
{
    mapView->setViewportSize(vulkanSwapChainImageSize.width(), vulkanSwapChainImageSize.height());
}

void MapRenderer::releaseSwapChainResources()
{
    // mapView->setViewportSize(0, 0);
}

void MapRenderer::releaseResources()
{
    // auto device = window.device();
    // VME_LOG_D("[window: " << window.debugName << "] MapRenderer::releaseResources (device: " << device << ")");

    vulkanInfo.vkDestroyDescriptorSetLayout(uboDescriptorSetLayout, nullptr);
    uboDescriptorSetLayout = VK_NULL_HANDLE;

    vulkanInfo.vkDestroyDescriptorSetLayout(textureDescriptorSetLayout, nullptr);
    textureDescriptorSetLayout = VK_NULL_HANDLE;

    vulkanInfo.vkDestroyDescriptorPool(descriptorPool, nullptr);
    descriptorPool = VK_NULL_HANDLE;

    vulkanInfo.vkDestroyPipeline(graphicsPipeline, nullptr);
    graphicsPipeline = VK_NULL_HANDLE;

    vulkanInfo.vkDestroyPipelineLayout(pipelineLayout, nullptr);
    pipelineLayout = VK_NULL_HANDLE;

    vulkanInfo.vkDestroyRenderPass(renderPass, nullptr);
    renderPass = VK_NULL_HANDLE;

    vertexBuffer.releaseResources();
    indexBuffer.releaseResources();

    for (const auto id : activeTextureAtlasIds)
    {
        vulkanTexturesForAppearances.at(id).releaseResources();
    }
    activeTextureAtlasIds.clear();

    vulkanTextures.clear();

    for (auto &frame : frames)
    {
        frame.uniformBuffer = {};
        frame.commandBuffer = VK_NULL_HANDLE;
        frame.frameBuffer = VK_NULL_HANDLE;
        frame.uboDescriptorSet = VK_NULL_HANDLE;
    }

    debug = true;
}

void MapRenderer::startNextFrame()
{
    _containsAnimation = false;
    // VME_LOG_D("index: " << _currentFrame->currentFrameIndex);
    // VME_LOG("Start next frame");
    updateUniformBuffer();

    beginRenderPass();

    setupFrame();

    // Attempt to avoid possible floating point errors. Might be unnecessary.
    float zoom = mapView->getZoomFactor();
    auto floorZoom = std::floor(zoom);
    isDefaultZoom = floorZoom == zoom && floorZoom == 1;

    drawMap();
    drawCurrentAction();
    drawMapOverlay();

    vulkanInfo.vkCmdEndRenderPass(_currentFrame->commandBuffer);

    vulkanInfo.frameReady();
    // vulkanInfo.requestUpdate();
    currentDescriptorSet = nullptr;
}

void MapRenderer::setupFrame()
{
    vulkanInfo.vkCmdBindPipeline(_currentFrame->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

    const util::Size size = vulkanInfo.vulkanSwapChainImageSize();

    VkViewport viewport;
    viewport.x = viewport.y = 0;
    viewport.width = size.width();
    viewport.height = size.height();
    viewport.minDepth = 0;
    viewport.maxDepth = 1;
    vulkanInfo.vkCmdSetViewport(_currentFrame->commandBuffer, 0, 1, &viewport);

    VkRect2D scissor;
    scissor.offset.x = scissor.offset.y = 0;
    scissor.extent.width = viewport.width;
    scissor.extent.height = viewport.height;
    vulkanInfo.vkCmdSetScissor(_currentFrame->commandBuffer, 0, 1, &scissor);

    VkDeviceSize offsets[] = {0};

    vulkanInfo.vkCmdBindDescriptorSets(
        _currentFrame->commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipelineLayout,
        0,
        1,
        &_currentFrame->uboDescriptorSet,
        0,
        nullptr);

    vulkanInfo.vkCmdBindVertexBuffers(_currentFrame->commandBuffer, 0, 1, &vertexBuffer.buffer, offsets);
    vulkanInfo.vkCmdBindIndexBuffer(_currentFrame->commandBuffer, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16);
}

void MapRenderer::drawMap()
{
    MapView &view = *mapView;

    ItemPredicate filter = nullptr;
    if (mapView->draggingWithSubtract() && !Settings::AUTO_BORDER)
    {
        auto [from, to] = mapView->getDragPoints().value();
        Region2D dragRegion(from.toPos(view.floor()), to.toPos(view.floor()));
        Brush *brush = std::get<MouseAction::MapBrush>(mapView->editorAction.action()).brush;

        filter = [brush, dragRegion](const Position pos, const Item &item) {
            return !(brush->erasesItem(item.serverId()) && dragRegion.contains(pos));
        };
    }
    else if (mapView->editorAction.is<MouseAction::DragDropItem>())
    {
        MouseAction::DragDropItem *drag = mapView->editorAction.as<MouseAction::DragDropItem>();
        filter = [drag](const Position pos, const Item &item) { return &item != drag->item; };
    }

    bool movingSelection = view.selection().isMoving();
    bool shadeLowerFloors = view.hasOption(MapView::ViewOption::ShadeLowerFloors);

    int viewZ = view.z();

    uint32_t flags = ItemDrawFlags::DrawNonSelected;

    if (!movingSelection)
        flags |= ItemDrawFlags::DrawSelected;

    auto selectAction = mapView->editorAction.as<MouseAction::Select>();
    if (selectAction && selectAction->area)
        flags |= ItemDrawFlags::ActiveSelectionArea;

    for (auto &tileLocation : view.mapRegion(1, 1))
    {
        if (!tileLocation.hasTile() || (movingSelection && tileLocation.tile()->allSelected()))
            continue;

        uint32_t tileFlags = flags;
        if (shadeLowerFloors && tileLocation.z() > viewZ)
        {
            tileFlags |= ItemDrawFlags::Shade;
        }

        drawTile(tileLocation, tileFlags, filter);
    }

    // Draw paste preview
    auto pasteAction = mapView->editorAction.as<MouseAction::PasteMapBuffer>();
    if (pasteAction)
    {
        auto mapBuffer = pasteAction->buffer;
        for (auto &tileLocation : mapBuffer->getBufferMap().getRegion(mapBuffer->topLeft, mapBuffer->bottomRight))
        {
            if (!tileLocation.hasTile() || (movingSelection && tileLocation.tile()->allSelected()))
                continue;

            auto offset = mapView->mouseGamePos() - mapBuffer->topLeft;

            drawTile(tileLocation.tile(), ItemDrawFlags::DrawSelected | ItemDrawFlags::Ghost, offset, filter);
        }
    }
}

void MapRenderer::drawCurrentAction()
{
    // Render current mouse action
    std::visit(
        util::overloaded{
            [this](const MouseAction::Select select) {
                if (select.area)
                {
                    DEBUG_ASSERT(mapView->isDragging(), "action.area == true is invalid if no drag is active.");

                    // const auto [from, to] = mapView->getDragPoints().value();
                    // drawSolidRectangle(SolidColor::Blue, from, to, 0.1f);
                }
                else if (mapView->selection().isMoving())
                {
                    drawMovingSelection();
                }
            },

            [this](const MouseAction::MapBrush &action) {
                Position pos = mapView->mouseGamePos();
                int variation = mapView->getBrushVariation();

                if (action.area)
                {
                    DEBUG_ASSERT(mapView->isDragging(), "action.area == true is invalid if no drag is active.");

                    if (mapView->draggingWithSubtract())
                    {
                        const auto [from, to] = mapView->getDragPoints().value();
                        Texture &texture = Texture::getOrCreateSolidTexture(SolidColor::Red);

                        drawRectangle(texture, from, to, 0.2f);
                        drawBrushPreviewAtWorldPos(action.brush, to, variation);
                    }
                    else
                    {
                        switch (action.brush->type())
                        {
                            case BrushType::Raw:
                            case BrushType::Ground:
                            {
                                auto [from, to] = mapView->getDragPoints().value();
                                int floor = mapView->floor();
                                auto area = MapArea(*mapView->map(), from.toPos(floor), to.toPos(floor));

                                for (auto &pos : area)
                                {
                                    drawPreview(action.brush->getPreviewTextureInfo(variation).at(0), pos);
                                }
                                break;
                            }
                            case BrushType::Wall:
                            {
                                auto wallBrush = static_cast<WallBrush *>(action.brush);
                                auto [from, to] = mapView->getDragPoints().value();
                                int floor = mapView->floor();

                                auto fromPos = from.toPos(floor);
                                auto toPos = to.toPos(floor);

                                auto previews = wallBrush->getPreviewTextureInfo(fromPos, toPos);
                                for (const auto &preview : previews)
                                {
                                    drawPreview(preview, fromPos);
                                }

                                break;
                            }
                            case BrushType::Mountain:
                            {
                                auto [from, to] = mapView->getDragPoints().value();
                                int floor = mapView->floor();
                                auto area = MapArea(*mapView->map(), from.toPos(floor), to.toPos(floor));

                                for (auto &pos : area)
                                {
                                    drawPreview(action.brush->getPreviewTextureInfo(variation).at(0), pos);
                                }
                                break;
                            }
                            default:
                                // TODO Area drag with other brushes
                                VME_LOG("Area drag with the current brush is not implemented.");
                                break;
                        }
                    }
                }
                else
                {
                    if (mapView->underMouse())
                    {
                        int variation = mapView->getBrushVariation();
                        drawBrushPreview(action.brush, pos, variation);
                    }
                }
            },

            [this](const MouseAction::DragDropItem drag) {
                if (mapView->underMouse())
                {
                    ItemDrawInfo info{};
                    info.item = drag.item;
                    info.position = drag.tile->position() + drag.moveDelta.value();

                    drawItem(info);
                }
            },

            [](const auto &arg) {}},
        _currentFrame->mouseAction);
}

bool MapRenderer::insideMap(const Position &position)
{
    return !(position.x < 0 || position.x > mapView->mapWidth() || position.y < 0 || position.y > mapView->mapHeight());
}

void MapRenderer::drawPreview(ThingDrawInfo drawInfo, const Position &position)
{
    rollbear::visit(
        util::overloaded{
            [this, position](const DrawItemType &draw) {
                auto drawPos = position + draw.relativePosition;

                if (!insideMap(drawPos))
                {
                    return;
                }

                ItemTypeDrawInfo info{};
                info.color = colors::ItemPreview;
                info.itemType = draw.itemType;
                info.worldPos = drawPos.worldPos();
                info.spriteId = draw.itemType->getSpriteId(drawPos);

                if (!draw.itemType->isGround())
                {
                    const Tile *tile = mapView->getTile(drawPos);
                    int elevation = tile ? tile->getTopElevation() : 0;
                    info.worldPosOffset = {-elevation, -elevation};
                }

                this->drawItemType(info);
            },
            [this, position](const DrawCreatureType &draw) {
                auto drawPos = position + draw.relativePosition;
                drawCreatureType(*draw.creatureType, drawPos, draw.direction, colors::ItemPreview);
            },

            [](const auto &arg) {
                ABORT_PROGRAM("Unknown ThingDrawInfo.");
            }},
        drawInfo);
}

void MapRenderer::drawCreatureType(const CreatureType &creatureType, const Position position, Direction direction, glm::vec4 color, const DrawOffset &drawOffset)
{
    auto drawPart = [this, position, color, drawOffset](const CreatureType *creatureType, int posture, int addonType, Direction direction) {
        if (!insideMap(position))
        {
            return;
        }

        DrawInfo::Creature info;
        info.color = color;
        info.textureInfo = creatureType->getTextureInfo(0, posture, addonType, direction);

        auto texture = creatureType->hasColorVariation()
                           ? info.textureInfo.getTexture(creatureType->outfitId())
                           : info.textureInfo.getTexture();

        info.descriptorSet = objectDescriptorSet(texture);
        info.position = position;
        info.width = info.textureInfo.atlas->spriteWidth;
        info.height = info.textureInfo.atlas->spriteHeight;
        info.drawOffset = drawOffset;

        this->drawCreature(info);
    };

    uint8_t posture = creatureType.hasMount() ? 1 : 0;

    // Mount?
    if (creatureType.hasMount())
    {
        // Draw mount first
        drawPart(Creatures::creatureType(creatureType.mountLooktype()), 0, 0, direction);

        drawPart(&creatureType, 1, 0, direction);
    }
    else
    {
        // No mount, draw the base outfit
        drawPart(&creatureType, posture, 0, direction);
    }

    // Addons?
    if (creatureType.hasAddon(Outfit::Addon::First))
    {
        drawPart(&creatureType, posture, 1, direction);
    }
    if (creatureType.hasAddon(Outfit::Addon::Second))
    {
        drawPart(&creatureType, posture, 2, direction);
    }
}

void MapRenderer::drawPreviewItem(uint32_t serverId, Position pos)
{
    if (pos.x < 0 || pos.x > mapView->mapWidth() || pos.y < 0 || pos.y > mapView->mapHeight())
        return;

    ItemType *itemType = Items::items.getItemTypeByServerId(serverId);

    ItemTypeDrawInfo info{};
    info.color = colors::ItemPreview;
    info.itemType = itemType;
    info.worldPos = pos.worldPos();
    info.spriteId = itemType->getSpriteId(pos);

    if (!itemType->isGround())
    {
        const Tile *tile = mapView->getTile(pos);
        int elevation = tile ? tile->getTopElevation() : 0;
        info.worldPosOffset = {-elevation, -elevation};
    }

    drawItemType(info);
}

void MapRenderer::drawMovingSelection()
{
    // External drag operation (e.g. for dropping an item in a container)
    if (!mapView->underMouse() && mapView->singleThingSelected())
    {
        return;
    }

    Position moveDelta = mapView->editorAction.as<MouseAction::Select>()->moveDelta.value();

    auto mapRect = mapView->getGameBoundingRect();
    mapRect = mapRect.translate(-moveDelta.x, -moveDelta.y, {0, 0});

    // TODO: Use selection Z bounds instead of all floors
    int startZ = MAP_LAYERS - 1;
    int endZ = 0;

    Position from(mapRect.x1, mapRect.y1, startZ);
    Position to(mapRect.x2, mapRect.y2, endZ);

    for (auto &tileLocation : mapView->map()->getRegion(from, to))
    {
        if (tileLocation.hasTile())
        {
            // Draw only if the tile has a selection.
            if (tileLocation.tile()->hasSelection())
            {
                drawTile(tileLocation, ItemDrawFlags::DrawSelected, moveDelta);
            }
        }
    }
}

void MapRenderer::drawMapOverlay()
{
    auto &overlay = mapView->overlay();
    if (overlay.draggedItem)
    {
        auto item = overlay.draggedItem;

        ItemDrawInfo info{};
        info.item = item;
        info.position = mapView->mouseGamePos();

        drawItem(info);
    }
}

bool MapRenderer::shouldDrawItem(const Position pos, const Item &item, uint32_t flags, const ItemPredicate &filter) const noexcept
{
    bool selected = item.selected && (flags & ItemDrawFlags::DrawSelected);
    bool unselected = !item.selected && (flags & ItemDrawFlags::DrawNonSelected);
    bool passFilter = !filter || filter(pos, item);

    return (selected || unselected) && passFilter;
}

void MapRenderer::drawTile(const TileLocation &tileLocation, uint32_t flags, const ItemPredicate &filter)
{
    drawTile(tileLocation, flags, PositionConstants::Zero, filter);
}

void MapRenderer::drawTile(const TileLocation &tileLocation, uint32_t flags, const Position offset, const ItemPredicate &filter)
{
    drawTile(tileLocation.tile(), flags, offset, filter);
}

void MapRenderer::drawTile(Tile *tile, uint32_t flags, const Position offset, const ItemPredicate &filter)
{
    auto position = tile->position();
    position += offset;

    Item *groundPtr = tile->ground();
    if (groundPtr != nullptr)
    {
        if (shouldDrawItem(position, *groundPtr, flags, filter))
        {
            if (Settings::RENDER_ANIMATIONS)
            {
                if (groundPtr->hasAnimation())
                {
                    groundPtr->animate();
                    _containsAnimation = true;
                }
            }

            ItemDrawInfo info{};

            info.drawFlags = flags;
            info.item = groundPtr;
            info.position = position;

            drawItem(info);
        }
    }

    DrawOffset worldPosOffset{0, 0};
    ItemDrawInfo info{};

    for (const std::shared_ptr<Item> &itemPtr : tile->items())
    {
        const Item &item = *itemPtr;
        if (!shouldDrawItem(position, item, flags, filter))
            continue;

        if (Settings::RENDER_ANIMATIONS)
        {
            if (item.hasAnimation())
            {
                item.animate();
                _containsAnimation = true;
            }
        }

        info.drawFlags = flags;
        info.item = &item;
        info.position = position;
        info.worldPosOffset = worldPosOffset;

        drawItem(info);

        if (item.itemType->hasElevation())
        {
            uint32_t elevation = item.itemType->getElevation();
            worldPosOffset.x -= elevation;
            worldPosOffset.y -= elevation;
        }
    }

    if (tile->hasCreature())
    {
        auto &creature = *tile->creature();
        auto color = getCreatureDrawColor(creature, position, flags);
        drawCreatureType(creature.creatureType, position, creature.direction(), color, worldPosOffset);
        // drawCreature(creatureDrawInfo(creature, position, flags));
    }
}

void MapRenderer::issueDraw(const DrawInfo::Base &info, const WorldPosition &worldPos)
{
    const auto atlas = info.textureInfo.atlas;
    const auto &window = info.textureInfo.window;
    PushConstantData pushConstant{};

    glm::vec4 pos{};
    pos.x = worldPos.x;
    pos.y = worldPos.y;
    pushConstant.pos = pos;

    glm::vec4 size{};
    size.x = info.width;
    size.y = info.height;

    pushConstant.size = size;
    pushConstant.color = info.color;
    pushConstant.textureQuad = glm::vec4(window.x0, window.y0, window.x1, window.y1);
    pushConstant.fragQuad = atlas->getFragmentBounds(window);

    vulkanInfo.vkCmdBindDescriptorSets(
        _currentFrame->commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipelineLayout,
        1,
        1,
        &info.descriptorSet,
        0,
        nullptr);

    vulkanInfo.vkCmdPushConstants(_currentFrame->commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstantData), &pushConstant);
    vulkanInfo.vkCmdDrawIndexed(_currentFrame->commandBuffer, 6, 1, 0, 0, 0);
}

void MapRenderer::issueRectangleDraw(DrawInfo::Rectangle &info)
{
    PushConstantData pushConstant{};

    if (std::holds_alternative<const Texture *>(info.texture))
    {
        pushConstant.textureQuad = {0, 0, 1, 1};
        pushConstant.fragQuad = {0, 0, 1, 1};
    }
    else if (std::holds_alternative<TextureInfo>(info.texture))
    {
        const TextureInfo textureInfo = std::get<TextureInfo>(info.texture);
        pushConstant.textureQuad = textureInfo.window.asVec4();
        pushConstant.fragQuad = textureInfo.atlas->getFragmentBounds(textureInfo.window);
    }

    auto [x1, y1] = info.from;
    auto [x2, y2] = info.to;

    // Handle the possible quadrants to draw the texture correctly
    if (x1 > x2)
    {
        std::swap(x1, x2);
        if (y1 > y2)
        {
            std::swap(y1, y2);
        }
    }
    else if (x1 < x2 && y1 > y2)
    {
        std::swap(y1, y2);
    }

    glm::vec4 pos{};
    pos.x = x1;
    pos.y = y1;
    pushConstant.pos = pos;

    glm::vec4 size{};
    size.x = std::abs(x2 - x1);
    size.y = std::abs(y2 - y1);
    pushConstant.size = size;
    pushConstant.color = info.color;

    vulkanInfo.vkCmdBindDescriptorSets(
        _currentFrame->commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipelineLayout,
        1,
        1,
        &info.descriptorSet,
        0,
        nullptr);

    vulkanInfo.vkCmdPushConstants(_currentFrame->commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstantData), &pushConstant);
    vulkanInfo.vkCmdDrawIndexed(_currentFrame->commandBuffer, 6, 1, 0, 0, 0);
}

void MapRenderer::drawBrushPreview(Brush *brush, const Position &position, int variation)
{
    SolidColor color = Settings::BORDER_BRUSH_VARIATION == BorderBrushVariationType::Detailed
                           ? SolidColor::Green
                           : SolidColor::MaterialUIBlue600;

    if (brush->type() == BrushType::Border)
    {
        auto mouseWorldPos = mapView->mousePos().worldPos(*mapView);
        int borderWidth = 1;
        int quadrantSide = MapTileSize / 2 - borderWidth;

        int dx = borderWidth;
        int dy = borderWidth;
        switch (mouseWorldPos.tileQuadrant())
        {
            case TileQuadrant::TopLeft:
                break;
            case TileQuadrant::TopRight:
                dx += quadrantSide;
                break;
            case TileQuadrant::BottomRight:
                dx += quadrantSide;
                dy += quadrantSide;
                break;
            case TileQuadrant::BottomLeft:
                dy += quadrantSide;
                break;
        }
        auto worldPos = position.worldPos();

        // TODO Fix border drawing when zoomed out. For now, we disable rectangle border drawing when zoomed out.
        // If fix by using a different shader, then we need a new shader pipeline (shaders are part of the pipeline state).
        if (mapView->getZoomFactor() >= 1)
        {
            auto quadrantSquare = RectangleDrawInfo::solid(color, worldPos + WorldPosition(dx, dy), quadrantSide, 0.35f);
            auto tileBorder = RectangleDrawInfo::border(color, worldPos, MapTileSize, 0.35f);
            drawRectangle(quadrantSquare);
            drawRectangle(tileBorder);
        }
        else
        {
            auto square = RectangleDrawInfo::solid(color, worldPos, MapTileSize, 0.35f);
            drawRectangle(square);
        }
    }
    else
    {
        for (const auto preview : brush->getPreviewTextureInfo(variation))
            drawPreview(preview, position);
    }
}

void MapRenderer::drawBrushPreviewAtWorldPos(Brush *brush, const WorldPosition &worldPos, int variation)
{
    for (const auto &drawInfo : brush->getPreviewTextureInfo(variation))
    {
        rollbear::visit(
            util::overloaded{
                [this, worldPos](const DrawItemType &draw) {
                    ItemTypeDrawInfo info{};
                    info.color = colors::ItemPreview;
                    info.itemType = draw.itemType;
                    info.spriteId = draw.itemType->appearance->getFirstSpriteId();

                    Position pos = draw.relativePosition;
                    // Position::worldPos() skews x and y depending on z. Since the preview is z-irrelevant, we need to add
                    // GROUND_FLOOR to get to the baseline position.
                    pos.z += GROUND_FLOOR;

                    info.worldPos = worldPos + pos.worldPos();

                    this->drawItemType(info);
                },
                [this, worldPos](const DrawCreatureType &draw) {
                    DrawInfo::Creature info;
                    info.color = colors::ItemPreview;
                    info.textureInfo = draw.creatureType->getTextureInfo(0, draw.direction);
                    auto texture = draw.creatureType->hasColorVariation()
                                       ? info.textureInfo.getTexture(draw.creatureType->outfitId())
                                       : info.textureInfo.getTexture();

                    info.descriptorSet = objectDescriptorSet(texture);

                    info.width = info.textureInfo.atlas->spriteWidth;
                    info.height = info.textureInfo.atlas->spriteHeight;

                    auto adjustedWorldPos = worldPos + Position(info.textureInfo.atlas->drawOffset.x, info.textureInfo.atlas->drawOffset.y, GROUND_FLOOR).worldPos();

                    issueDraw(info, adjustedWorldPos);
                },

                [](const auto &arg) {
                    ABORT_PROGRAM("Unknown ThingDrawInfo.");
                }},
            drawInfo);
    }
}

WorldPosition MapRenderer::getWorldPosForDraw(const ItemTypeDrawInfo &info, TextureAtlas *atlas) const
{
    WorldPosition worldPos = info.worldPos + atlas->worldPosOffset();

    // Add draw offsets like elevation
    worldPos.x += std::clamp(info.worldPosOffset.x, -MaxDrawOffsetPixels, MaxDrawOffsetPixels);
    worldPos.y += std::clamp(info.worldPosOffset.y, -MaxDrawOffsetPixels, MaxDrawOffsetPixels);

    auto appearance = info.itemType->appearance;

    // Add the item shift if necessary
    if (appearance->hasFlag(AppearanceFlag::Shift))
    {
        worldPos.x -= appearance->flagData.shiftX;
        worldPos.y -= appearance->flagData.shiftY;
    }

    return worldPos;
}

void MapRenderer::drawOverlayItemType(uint32_t serverId, const WorldPosition position, const glm::vec4 color)
{
    ItemType &itemType = *Items::items.getItemTypeByServerId(serverId);
    DrawInfo::OverlayObject info;
    info.position = position;
    info.color = color;
    info.textureInfo = itemType.getTextureInfo();
    info.descriptorSet = objectDescriptorSet(info.textureInfo.atlas);

    issueDraw(info, position);
}

void MapRenderer::drawCreature(const DrawInfo::Creature &info)
{
    WorldPosition worldPos = (info.position + Position(info.textureInfo.atlas->drawOffset.x, info.textureInfo.atlas->drawOffset.y, 0)).worldPos();

    // Add draw offsets like elevation
    worldPos.x += std::clamp(info.drawOffset.x, -MaxDrawOffsetPixels, MaxDrawOffsetPixels);
    worldPos.y += std::clamp(info.drawOffset.y, -MaxDrawOffsetPixels, MaxDrawOffsetPixels);

    issueDraw(info, worldPos);
}

void MapRenderer::drawRectangle(const RectangleDrawInfo &info)
{
    // Draw border
    if (info.borderColor)
    {
        auto pos = info.position;
        Texture &texture = Texture::getOrCreateSolidTexture(*info.borderColor);
        // Top
        drawRectangle(texture, pos, info.width, 1, info.opacity);

        // Right
        drawRectangle(texture, pos + WorldPosition(info.width - 1, 1), 1, info.height - 2, info.opacity);

        // Bottom
        drawRectangle(texture, pos + WorldPosition(0, info.height - 1), info.width, 1, info.opacity);

        // Left
        drawRectangle(texture, pos + WorldPosition(0, 1), 1, info.height - 2, info.opacity);

        // {
        //     // Debug RED
        //     Texture &red = Texture::getOrCreateSolidTexture(SolidColor::Red);
        //     auto offset = WorldPosition(0, 1);
        //     // Top
        //     drawRectangle(red, pos + offset, info.width / 2, 1, info.opacity);

        //     // Bottom
        //     drawRectangle(red, pos + offset + WorldPosition(0, info.height - 1), info.width / 2, 1, info.opacity);
        // }

        // {
        //     // Debug YELLOW
        //     Texture &red = Texture::getOrCreateSolidTexture(SolidColor::Yellow);
        //     auto offset = WorldPosition(0, 2);
        //     // Top
        //     drawRectangle(red, pos + offset, info.width / 2, 1, info.opacity);

        //     // Bottom
        //     drawRectangle(red, pos + offset + WorldPosition(0, info.height - 1), info.width / 2, 1, info.opacity);
        // }

        // {
        //     // Debug BLUE
        //     Texture &blue = Texture::getOrCreateSolidTexture(SolidColor::Blue);
        //     auto offset = WorldPosition(info.width / 2, -1);
        //     // Top
        //     drawRectangle(blue, pos + offset, info.width / 2, 1, info.opacity);

        //     // Bottom
        //     drawRectangle(blue, pos + offset + WorldPosition(0, info.height - 1), info.width / 2, 1, info.opacity);
        // }
    }

    if (info.color)
    {
        Texture &texture = Texture::getOrCreateSolidTexture(*info.color);

        drawRectangle(texture, info.position, info.width, info.height, info.opacity);
    }
}

void MapRenderer::drawRectangle(const Texture &texture, const WorldPosition from, int width, int height, float opacity)
{
    drawRectangle(texture, from, from + WorldPosition(width, height), opacity);
}

void MapRenderer::drawRectangle(const Texture &texture, const WorldPosition from, const WorldPosition to, float opacity)
{
    VulkanTexture::Descriptor descriptor;
    descriptor.layout = textureDescriptorSetLayout;
    descriptor.pool = descriptorPool;

    auto &vulkanTexture = vulkanTextures[&texture];
    if (!vulkanTexture.hasResources())
        vulkanTexture.initResources(texture, vulkanInfo, descriptor);

    DrawInfo::Rectangle info;
    info.from = from;
    info.to = to;
    info.texture = &texture;
    info.color = colors::opacity(opacity);
    info.descriptorSet = vulkanTexture.descriptorSet();

    issueRectangleDraw(info);
}

glm::vec4 MapRenderer::getItemDrawColor(const Item &item, const Position &position, uint32_t drawFlags)
{
    if (drawFlags & ItemDrawFlags::Ghost)
    {
        return colors::ItemPreview;
    }
    bool drawAsSelected = item.selected || ((drawFlags & ItemDrawFlags::ActiveSelectionArea) && mapView->inDragRegion(position));
    if (drawAsSelected)
    {
        return colors::Selected;
    }
    else if (drawFlags & ItemDrawFlags::Shade)
    {
        return colors::Shade;
    }
    else
    {
        return colors::Default;
    }
}

glm::vec4 MapRenderer::getItemTypeDrawColor(uint32_t drawFlags)
{
    return drawFlags & ItemDrawFlags::Ghost ? colors::ItemPreview : colors::Default;
}

glm::vec4 MapRenderer::getCreatureDrawColor(const Creature &creature, const Position &position, uint32_t drawFlags) const
{
    bool drawAsSelected = creature.selected || ((drawFlags & ItemDrawFlags::ActiveSelectionArea) && mapView->inDragRegion(position));
    if (drawAsSelected)
    {
        return colors::Selected;
    }
    else if (drawFlags & ItemDrawFlags::Shade)
    {
        return colors::Shade;
    }
    else
    {
        return colors::Default;
    }
}

void MapRenderer::drawItemType(const ItemTypeDrawInfo &drawInfo, QuadrantRenderType renderType)
{
    const ItemType *itemType = drawInfo.itemType;

    // When zoomed in or zoomed out, there are special cases for drawing grounds with a 64x64 texture. These cases are
    // sprites where only the top-left (or top-left and bottom-right, like serverID 8133) quadrant is non-transparent.
    // These extra render types make sure that these special grounds are tiled properly when placed next to each other.
    // Without these rules, antialiasing creates black borders between separate tiles of these grounds when they are
    // tiled (i.e. placed next to each other).
    switch (renderType)
    {
        case QuadrantRenderType::Full:
        {
            DrawInfo::Object info{};

            info.color = drawInfo.color;
            info.textureInfo = itemType->getTextureInfo(drawInfo.spriteId);
            info.descriptorSet = objectDescriptorSet(info.textureInfo.atlas);
            info.width = info.textureInfo.atlas->spriteWidth;
            info.height = info.textureInfo.atlas->spriteHeight;

            auto worldPos = getWorldPosForDraw(drawInfo, info.textureInfo.atlas);
            issueDraw(info, worldPos);
            break;
        }
        case QuadrantRenderType::TopLeft:
        {
            DrawInfo::ObjectQuadrant info{};
            info.color = drawInfo.color;
            info.textureInfo = itemType->getTextureInfoTopLeftQuadrant(drawInfo.spriteId);
            info.width = info.textureInfo.atlas->spriteWidth / 2;
            info.height = info.textureInfo.atlas->spriteHeight / 2;
            info.descriptorSet = objectDescriptorSet(info.textureInfo.atlas);

            auto worldPos = getWorldPosForDraw(drawInfo, info.textureInfo.atlas);
            issueDraw(info, worldPos);

            break;
        }
        case QuadrantRenderType::TopLeftBottomRight:
        {
            DrawInfo::ObjectQuadrant info{};
            info.color = drawInfo.color;

            const auto [topLeftTextureInfo,
                        bottomRightTextureInfo] = itemType->getTextureInfoTopLeftBottomRightQuadrant(drawInfo.spriteId);

            auto atlas = topLeftTextureInfo.atlas;

            info.textureInfo = bottomRightTextureInfo;
            info.width = atlas->spriteWidth / 2;
            info.height = atlas->spriteHeight / 2;

            info.descriptorSet = objectDescriptorSet(atlas);

            auto worldPos = getWorldPosForDraw(drawInfo, atlas);

            issueDraw(info, worldPos + WorldPosition(atlas->spriteWidth / 2, atlas->spriteHeight / 2));

            info.textureInfo = topLeftTextureInfo;
            issueDraw(info, worldPos);
            break;
        }
        case QuadrantRenderType::TopRightBottomRightBottomLeft:
        {
            DrawInfo::ObjectQuadrant info{};
            info.color = drawInfo.color;

            const auto [topRightTextureInfo,
                        bottomRightTextureInfo,
                        bottomLeftTextureInfo] = itemType->getTextureInfoTopRightBottomRightBottomLeftQuadrant(drawInfo.spriteId);

            auto atlas = topRightTextureInfo.atlas;

            info.textureInfo = bottomRightTextureInfo;
            info.width = atlas->spriteWidth / 2;
            info.height = atlas->spriteHeight / 2;
            info.descriptorSet = objectDescriptorSet(atlas);

            auto worldPos = getWorldPosForDraw(drawInfo, atlas);

            issueDraw(info, worldPos + WorldPosition(atlas->spriteWidth / 2, atlas->spriteHeight / 2));

            info.textureInfo = bottomLeftTextureInfo;
            issueDraw(info, worldPos + WorldPosition(0, atlas->spriteHeight / 2));

            info.textureInfo = topRightTextureInfo;
            issueDraw(info, worldPos + WorldPosition(atlas->spriteWidth / 2, 0));
            break;
        }
        default:
            ABORT_PROGRAM("Should be unreachable.");
    }
}

void MapRenderer::drawItemType(const ItemTypeDrawInfo &drawInfo)
{
    QuadrantRenderType renderType = isDefaultZoom
                                        ? QuadrantRenderType::Full
                                        : drawInfo.itemType->appearance->quadrantRenderType;

    drawItemType(drawInfo, renderType);
}

void MapRenderer::drawItem(const ItemDrawInfo &itemDrawInfo)
{
    ItemTypeDrawInfo info{};
    info.color = getItemDrawColor(*itemDrawInfo.item, itemDrawInfo.position, itemDrawInfo.drawFlags);
    info.itemType = itemDrawInfo.item->itemType;
    info.worldPos = itemDrawInfo.position.worldPos();
    info.spriteId = itemDrawInfo.item->getSpriteId(itemDrawInfo.position);
    info.worldPosOffset = itemDrawInfo.worldPosOffset;

    drawItemType(info);
}

DrawInfo::Object MapRenderer::getItemDrawInfo(const Item &item, const Position &position, uint32_t drawFlags)
{
    DrawInfo::Object info;
    info.position = position;
    info.color = getItemDrawColor(item, position, drawFlags);
    info.textureInfo = item.getTextureInfo(position);
    info.descriptorSet = objectDescriptorSet(info.textureInfo.atlas->getOrCreateTexture());
    info.width = info.textureInfo.atlas->spriteWidth;
    info.height = info.textureInfo.atlas->spriteHeight;

    return info;
}

DrawInfo::Creature MapRenderer::creatureDrawInfo(const Creature &creature, const Position &position, uint32_t drawFlags) const
{
    DrawInfo::Creature info;
    info.color = getCreatureDrawColor(creature, position, drawFlags);
    info.textureInfo = creature.getTextureInfo();

    auto texture = creature.creatureType.hasColorVariation()
                       ? info.textureInfo.getTexture(creature.creatureType.outfitId())
                       : info.textureInfo.getTexture();

    info.descriptorSet = objectDescriptorSet(texture);

    info.position = position;
    info.width = info.textureInfo.atlas->spriteWidth;
    info.height = info.textureInfo.atlas->spriteHeight;

    return info;
}

DrawInfo::Object MapRenderer::itemTypeDrawInfo(const ItemType &itemType, const Position &position, uint32_t drawFlags)
{
    DrawInfo::Object info;
    info.position = position;
    info.color = getItemTypeDrawColor(drawFlags);
    info.textureInfo = itemType.getTextureInfo(position);
    info.descriptorSet = objectDescriptorSet(info.textureInfo.atlas->getOrCreateTexture());
    info.width = info.textureInfo.atlas->spriteWidth;
    info.height = info.textureInfo.atlas->spriteHeight;

    return info;
}

VkDescriptorSet MapRenderer::objectDescriptorSet(TextureAtlas *atlas) const
{
    return objectDescriptorSet(atlas->getOrCreateTexture());
}

VkDescriptorSet MapRenderer::objectDescriptorSet(const Texture &texture) const
{
    VulkanTexture::Descriptor descriptor;
    descriptor.layout = textureDescriptorSetLayout;
    descriptor.pool = descriptorPool;
    uint32_t id = texture.id();

    if (vulkanTexturesForAppearances.size() < id)
    {
        vulkanTexturesForAppearances.resize(vulkanTexturesForAppearances.size() * 1.25);
    }

    VulkanTexture &vulkanTexture = vulkanTexturesForAppearances.at(id);

    if (!vulkanTexture.hasResources())
    {
        if (vulkanTexture.unused)
        {
            activeTextureAtlasIds.emplace_back(id);
        }

        vulkanTexture.initResources(texture, vulkanInfo, descriptor);
    }

    return vulkanTexture.descriptorSet();
}

void MapRenderer::updateUniformBuffer()
{
    glm::mat4 projection = vulkanInfo.projectionMatrix(mapView);
    // const auto p = projection;
    // std::ostringstream s;
    // VME_LOG_D("Next:");
    // for (int i = 0; i < 4; ++i)
    // {
    //   s << "x: " << p[i].x << ", ";
    //   s << "y: " << p[i].y << ", ";
    //   s << "z: " << p[i].z << ", ";
    //   s << "w: " << p[i].w << ", ";
    //   s << std::endl;
    // }
    // VME_LOG_D(s.str());
    ItemUniformBufferObject uniformBufferObject{projection};

    void *data;
    vulkanInfo.vkMapMemory(_currentFrame->uniformBuffer.deviceMemory, 0, sizeof(ItemUniformBufferObject), 0, &data);
    memcpy(data, &uniformBufferObject, sizeof(ItemUniformBufferObject));
    vulkanInfo.vkUnmapMemory(_currentFrame->uniformBuffer.deviceMemory);
}

/**
 * 
 * >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
 * >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
 * >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
 * Vulkan rendering setup/teardown
 * >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
 * >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
 * >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
 *
 **/
void MapRenderer::beginRenderPass()
{
    util::Size size = vulkanInfo.vulkanSwapChainImageSize();
    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = _currentFrame->frameBuffer;
    renderPassInfo.renderArea.extent.width = size.width();
    renderPassInfo.renderArea.extent.width = size.height();

    VkExtent2D extent;
    extent.width = size.width();
    extent.height = size.height();
    renderPassInfo.renderArea.extent = extent;

    renderPassInfo.clearValueCount = 1;
    VkClearValue clearValue;
    clearValue.color = ClearColor;
    renderPassInfo.pClearValues = &clearValue;

    vulkanInfo.vkCmdBeginRenderPass(_currentFrame->commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void MapRenderer::createRenderPass()
{
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = this->colorFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vulkanInfo.vkCreateRenderPass(&renderPassInfo, nullptr, &this->renderPass) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create render pass!");
    }
}

void MapRenderer::createGraphicsPipeline()
{
    std::vector<uint8_t> vertShaderCode = File::read("shaders/vert.spv");
    std::vector<uint8_t> fragShaderCode = File::read("shaders/frag.spv");

    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    auto bindingDescription = NewVertex::getBindingDescription();
    auto attributeDescriptions = NewVertex::getAttributeDescriptions();

    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // The viewport and scissor will be set dynamically via vkCmdSetViewport/Scissor.
    // This way the pipeline does not need to be touched when resizing the window.
    VkPipelineViewportStateCreateInfo viewportState;
    memset(&viewportState, 0, sizeof(viewportState));
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT;

    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    // colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkDynamicState dynEnable[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicInfo;
    memset(&dynamicInfo, 0, sizeof(dynamicInfo));
    dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicInfo.dynamicStateCount = sizeof(dynEnable) / sizeof(VkDynamicState);
    dynamicInfo.pDynamicStates = dynEnable;

    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(PushConstantData);

    std::array<VkDescriptorSetLayout, 2> layouts = {uboDescriptorSetLayout, textureDescriptorSetLayout};
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
    pipelineLayoutInfo.pSetLayouts = layouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vulkanInfo.vkCreatePipelineLayout(&pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicInfo;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    if (vulkanInfo.vkCreateGraphicsPipelines(VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) !=
        VK_SUCCESS)
    {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    vulkanInfo.vkDestroyShaderModule(fragShaderModule, nullptr);
    vulkanInfo.vkDestroyShaderModule(vertShaderModule, nullptr);
}

void MapRenderer::createDescriptorSetLayouts()
{
    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;

    VkDescriptorSetLayoutBinding uboLayoutBinding = {};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    layoutInfo.pBindings = &uboLayoutBinding;

    if (vulkanInfo.vkCreateDescriptorSetLayout(&layoutInfo, nullptr, &uboDescriptorSetLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create descriptor set layout for the uniform buffer object.");
    }

    VkDescriptorSetLayoutBinding textureLayoutBinding = {};
    textureLayoutBinding.binding = 0;
    textureLayoutBinding.descriptorCount = 1;
    textureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    textureLayoutBinding.pImmutableSamplers = nullptr;
    textureLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    layoutInfo.pBindings = &textureLayoutBinding;

    if (vulkanInfo.vkCreateDescriptorSetLayout(&layoutInfo, nullptr, &textureDescriptorSetLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create descriptor set layout for the textures.");
    }
}

void MapRenderer::createUniformBuffers()
{
    VkDeviceSize bufferSize = sizeof(ItemUniformBufferObject);

    Buffer::CreateInfo info;
    info.vulkanInfo = &vulkanInfo;
    info.size = bufferSize;
    info.usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    info.memoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    for (size_t i = 0; i < vulkanInfo.maxConcurrentFrameCount(); i++)
    {
        frames[i].uniformBuffer = Buffer::create(info);
    }
}

void MapRenderer::createDescriptorPool()
{
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    uint32_t descriptorCount = vulkanInfo.maxConcurrentFrameCount() * 2;

    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = descriptorCount;

    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = MAX_NUM_TEXTURES;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = descriptorCount + MAX_NUM_TEXTURES;

    if (vulkanInfo.vkCreateDescriptorPool(&poolInfo, nullptr, &this->descriptorPool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

void MapRenderer::createDescriptorSets()
{
    const uint32_t maxFrames = 3;
    std::vector<VkDescriptorSetLayout> layouts(maxFrames, uboDescriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = maxFrames;
    allocInfo.pSetLayouts = layouts.data();

    std::array<VkDescriptorSet, maxFrames> descriptorSets;

    if (vulkanInfo.vkAllocateDescriptorSets(&allocInfo, &descriptorSets[0]) != VK_SUCCESS)
    {
        ABORT_PROGRAM("failed to allocate descriptor sets");
    }

    for (uint32_t i = 0; i < descriptorSets.size(); ++i)
    {
        frames[i].uboDescriptorSet = descriptorSets[i];
    }

    for (size_t i = 0; i < vulkanInfo.maxConcurrentFrameCount(); ++i)
    {
        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer = frames[i].uniformBuffer.buffer;
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(ItemUniformBufferObject);

        VkWriteDescriptorSet descriptorWrites = {};
        descriptorWrites.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites.dstSet = frames[i].uboDescriptorSet;
        descriptorWrites.dstBinding = 0;
        descriptorWrites.dstArrayElement = 0;
        descriptorWrites.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites.descriptorCount = 1;
        descriptorWrites.pBufferInfo = &bufferInfo;

        vulkanInfo.vkUpdateDescriptorSets(1, &descriptorWrites, 0, nullptr);
    }
}

void MapRenderer::createVertexBuffer()
{
    std::array<glm::ivec2, 4> vertices{{{0, 0}, {0, 1}, {1, 1}, {1, 0}}};

    Buffer::CreateInfo stagingInfo;
    stagingInfo.vulkanInfo = &vulkanInfo;
    stagingInfo.size = VertexBufferSize;
    stagingInfo.usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    stagingInfo.memoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    BoundBuffer stagingBuffer = Buffer::create(stagingInfo);

    void *data;
    vulkanInfo.vkMapMemory(stagingBuffer.deviceMemory, 0, VertexBufferSize, 0, &data);
    memcpy(data, &vertices, sizeof(vertices));

    Buffer::CreateInfo bufferInfo;
    bufferInfo.vulkanInfo = &vulkanInfo;
    bufferInfo.size = VertexBufferSize;
    bufferInfo.usageFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.memoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    vertexBuffer = Buffer::create(bufferInfo);

    VkCommandBuffer commandBuffer = vulkanInfo.beginSingleTimeCommands();
    VkBufferCopy copyRegion = {};
    copyRegion.size = VertexBufferSize;
    vulkanInfo.vkCmdCopyBuffer(commandBuffer, stagingBuffer.buffer, vertexBuffer.buffer, 1, &copyRegion);
    vulkanInfo.endSingleTimeCommands(commandBuffer);

    vulkanInfo.vkUnmapMemory(stagingBuffer.deviceMemory);
}

void MapRenderer::createIndexBuffer()
{
    std::array<uint16_t, 6> indices{0, 1, 3, 3, 1, 2};
    Buffer::CreateInfo stagingInfo;
    stagingInfo.vulkanInfo = &vulkanInfo;
    stagingInfo.size = IndexBufferSize;
    stagingInfo.usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    stagingInfo.memoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    BoundBuffer indexStagingBuffer = Buffer::create(stagingInfo);

    void *data;
    vulkanInfo.vkMapMemory(indexStagingBuffer.deviceMemory, 0, IndexBufferSize, 0, &data);
    memcpy(data, &indices, sizeof(indices));

    Buffer::CreateInfo bufferInfo;
    bufferInfo.vulkanInfo = &vulkanInfo;
    bufferInfo.size = IndexBufferSize;
    bufferInfo.usageFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    bufferInfo.memoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    indexBuffer = Buffer::create(bufferInfo);

    VkCommandBuffer commandBuffer = vulkanInfo.beginSingleTimeCommands();
    VkBufferCopy copyRegion = {};
    copyRegion.size = IndexBufferSize;
    vulkanInfo.vkCmdCopyBuffer(commandBuffer, indexStagingBuffer.buffer, indexBuffer.buffer, 1, &copyRegion);
    vulkanInfo.endSingleTimeCommands(commandBuffer);

    vulkanInfo.vkUnmapMemory(indexStagingBuffer.deviceMemory);
}

VkShaderModule MapRenderer::createShaderModule(const std::vector<uint8_t> &code)
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

    VkShaderModule shaderModule;
    if (vulkanInfo.vkCreateShaderModule(&createInfo, nullptr, &shaderModule) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create shader module!");
    }

    return shaderModule;
}

VkCommandBuffer MapRenderer::beginSingleTimeCommands(VulkanInfo *info)
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = info->graphicsCommandPool();
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    info->vkAllocateCommandBuffers(&allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    info->vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

uint32_t MapRenderer::findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vulkanInfo.vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    throw std::runtime_error("Failed to find suitable memory type!");
}

/**
 * >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
 * >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
 * >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
 * VulkanTexture
 * >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
 * >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
 * >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
 **/
VulkanTexture::VulkanTexture()
{
}

VulkanTexture::~VulkanTexture()
{
    if (hasResources())
    {
        releaseResources();
    }
}

void VulkanTexture::initResources(const Texture &texture, VulkanInfo &vulkanInfo, const VulkanTexture::Descriptor descriptor)
{
    unused = false;

    width = texture.width();
    height = texture.height();
    uint32_t sizeInBytes = texture.sizeInBytes();

    this->vulkanInfo = &vulkanInfo;
    Buffer::CreateInfo bufferInfo;
    bufferInfo.vulkanInfo = &vulkanInfo;
    bufferInfo.size = sizeInBytes;
    bufferInfo.usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.memoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    auto stagingBuffer = Buffer::create(bufferInfo);

    Buffer::copyToMemory(&vulkanInfo, stagingBuffer.deviceMemory, texture.pixels().data(), sizeInBytes);

    createImage(
        ColorFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    vulkanInfo.transitionImageLayout(textureImage,
                                     VK_IMAGE_LAYOUT_UNDEFINED,
                                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    copyStagingBufferToImage(stagingBuffer.buffer);

    vulkanInfo.transitionImageLayout(textureImage,
                                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    _descriptorSet = createDescriptorSet(descriptor);
}

void VulkanTexture::releaseResources()
{
    DEBUG_ASSERT(hasResources(), "Tried to release resources, but there are no resources in the Texture Resource.");

    vulkanInfo->vkDestroyImage(textureImage, nullptr);
    vulkanInfo->vkFreeMemory(textureImageMemory, nullptr);

    textureImage = VK_NULL_HANDLE;
    _descriptorSet = VK_NULL_HANDLE;
    textureImageMemory = VK_NULL_HANDLE;

    vulkanInfo = nullptr;

    unused = true;
}

void VulkanTexture::createImage(
    VkFormat format,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkMemoryPropertyFlags properties)
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vulkanInfo->vkCreateImage(&imageInfo, nullptr, &textureImage) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vulkanInfo->vkGetImageMemoryRequirements(textureImage, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = vulkanInfo->findMemoryType(vulkanInfo->physicalDevice(), memRequirements.memoryTypeBits, properties);

    if (vulkanInfo->vkAllocateMemory(&allocInfo, nullptr, &textureImageMemory) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vulkanInfo->vkBindImageMemory(textureImage, textureImageMemory, 0);
}

void VulkanTexture::copyStagingBufferToImage(VkBuffer stagingBuffer)
{
    VkCommandBuffer commandBuffer = vulkanInfo->beginSingleTimeCommands();

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};

    vulkanInfo->vkCmdCopyBufferToImage(
        commandBuffer,
        stagingBuffer,
        textureImage,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region);

    vulkanInfo->endSingleTimeCommands(commandBuffer);
}

VkSampler VulkanTexture::createSampler()
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;

    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;

    VkSampler sampler;
    if (vulkanInfo->vkCreateSampler(&samplerInfo, nullptr, &sampler) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create texture sampler!");
    }

    return sampler;
}

VkDescriptorSet VulkanTexture::createDescriptorSet(VulkanTexture::Descriptor descriptor)
{
    VkImageView imageView = createImageView(textureImage, ColorFormat);
    VkSampler sampler = createSampler();

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptor.pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descriptor.layout;

    if (vulkanInfo->vkAllocateDescriptorSets(&allocInfo, &_descriptorSet) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate texture descriptor set");
    }

    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = imageView;
    imageInfo.sampler = sampler;

    VkWriteDescriptorSet descriptorWrites = {};
    descriptorWrites.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites.dstSet = _descriptorSet;
    descriptorWrites.dstBinding = 0;
    descriptorWrites.dstArrayElement = 0;
    descriptorWrites.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites.descriptorCount = 1;
    descriptorWrites.pImageInfo = &imageInfo;

    vulkanInfo->vkUpdateDescriptorSets(1, &descriptorWrites, 0, nullptr);

    return _descriptorSet;
}

VkImageView VulkanTexture::createImageView(VkImage image, VkFormat format)
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vulkanInfo->vkCreateImageView(&viewInfo, nullptr, &imageView) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create texture image view!");
    }

    return imageView;
}

RectangleDrawInfo::RectangleDrawInfo(SolidColor color, SolidColor borderColor, WorldPosition position, int width, int height, float opacity)
    : color(color), borderColor(borderColor), position(position), width(width), height(height), opacity(opacity) {}

RectangleDrawInfo::RectangleDrawInfo() {}

RectangleDrawInfo RectangleDrawInfo::border(SolidColor color, WorldPosition position, int width, int height, float opacity)
{
    RectangleDrawInfo info;
    info.borderColor = color;
    info.position = position;
    info.width = width;
    info.height = height;
    info.opacity = opacity;
    return info;
}

RectangleDrawInfo RectangleDrawInfo::border(SolidColor color, WorldPosition position, int size, float opacity)
{
    return border(color, position, size, size, opacity);
}

RectangleDrawInfo RectangleDrawInfo::solid(SolidColor color, WorldPosition position, int width, int height, float opacity)
{
    RectangleDrawInfo info;
    info.color = color;
    info.position = position;
    info.width = width;
    info.height = height;
    info.opacity = opacity;
    return info;
}

RectangleDrawInfo RectangleDrawInfo::solid(SolidColor color, WorldPosition position, int size, float opacity)
{
    return solid(color, position, size, size, opacity);
}
