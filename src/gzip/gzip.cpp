#include "gzip.hpp"

#include <zlib.h>

std::string gzipCompress(const std::string &input)
{
    z_stream zs{};
    std::string output;

    if (deflateInit2(
            &zs,
            Z_DEFAULT_COMPRESSION,
            Z_DEFLATED,
            MAX_WBITS + 16,
            8,
            Z_DEFAULT_STRATEGY) != Z_OK)
    {
        return "";
    }

    zs.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(input.data()));
    zs.avail_in = input.size();

    char buffer[32768];

    int ret;

    do
    {
        zs.next_out = reinterpret_cast<Bytef *>(buffer);
        zs.avail_out = sizeof(buffer);

        ret = deflate(&zs, Z_FINISH);

        size_t bytes_written = sizeof(buffer) - zs.avail_out;

        output.append(buffer, bytes_written);

    } while (ret == Z_OK);

    deflateEnd(&zs);

    if (ret != Z_STREAM_END)
        return "";

    return output;
}