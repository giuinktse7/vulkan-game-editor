#pragma once

#include <fstream>
#include <unordered_map>
#include <vector>

#include "item.h"
#include "item_attribute.h"
#include "map.h"
#include "otbm.h"
#include "util.h"

/*
Small wrapper for a buffer that is written to when saving an OTBM map.
*/
class SaveBuffer
{
public:
	SaveBuffer(std::ofstream &stream);

	void writeU8(uint8_t value);
	inline void writeU8(OTBM::NodeAttribute value);
	inline void writeU8(OTBM::AttributeTypeId value);
	void writeU16(uint16_t value);
	void writeU32(uint32_t value);
	void writeU64(uint64_t value);
	void writeString(const std::string &s);
	void writeLongString(const std::string &s);
	void writeRawString(const std::string &s);

	void startNode(OTBM::Node_t value);
	void endNode();

	inline void writeToken(OTBM::Token token);
	inline void writeNodeType(OTBM::Node_t token);

	void finish();

private:
	std::ostream &stream;
	std::vector<uint8_t> buffer;

	size_t maxBufferSize;

	void writeBytes(const uint8_t *start, size_t amount);
	void flushToFile();
};

namespace SaveMap
{
	void saveMap(const Map &map);

	class Serializer
	{
	public:
		Serializer(SaveBuffer &buffer, const MapVersion &mapVersion)
				: mapVersion(mapVersion), buffer(buffer) {}
		void serializeItem(const Item &item);
		void serializeItemAttributes(const Item &item);
		void serializeItemAttributeMap(const vme_unordered_map<ItemAttribute_t, ItemAttribute> &attributes);
		void serializeItemAttribute(const ItemAttribute &attribute);

	private:
		MapVersion mapVersion;
		SaveBuffer &buffer;
	};

} // namespace SaveMap

inline void SaveBuffer::writeU8(OTBM::NodeAttribute value)
{
	writeU8(static_cast<uint8_t>(to_underlying(value)));
}

inline void SaveBuffer::writeU8(OTBM::AttributeTypeId value)
{
	writeU8(static_cast<uint8_t>(to_underlying(value)));
}

inline void SaveBuffer::writeToken(OTBM::Token token)
{
	buffer.emplace_back(static_cast<uint8_t>(to_underlying(token)));
}

inline void SaveBuffer::writeNodeType(OTBM::Node_t nodeType)
{
	buffer.emplace_back(static_cast<uint8_t>(to_underlying(nodeType)));
}
