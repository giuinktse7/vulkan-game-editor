#include "otbm.h"

namespace
{
    vme_unordered_set<OTBM::Node_t> nodeTypes{
        OTBM::Node_t::Root,
        OTBM::Node_t::Rootv1,
        OTBM::Node_t::MapData,
        OTBM::Node_t::ItemDef,
        OTBM::Node_t::TileArea,
        OTBM::Node_t::Tile,
        OTBM::Node_t::Item,
        OTBM::Node_t::TileSquare,
        OTBM::Node_t::TileRef,
        OTBM::Node_t::Spawns,
        OTBM::Node_t::SpawnArea,
        OTBM::Node_t::Monster,
        OTBM::Node_t::Towns,
        OTBM::Node_t::Town,
        OTBM::Node_t::Housetile,
        OTBM::Node_t::Waypoints,
        OTBM::Node_t::Waypoint,
    };
}

bool OTBM::isNodeType(uint8_t value)
{
    return nodeTypes.find(static_cast<OTBM::Node_t>(value)) != nodeTypes.end();
}