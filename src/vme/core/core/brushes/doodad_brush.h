#pragma once

#include <memory>
#include <unordered_set>
#include <vector>

#include "brush.h"

struct Position;
class MapView;

class DoodadBrush final : public Brush
{

  public:
    enum class ReplaceBehavior
    {
        // Block brush from placing if the tile already has a doodad of this type
        Block,

        // Replace existing doodad of this type on tile
        Replace,

        // Add to tile regardless of whether this doodad already exists on the tile.
        Stack
    };

    static ReplaceBehavior parseReplaceBehavior(std::string raw);

    enum class EntryType : std::uint8_t
    {
        Single,
        Composite,
    };

    struct DoodadEntry
    {
        DoodadEntry(uint32_t weight, EntryType type)
            : weight(weight), type(type) {}

        // Non-copyable
        DoodadEntry(const DoodadEntry &other) = delete;
        DoodadEntry operator&=(const DoodadEntry &other) = delete;

        DoodadEntry(DoodadEntry &&other) = default;
        DoodadEntry &operator=(DoodadEntry &&other) = default;

        virtual ~DoodadEntry() = default;

        uint32_t weight;
        EntryType type;
    };

    struct DoodadSingle final : public DoodadEntry
    {
        DoodadSingle(uint32_t serverId, uint32_t weight);
        uint32_t serverId;
    };

    struct CompositeTile
    {
        Position relativePosition() const;

        int8_t dx = 0;
        int8_t dy = 0;
        int8_t dz = 0;

        uint32_t serverId = 0;
    };

    struct DoodadComposite final : public DoodadEntry
    {
        DoodadComposite(std::vector<CompositeTile> &&tiles, uint32_t weight);

        Position relativePosition(uint32_t serverId);

        std::vector<CompositeTile> tiles;
    };

    class DoodadAlternative
    {
      public:
        DoodadAlternative(std::vector<std::unique_ptr<DoodadEntry>> &&choices);

        DoodadAlternative(DoodadAlternative &&other) = default;
        DoodadAlternative &operator=(DoodadAlternative &&other) = default;

        // The brush name is only used for better error messages
        std::vector<ItemPreviewInfo> sample(const std::string &brushName) const;

      private:
        friend class DoodadBrush;
        std::vector<std::unique_ptr<DoodadEntry>> choices;

        uint32_t totalWeight = 0;
    };

    DoodadBrush(std::string id, const std::string &name, DoodadAlternative &&alternative, uint32_t iconServerId);
    DoodadBrush(std::string id, const std::string &name, std::vector<DoodadAlternative> &&alternatives, uint32_t iconServerId);

    void apply(MapView &mapView, const Position &position) override;
    void erase(MapView &mapView, const Position &position) override;

    uint32_t iconServerId() const;
    bool erasesItem(uint32_t serverId) const override;
    BrushType type() const override;
    std::string getDisplayId() const override;

    const std::string &id() const noexcept;

    std::vector<ThingDrawInfo> getPreviewTextureInfo(int variation) const override;
    void updatePreview(int variation) override;
    int variationCount() const override;

    ReplaceBehavior replaceBehavior = ReplaceBehavior::Block;

    std::unordered_set<uint32_t> serverIds() const;

  private:
    void initialize();
    bool prepareApply(MapView &mapView, const Position &position);

    vme_unordered_map<uint32_t, DoodadComposite *> composites;

    std::unordered_set<uint32_t> _serverIds;
    std::vector<DoodadAlternative> alternatives;

    /**
     * Thickness in [0, 1].
     * 0: Brush never applies.
     * 1: Brush always applies.
     */
    float thickness;

    std::string _id;
    uint32_t _iconServerId;

    bool replace = false;

    std::vector<ItemPreviewInfo> _buffer;

    int alternateIndex = 0;
};