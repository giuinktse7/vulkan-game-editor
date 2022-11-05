#pragma once

#include <array>
#include <limits>
#include <memory>
#include <optional>
#include <string>

#include "../tile_cover.h"
#include "border_brush_variation.h"
#include "brush.h"

struct Position;
class MapView;
class Map;
class GroundBrush;
class RawBrush;
class GeneralBorderBrush;
struct TileBorderBlock;

enum class BorderBrushVariationType;

enum class BorderStackBehavior
{
    Default,
    Clear,
    FullGround,
};

/*
{
    "whenBorder": {
        "borderId": "sea_border",
        "case": {
            "selfEdge": "s",
            "borderEdge": "csw",
            "actions": [
                {
                    "type": "replace",
                    "target": "other",
                    "value": 4657
                }
            ]
        }
    }
}
*/

struct BorderRuleAction
{
    enum class Type
    {
        SetFull,
        Replace
    };

    BorderRuleAction(Type type)
        : type(type) {}

    Type type;
};

struct RuleAction
{
    virtual void apply(MapView &mapView, BorderBrush *brush, const Position &position);
};

struct SetFullAction : public BorderRuleAction
{
    SetFullAction(bool setSelf)
        : BorderRuleAction(Type::SetFull), setSelf(setSelf) {}

    SetFullAction()
        : BorderRuleAction(Type::SetFull) {}

    void apply(MapView &mapView, const Position &position, GroundBrush *groundBrush);

    bool setSelf = true;
};

struct ReplaceAction : public BorderRuleAction
{
    ReplaceAction(bool replaceSelf, uint32_t serverId)
        : BorderRuleAction(Type::Replace), replaceSelf(replaceSelf), serverId(serverId) {}

    ReplaceAction()
        : BorderRuleAction(Type::Replace) {}

    void apply(MapView &mapView, const Position &position, uint32_t oldServerId);

    bool replaceSelf = true;
    uint32_t serverId;
};

struct WhenBorderRule
{
    struct Case
    {
        BorderType selfEdge;
        BorderType borderEdge;
        // std::vector<std::unique_ptr<RuleAction *>> actions;
        std::unique_ptr<BorderRuleAction> action;
    };

    std::optional<TileCover> check(const TileBorderBlock &block) const;

    std::string borderId;
    std::vector<Case> cases;
    std::vector<std::unique_ptr<BorderRuleAction>> actions;
};

struct BorderRule
{
    struct Condition
    {
        bool check(const TileBorderBlock &block) const;
        std::string borderId;
        std::optional<TileCover> edges;
    };

    void apply(MapView &mapView, BorderBrush *brush, const Position &position) const;

    std::vector<Condition> conditions;
    std::string action;
};

class BorderBrush final : public Brush
{
  public:
    BorderBrush(std::string id, const std::string &name, std::array<uint32_t, 12> borderIds);
    BorderBrush(std::string id, const std::string &name, std::array<uint32_t, 12> borderIds, GroundBrush *centerBrush);
    BorderBrush(std::string id, const std::string &name, std::array<uint32_t, 12> borderIds, RawBrush *centerBrush);

    void fixBorders(MapView &mapView, const Position &position, BorderNeighborMap &neighbors);

    void apply(MapView &mapView, const Position &position) override;
    void erase(MapView &mapView, const Position &position) override;

    bool erasesItem(uint32_t serverId) const override;
    BrushType type() const override;
    const std::string getDisplayId() const override;
    std::vector<ThingDrawInfo> getPreviewTextureInfo(int variation) const override;

    void apply(MapView &mapView, const Position &position, BorderType borderType);

    void quadrantChanged(MapView &mapView, const Position &position, TileQuadrant prevQuadrant, TileQuadrant currQuadrant);

    uint32_t iconServerId() const;

    void setIconServerId(uint32_t serverId);

    void setCenterGroundId(const std::string &id);

    uint32_t preferredZOrder() const;

    std::string brushId() const noexcept;

    bool includes(uint32_t serverId) const;
    bool hasBorder(uint32_t serverId) const;

    const std::vector<uint32_t> &serverIds() const;

    std::optional<uint32_t> getServerId(BorderType borderType) const noexcept;

    GroundBrush *centerBrush() const;

    BorderType getBorderType(uint32_t serverId) const;
    TileCover getTileCover(uint32_t serverId) const;

    TileQuadrant getNeighborQuadrant(int dx, int dy);

    static void setBrushVariation(BorderBrushVariationType brushVariationType);

    inline const BorderData &getBorderData() const noexcept;
    inline BorderData &getBorderData() noexcept;

    BorderStackBehavior stackBehavior() const noexcept;
    void setStackBehavior(BorderStackBehavior behavior) noexcept;

    std::vector<WhenBorderRule> rules;

  private:
    friend class GeneralBorderBrush;
    friend class DetailedBorderBrush;

    void quadrantChanged(MapView &mapView, const Position &position, BorderNeighborMap &neighbors, TileQuadrant prevQuadrant, TileQuadrant currQuadrant);

    bool presentAt(const Map &map, const Position position) const;
    uint32_t borderItemAt(const Map &map, const Position position) const;

    void initialize();

    void updateCenter(BorderNeighborMap &neighbors);

    void fixBordersAtOffset(MapView &mapView, const Position &position, BorderNeighborMap &neighbors, int x, int y);
    void fixBorderEdgeCases(int x, int y, TileCover &cover, const BorderNeighborMap &neighbors);

    static void quadrantChangeInFirstTile(TileCover cover, TileCover &nextCover, TileQuadrant prevQuadrant, TileQuadrant currQuadrant);

    static TileBorderInfo tileInfo;

    static BorderExpandDirection currDir;
    static std::optional<BorderExpandDirection> prevDir;

    std::vector<uint32_t> sortedServerIds;

    std::string id;
    uint32_t _iconServerId;

    mutable uint32_t zOrder = std::numeric_limits<uint32_t>::max();

    BorderData borderData;

    static BorderBrushVariation *brushVariation;

    BorderStackBehavior _stackBehavior = BorderStackBehavior::Default;
};

inline const BorderData &BorderBrush::getBorderData() const noexcept
{
    return borderData;
}

inline BorderData &BorderBrush::getBorderData() noexcept
{
    return borderData;
}