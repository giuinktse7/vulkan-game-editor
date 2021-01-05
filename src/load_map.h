#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

#include "item.h"
#include "map.h"
#include "otbm.h"
#include "version.h"

class LoadBuffer;
namespace OTBM
{
    class Deserializer;
}

class LoadMap
{
  public:
    static std::variant<Map, std::string> loadMap(std::filesystem::path &path);

  private:
    static bool isValidOTBMVersion(uint32_t value);

    static void logWarning(std::string message);
    static std::variant<Map, std::string> error(std::string message);
    static std::optional<std::string> deserializeTileArea(LoadBuffer &buffer, OTBM::Deserializer &deserializer, Map &map);
    static std::optional<std::string> deserializeTowns(LoadBuffer &buffer, OTBM::Deserializer &deserializer, Map &map);

    static Item deserializeItem(LoadBuffer &buffer);
};

class LoadBuffer
{
  public:
    LoadBuffer(std::vector<uint8_t> &&buffer);

    uint8_t peek() const;
    uint8_t nextU8();
    uint16_t nextU16();
    uint32_t nextU32();
    uint64_t nextU64();
    Position readPosition();
    OTBM::Node_t readNodeStart();
    bool readEnd();
    void skip(size_t amount);
    void skipNode(bool remainAtEnd = false);

    std::string nextString(size_t size);
    std::string nextString();
    std::string nextLongString();

  private:
    std::vector<uint8_t>::iterator cursor;
    std::vector<uint8_t> buffer;
};

namespace OTBM
{
    class Deserializer
    {
      public:
        static std::unique_ptr<Deserializer> create(OTBMVersion version, LoadBuffer &buffer);

        virtual Item deserializeCompactItem() = 0;

        virtual Item deserializeItem() = 0;
        virtual std::optional<std::string> deserializeItemAttributes(Item &item) = 0;
    };

    class OTBM1Deserializer : public Deserializer
    {
      public:
        OTBM1Deserializer(LoadBuffer &buffer)
            : buffer(buffer) {}

        Item deserializeCompactItem() override;
        Item deserializeItem() override;
        std::optional<std::string> deserializeItemAttributes(Item &item) override;

      protected:
        LoadBuffer &buffer;
        void logWarning(std::string message);
    };

    class OTBM2Deserializer : public OTBM1Deserializer
    {
      public:
        OTBM2Deserializer(LoadBuffer &buffer)
            : OTBM1Deserializer(buffer) {}
    };

    class OTBM3Deserializer : public OTBM2Deserializer
    {
      public:
        OTBM3Deserializer(LoadBuffer &buffer)
            : OTBM2Deserializer(buffer) {}
    };

    class OTBM4Deserializer : public OTBM3Deserializer
    {
      public:
        OTBM4Deserializer(LoadBuffer &buffer)
            : OTBM3Deserializer(buffer) {}

        std::optional<std::string> deserializeItemAttributes(Item &item) override;
    };
} // namespace OTBM