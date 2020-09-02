#pragma once

#include <exception>
#include <string>
#include <vector>
#include <array>
#include <algorithm>
#include <iterator>


class PropStream;

namespace OTB
{

  struct LightInfo
  {
    uint16_t level;
    uint16_t color;
  };

  enum class RootAttributes : uint8_t
  {
    Version = 0x01,
  };

  struct VersionInfo
  {
    uint32_t majorVersion = 0xFFFFFFFF;
    int minorVersion = 0x0;
    uint32_t buildNumber;
    uint8_t CSDVersion[128];
  };

  using Identifier = std::array<char, 4>;
  using ByteIterator = std::vector<uint8_t>::iterator;

  struct Node
  {

    std::vector<Node> children;
    ByteIterator propsBegin;
    ByteIterator propsEnd;
    uint8_t type;
    enum NodeChar : uint8_t
    {
      ESCAPE = 0xFD,
      START = 0xFE,
      END = 0xFF,
    };
  };

  struct LoadError : std::exception
  {
    const char *what() const noexcept override = 0;
  };

  struct InvalidOTBFormat final : LoadError
  {
    const char *what() const noexcept override
    {
      return "Invalid OTBM file format";
    }
  };

  class Loader
  {
  public:
    Loader(const std::string &fileName, const Identifier &acceptedIdentifier);
    bool getProps(const Node &node, PropStream &props);
    const Node &parseTree();

  private:
    std::vector<uint8_t> fileBuffer;
    Node root;
    std::vector<char> propBuffer;
  };
} // namespace OTB

class PropStream
{
public:
  void init(const char *a, size_t size)
  {
    cursor = a;
    end = a + size;
  }

  size_t size() const
  {
    return end - cursor;
  }

  template <typename T>
  bool read(T &ret)
  {
    if (size() < sizeof(T))
    {
      return false;
    }

    memcpy(&ret, cursor, sizeof(T));
    cursor += sizeof(T);
    return true;
  }

  bool readString(std::string &ret)
  {
    uint16_t strLen;
    if (!read<uint16_t>(strLen))
    {
      return false;
    }

    if (size() < strLen)
    {
      return false;
    }

    char *str = new char[strLen + 1];
    memcpy(str, cursor, strLen);
    str[strLen] = 0;
    ret.assign(str, strLen);
    delete[] str;
    cursor += strLen;
    return true;
  }

  bool skip(size_t n)
  {
    if (size() < n)
    {
      return false;
    }

    cursor += n;
    return true;
  }

private:
  const char *cursor = nullptr;
  const char *end = nullptr;
};

class PropWriteStream
{
public:
  PropWriteStream() = default;

  // non-copyable
  PropWriteStream(const PropWriteStream &) = delete;
  PropWriteStream &operator=(const PropWriteStream &) = delete;

  const char *getStream(size_t &size) const
  {
    size = buffer.size();
    return buffer.data();
  }

  void clear()
  {
    buffer.clear();
  }

  template <typename T>
  void write(T add)
  {
    char *addr = reinterpret_cast<char *>(&add);
    std::copy(addr, addr + sizeof(T), std::back_inserter(buffer));
  }

  void writeString(const std::string &str)
  {
    size_t strLength = str.size();
    if (strLength > std::numeric_limits<uint16_t>::max())
    {
      write<uint16_t>(0);
      return;
    }

    write(static_cast<uint16_t>(strLength));
    std::copy(str.begin(), str.end(), std::back_inserter(buffer));
  }

private:
  std::vector<char> buffer;
};