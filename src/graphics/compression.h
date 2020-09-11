#pragma once

#include <string>
#include <vector>
#include <filesystem>

#include "lzma.h"
#include "../debug.h"

#include "../lzma/LzmaLib.h"

struct LZMA
{
  static std::vector<uint8_t> decompressFile(const std::filesystem::path &filepath);

  static std::vector<uint8_t> decompress(std::vector<uint8_t> &&buffer);

  /*
    Runs around 1 second faster in debug. This and runDebugDecompression are the only two methods that depend on
    liblzma. decompressRelease can be run in debug mode too, but is ~1 second slower.
    If decompressRelease is used in debug mode too, the libzlma dependency can be removed.
    Should perhaps be done in the future.
    NOTE: In Release mode, decompressRelease is ~1 second faster.
  */
  static std::vector<uint8_t> decompressDebug(std::vector<uint8_t> &&buffer);

  static std::vector<uint8_t> decompressRelease(std::vector<uint8_t> &&buffer);

private:
  static std::vector<uint8_t> runDebugDecompression(const std::vector<uint8_t> &in, lzma_options_lzma &options);
};

inline std::vector<uint8_t> LZMA::decompress(std::vector<uint8_t> &&inBuffer)
{
#ifdef __DEBUG__VME
  return decompressDebug(std::move(inBuffer));
#else
  return decompressRelease(std::move(inBuffer));
#endif
}