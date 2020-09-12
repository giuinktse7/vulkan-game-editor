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
	TextureAtlas *atlas;
	TextureWindow window;
};

struct DrawOffset
{
	int x;
	int y;
};

struct TextureAtlas
{
private:
	using CompressedBytes = std::vector<uint8_t>;

public:
	TextureAtlas(uint32_t id, CompressedBytes &&buffer, uint32_t width, uint32_t height, uint32_t firstSpriteId, SpriteLayout spriteLayout, std::filesystem::path sourceFile);

	uint32_t id;

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

	const TextureWindow getTextureWindow(uint32_t spriteId) const;
	const TextureWindow getTextureWindowUnNormalized(uint32_t spriteId) const;

	bool isCompressed() const;

	void decompressTexture();
	Texture *getTexture();
	Texture &getOrCreateTexture();

	VkDescriptorSet getDescriptorSet()
	{
		if (std::holds_alternative<Texture>(texture))
		{
			return std::get<Texture>(texture).getDescriptorSet();
		}
		else
		{
			return nullptr;
		}
	}

private:
	std::variant<CompressedBytes, Texture> texture;

	void validateBmp(std::vector<uint8_t> buffer);
};
