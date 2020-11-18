#pragma once

#include <glm/glm.hpp>

#include <filesystem>
#include <memory>
#include <variant>

#include "../const.h"
#include "compression.h"
#include "texture.h"

struct TextureAtlas;

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

struct TextureAtlas
{
public:
	TextureAtlas(uint32_t id, LZMACompressedBuffer &&buffer, uint32_t width, uint32_t height, uint32_t firstSpriteId, uint32_t lastSpriteId, SpriteLayout spriteLayout, std::filesystem::path sourceFile);

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

	glm::vec4 getFragmentBounds(const TextureWindow window) const;
	const TextureWindow getTextureWindow(uint32_t spriteId, TextureInfo::CoordinateType coordinateType = TextureInfo::CoordinateType::Normalized) const;

	bool isCompressed() const;

	inline uint32_t sizeInBytes() const
	{
		return width * height * 4;
	}

	void decompressTexture();
	Texture *getTexture();
	Texture &getOrCreateTexture();

	/*
		The ID is generated based on when the texture atlas was added to the texture atlases list.
		The Texture atlas ID's begin at 0 and end at (amountOfAtlases - 1).
	*/
	inline uint32_t id() const;

private:
	uint32_t _id;

	std::variant<LZMACompressedBuffer, Texture> texture;

	void validateBmp(std::vector<uint8_t> buffer);
};

inline uint32_t TextureAtlas::id() const
{
	return _id;
}
