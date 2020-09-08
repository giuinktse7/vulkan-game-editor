#include "compression.h"

#include <stdexcept>
#include "../file.h"
#include "../logger.h"
#include "../util.h"

#include "../lzma/LzmaLib.h"

#include <cassert>

#pragma warning(push)
#pragma warning(disable : 26812)

constexpr int UncompressedSizeLengthBytes = 8;
constexpr int LzmaHeaderSize = LZMA_PROPS_SIZE + UncompressedSizeLengthBytes;

constexpr size_t BytesInTextureAtlas = 12 * 12 * 32 * 32 * 4;

std::vector<uint8_t> LZMA::decompressFile(const std::filesystem::path &filepath)
{
    // std::cout << filepath.filename().string() << std::endl;
    auto file = File::read(filepath.string());
    return decompress(std::move(file));
}

std::vector<uint8_t> LZMA::decompressRelease(std::vector<uint8_t> &&srcVector)
{
    std::vector<uint8_t> result(BytesInTextureAtlas);
    auto srcBuffer = srcVector.data();

    // Skip Cipsoft specific headers
    size_t cursor = 0;
    while (srcBuffer[cursor++] == 0x00)
        ;
    cursor += 4;
    while ((srcBuffer[cursor++] & 0x80) == 0x80)
        ;

    /*
        LZMA data. The header consists of 13 bytes.
        [0, 4] - 5 bytes: LZMA Props data
        [5, 12] - 8 bytes: The uncompressed size of the data
    */
    uint8_t *lzmaBuffer = &srcBuffer[cursor];
    uint8_t *compressedBufferStart = lzmaBuffer + LzmaHeaderSize;

    // size_t uncompressedDataSize;
    // memcpy(&uncompressedDataSize, lzmaBuffer + LZMA_PROPS_SIZE, UncompressedSizeLengthBytes);

    size_t inSize = srcVector.size() - LzmaHeaderSize;
    size_t outSize = result.size();
    LzmaUncompress(result.data(), &outSize, compressedBufferStart, &inSize, lzmaBuffer, LZMA_PROPS_SIZE);

    return result;
}

// std::vector<uint8_t> removeLeading(std::vector<uint8_t> const &buffer, uint8_t c)
// {
//     auto end = buffer.end();

//     for (auto i = buffer.begin(); i != end; ++i)
//     {
//         if (*i != c)
//         {
//             return std::vector<uint8_t>(i, end);
//         }
//     }

//     // All characters were leading or the string is empty.
//     return std::vector<uint8_t>();
// }

std::vector<uint8_t> LZMA::decompressDebug(std::vector<uint8_t> &&inBuffer)
{
    /*
        The file can be padded with a number of NULL bytes (depending on LZMA file size)
    */
    auto buffer = util::sliceLeading<uint8_t>(inBuffer, 0);

    /*
        CIPSoft header, 32 bytes
        (CIPSoft specific) constant byte sequence: 70 0A FA 80 24
    */
    auto cursor = buffer.begin() + 5;

    // Skip to start of LZMA file
    uint8_t FLAG = 0x5d;
    while (!(*cursor == FLAG && *(cursor + 1) == 0x0))
        ++cursor;

    /*
        Parse the LZMA1 header (see https://en.wikipedia.org/wiki/Lempel%E2%80%93Ziv%E2%80%93Markov_chain_algorithm#LZMA_configuration)
    */

    char lclppb = *cursor++;

    int lc = lclppb % 9;
    int remainder = lclppb / 9;
    int lp = remainder % 5;
    int pb = remainder / 5;

    lzma_options_lzma options{};
    options.lc = lc;
    options.lp = lp;
    options.pb = pb;

    uint32_t dictionarySize = 0;
    for (uint8_t i = 0; i < 4; ++i)
    {
        dictionarySize += (*cursor) << (i * 8);
        cursor++;
    }

    options.dict_size = dictionarySize;

    /*
        These bytes should correspond to the uncompressed file size, but CIP writes
        the compressed size. We just skip them.
    */
    buffer = std::vector<uint8_t>(cursor + 8, buffer.end());

    return LZMA::runDebugDecompression(buffer, options);
}

std::vector<uint8_t> LZMA::runDebugDecompression(const std::vector<uint8_t> &in, lzma_options_lzma &options)
{
    lzma_stream stream = LZMA_STREAM_INIT;

    std::vector<uint8_t> result(BytesInTextureAtlas);
    size_t cursor = 0;
    lzma_ret lzmaStatus;

    lzma_filter filters[2] = {
        lzma_filter{LZMA_FILTER_LZMA1, &options},
        lzma_filter{LZMA_VLI_UNKNOWN, NULL}};

    lzmaStatus = lzma_raw_decoder(&stream, filters);

    assert(lzmaStatus == LZMA_OK);

    size_t remainingOut = BytesInTextureAtlas;

    stream.next_in = in.data();
    stream.avail_in = in.size();
    stream.next_out = result.data();
    stream.avail_out = remainingOut;

    while (true)
    {
        lzmaStatus = lzma_code(&stream, stream.avail_in == 0 ? LZMA_FINISH : LZMA_RUN);
        if (lzmaStatus == LZMA_STREAM_END)
        {
            cursor += remainingOut - stream.avail_out;
            if (0 != stream.avail_in)
                abort();
            result.resize(cursor);
            lzma_end(&stream);
            return result;
        }
        if (lzmaStatus != LZMA_OK)
            abort();
        if (stream.avail_out == 0)
        {
            cursor += remainingOut - stream.avail_out;
            result.resize(result.size() << 1);
            stream.next_out = reinterpret_cast<uint8_t *>(&result[0] + cursor);
            stream.avail_out = remainingOut = result.size() - cursor;
        }
    }
}

#pragma warning(pop)