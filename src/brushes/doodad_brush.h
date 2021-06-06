#pragma once

#include <memory>
#include <optional>
#include <unordered_set>
#include <vector>

#include "brush.h"

struct Position;
class MapView;

class DoodadBrush final : public Brush
{

  public:
    enum class EntryType
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
        int8_t dx;
        int8_t dy;
        int8_t dz;

        std::vector<uint32_t> serverIds;
    };

    struct DoodadComposite final : public DoodadEntry
    {
        DoodadComposite(std::vector<CompositeTile> &&tiles, uint32_t weight);

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

    DoodadBrush(uint32_t id, const std::string &name, DoodadAlternative &&alternative, uint32_t iconServerId);
    DoodadBrush(uint32_t id, const std::string &name, std::vector<DoodadAlternative> &&alternatives, uint32_t iconServerId);

    BrushResource brushResource() const override;

    void apply(MapView &mapView, const Position &position) override;
    uint32_t iconServerId() const;
    bool erasesItem(uint32_t serverId) const override;
    BrushType type() const override;
    const std::string getDisplayId() const override;

    uint32_t brushId() const noexcept;

    std::vector<ThingDrawInfo> getPreviewTextureInfo() const override;

  private:
    void initialize();
    std::vector<ItemPreviewInfo> sampleGroup();

    std::unordered_set<uint32_t> serverIds;
    std::vector<DoodadAlternative> alternatives;

    uint8_t alternateIndex = 0;

    uint32_t id;
    uint32_t _iconServerId;

    std::vector<ItemPreviewInfo> _nextGroup;

    // Info that lets the GUI know how to draw the brush
    BrushResource _brushResource;
};