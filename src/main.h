#pragma once

#include <QtQuick/QQuickView>

#include <filesystem>
#include <optional>
#include <stdarg.h>
#include <string>
#include <utility>
#include <vector>

// TemporaryTest includes
#include <memory>

#include "common/map.h"
#include "common/octree.h"
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

class MainApp
{
  public:
    int start(int argc, char **argv);

  private:
    std::unique_ptr<QQuickView> rootView;
};

void overlay();
void testApplyAtlasTemplate();
std::pair<int, int> offsetFromSpriteId(TextureAtlas *atlas, uint32_t spriteId);