#include "compression.h"

#include <stdexcept>
#include "../file.h"

#include <cassert>

std::vector<uint8_t> remove_leading(std::vector<uint8_t> const &buffer, uint8_t c)
{
    auto end = buffer.end();

    for (auto i = buffer.begin(); i != end; ++i)
    {
        if (*i != c)
        {
            return std::vector<uint8_t>(i, end);
        }
    }

    // All characters were leading or the string is empty.
    return std::vector<uint8_t>();
}

std::vector<uint8_t> LZMA::decompress(const std::vector<uint8_t> &inBuffer)
{

    /*
        The file can be padded with a number of NULL bytes (depending on LZMA file size)
    */
    auto buffer = remove_leading(inBuffer, 0x0);

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

    return LZMA::decompressRaw(buffer, options);
}

std::vector<uint8_t> LZMA::decompressFile(const std::filesystem::path &filepath)
{
    // std::cout << filepath.filename().string() << std::endl;
    auto file = File::read(filepath.string());
    return decompress(file);
}

// Level is between 0 (no compression), 9 (slow compression, small output).
std::string LZMA::compress(const std::string &in, int level)
{
    std::string result;
    result.resize(in.size() + (in.size() >> 2) + 128);
    size_t out_pos = 0;
    if (LZMA_OK != lzma_easy_buffer_encode(
                       level, LZMA_CHECK_CRC32, NULL,
                       reinterpret_cast<uint8_t *>(const_cast<char *>(in.data())), in.size(),
                       reinterpret_cast<uint8_t *>(&result[0]), &out_pos, result.size()))
        abort();
    result.resize(out_pos);
    return result;
}

std::vector<uint8_t> LZMA::decompressRaw(const std::vector<uint8_t> &in, lzma_options_lzma &options)
{
    static const size_t kMemLimit = 1 << 30; // 1 GB.
    lzma_stream strm = LZMA_STREAM_INIT;

    std::vector<uint8_t> result;
    result.resize(1048576);
    size_t cursor = 0;
    lzma_ret lzmaStatus;

    lzma_filter filters[2] = {
        lzma_filter{LZMA_FILTER_LZMA1, &options},
        lzma_filter{LZMA_VLI_UNKNOWN, NULL}};

    lzmaStatus = lzma_raw_decoder(&strm, filters);

    assert(lzmaStatus == LZMA_OK);

    size_t avail0 = result.size();
    strm.next_in = reinterpret_cast<const uint8_t *>(in.data());
    strm.avail_in = in.size();
    strm.next_out = reinterpret_cast<uint8_t *>(&result[0]);
    strm.avail_out = avail0;
    while (true)
    {
        lzmaStatus = lzma_code(&strm, strm.avail_in == 0 ? LZMA_FINISH : LZMA_RUN);
        if (lzmaStatus == LZMA_STREAM_END)
        {
            cursor += avail0 - strm.avail_out;
            if (0 != strm.avail_in)
                abort();
            result.resize(cursor);
            lzma_end(&strm);
            return result;
        }
        if (lzmaStatus != LZMA_OK)
            abort();
        if (strm.avail_out == 0)
        {
            cursor += avail0 - strm.avail_out;
            result.resize(result.size() << 1);
            strm.next_out = reinterpret_cast<uint8_t *>(&result[0] + cursor);
            strm.avail_out = avail0 = result.size() - cursor;
        }
    }
}