#pragma once

#include "gui/mainwindow.h"
#include "gui/vulkan_window.h"
#include <QApplication>
#include <QVulkanInstance>
#include <filesystem>
#include <optional>
#include <stdarg.h>
#include <string>
#include <utility>
#include <vector>

// TemporaryTest includes
#include <memory>

#include "map.h"
#include "octree.h"
// End TemporaryTest includes

namespace MainUtils
{
    void printOutfitAtlases(std::vector<uint32_t> outfitIds);
}

namespace TemporaryTest
{
    void loadAllTexturesIntoMemory();

    void addChunk(Position from, vme::octree::Tree &tree);
    void testOctree();

    std::shared_ptr<Map> makeTestMap1();
    std::shared_ptr<Map> makeTestMap2();
} // namespace TemporaryTest

void testApplyAtlasTemplate();
std::pair<int, int> offsetFromSpriteId(TextureAtlas *atlas, uint32_t spriteId);
void applyTemplate(int spriteX, int spriteY, int templateX, int templateY, int width, int height, int atlasWidth, std::vector<uint8_t> &pixels, const Outfit &outfit);