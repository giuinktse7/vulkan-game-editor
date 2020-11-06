#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

#define VME_TEST

#include "../src/ecs/ecs.h"
#include "../src/ecs/item_animation.h"
#include "../src/graphics/appearances.h"
#include "../src/items.h"

int main(int argc, char *argv[])
{
  g_ecs.registerComponent<ItemAnimationComponent>();
  g_ecs.registerSystem<ItemAnimationSystem>();

  Appearances::loadTextureAtlases("data/catalog-content.json");
  Appearances::loadAppearanceData("data/appearances.dat");

  Items::loadFromOtb("data/items.otb");
  Items::loadFromXml("data/items.xml");

  int result = Catch::Session().run(argc, argv);
  return result;
}