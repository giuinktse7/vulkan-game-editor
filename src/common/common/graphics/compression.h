#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "../debug.h"
#include "../definitions.h"
#include "lzma.h"

#include "../vendor/lzma/LzmaLib.h"

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
    static std::vector<uint8_t> decompressLibLzma(std::vector<uint8_t> &&buffer);

    static std::vector<uint8_t> decompressRelease(std::vector<uint8_t> &&buffer);
};

inline std::vector<uint8_t> LZMA::decompress(std::vector<uint8_t> &&inBuffer)
{
    // #ifdef _DEBUG_VME
    return decompressLibLzma(std::move(inBuffer));
    // #else
    // return decompressRelease(std::move(inBuffer));
    // #endif
}