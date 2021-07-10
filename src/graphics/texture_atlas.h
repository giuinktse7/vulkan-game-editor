#pragma once

#include <filesystem>
#include <memory>
#include <tuple>
#include <utility>
#include <variant>

#include "../const.h"
#include "../outfit.h"
#include "compression.h"
#include "texture.h"

struct TextureAtlas;
struct WorldPosition;

/*
	The width and height of a texture atlas in pixels
*/
constexpr struct
{
    uint16_t width = 384;
    uint16_t height = 384;
} TextureAtlasSize;

struct TextureInfo
{
    enum class CoordinateType
    {
        Normalized,
        Unnormalized
    };

    TextureAtlas *atlas;
    TextureWindow window;

    const Texture &getTexture() const;
    const Texture &getTexture(uint32_t textureVariationId) const;
};

struct DrawOffset
{
    int x;
    int y;
};

struct LZMACompressedBuffer
{
    std::vector<uint8_t> buffer;
};

struct TextureAtlasVariation
{
    TextureAtlasVariation(uint32_t id, Texture texture);

    uint32_t id;
    Texture texture;
};

struct TextureAtlas
{
  public:
    TextureAtlas(LZMACompressedBuffer &&buffer, uint32_t width, uint32_t height, uint32_t firstSpriteId, uint32_t lastSpriteId, SpriteLayout spriteLayout, std::filesystem::path sourceFile);

    std::filesystem::path sourceFile;

    uint32_t width;
    uint32_t height;

    uint32_t firstSpriteId;
    uint32_t lastSpriteId;

    uint32_t rows;
    uint32_t columns;

    uint32_t spriteWidth;
    uint32_t spriteHeight;

    DrawOffset drawOffset;

    TextureAtlasVariation *getVariation(uint32_t id);

    std::pair<int, int> textureOffset(uint32_t spriteId);

    bool hasColorVariation(uint32_t variationId) const;

    void overlay(TextureAtlas *templateAtlas, uint32_t variationId, uint32_t templateSpriteId, uint32_t targetSpriteId, Outfit::Look look);

    glm::vec4 getFragmentBounds(const TextureWindow window) const;
    const TextureWindow getTextureWindow(uint32_t spriteId, uint32_t variationId, TextureInfo::CoordinateType coordinateType = TextureInfo::CoordinateType::Normalized) const;
    const TextureWindow getTextureWindow(uint32_t spriteId, TextureInfo::CoordinateType coordinateType = TextureInfo::CoordinateType::Normalized) const;
    const TextureWindow getTextureWindowTopLeft(uint32_t spriteId) const;
    const std::pair<TextureWindow, TextureWindow> getTextureWindowTopLeftBottomRight(uint32_t spriteId) const;
    const std::tuple<TextureWindow, TextureWindow, TextureWindow> getTextureWindowTopRightBottomRightBottomLeft(uint32_t spriteId) const;

    bool isCompressed() const;

    inline uint32_t sizeInBytes() const
    {
        return width * height * 4;
    }

    WorldPosition worldPosOffset() const noexcept;

    void decompressTexture() const;
    Texture *getTexture();
    Texture &getOrCreateTexture();
    Texture &getTexture(uint32_t variationId);

  private:
    Texture &getOrCreateTexture() const;

    struct InternalTextureInfo
    {
        float x;
        float y;
        float width;
        float height;
    };

    InternalTextureInfo internalTextureInfoNormalized(uint32_t spriteId) const;
    void validateBmp(std::vector<uint8_t> &decompressed) const;

    mutable std::variant<LZMACompressedBuffer, Texture> texture;

    mutable std::unique_ptr<std::vector<TextureAtlasVariation>> variations;
};
