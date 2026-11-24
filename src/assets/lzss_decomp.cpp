#include "lzss_decomp.h"
#include <cstring>
#include <algorithm>

namespace SSV {
namespace Assets {

constexpr int N = 4096;
constexpr int F = 18;
constexpr int THRESHOLD = 2;

std::vector<uint8_t> LZSSDecompressor::Decompress(const uint8_t* inData, size_t inSize, size_t outSize) {
    if (!inData || inSize == 0) return {};

    std::vector<uint8_t> out;
    out.reserve(outSize > 0 ? outSize : inSize * 2);

    uint8_t buf[N];
    std::memset(buf, 0, sizeof(buf));

    int r = N - F;
    size_t inPos = 0;
    unsigned int flags = 0;

    while (inPos < inSize) {
        if (outSize > 0 && out.size() >= outSize) break;

        flags >>= 1;
        if ((flags & 0x100) == 0) {
            if (inPos >= inSize) break;
            flags = inData[inPos++] | 0xFF00;
        }

        if (flags & 1) {
            if (inPos >= inSize) break;
            uint8_t c = inData[inPos++];
            out.push_back(c);
            buf[r] = c;
            r = (r + 1) & (N - 1);
        } else {
            if (inPos + 1 >= inSize) break;
            uint8_t b1 = inData[inPos++];
            uint8_t b2 = inData[inPos++];
            int matchPos = b1 | ((b2 & 0xF0) << 4);
            int matchLen = (b2 & 0x0F) + THRESHOLD + 1;

            for (int k = 0; k < matchLen; ++k) {
                if (outSize > 0 && out.size() >= outSize) break;
                uint8_t c = buf[(matchPos + k) & (N - 1)];
                out.push_back(c);
                buf[r] = c;
                r = (r + 1) & (N - 1);
            }
        }
    }

    return out;
}

std::vector<uint8_t> LZSSDecompressor::Compress(const uint8_t* inData, size_t inSize) {
    if (!inData || inSize == 0) return {};

    std::vector<uint8_t> out;
    out.reserve(inSize);

    uint8_t buf[N];
    std::memset(buf, 0, sizeof(buf));

    int r = N - F;
    size_t inPos = 0;

    uint8_t flags = 0;
    uint8_t flagMask = 1;
    std::vector<uint8_t> chunk;
    chunk.reserve(16);

    while (inPos < inSize) {
        int maxLen = 0;
        int maxPos = 0;
        size_t avail = std::min<size_t>(F, inSize - inPos);

        for (int i = 0; i < N; ++i) {
            size_t l = 0;
            while (l < avail && buf[(i + l) & (N - 1)] == inData[inPos + l]) {
                l++;
            }
            if (static_cast<int>(l) > maxLen) {
                maxLen = static_cast<int>(l);
                maxPos = i;
            }
        }

        if (maxLen <= THRESHOLD) {
            uint8_t c = inData[inPos++];
            flags |= flagMask;
            chunk.push_back(c);
            buf[r] = c;
            r = (r + 1) & (N - 1);
        } else {
            chunk.push_back(static_cast<uint8_t>(maxPos & 0xFF));
            chunk.push_back(static_cast<uint8_t>(((maxPos >> 4) & 0xF0) | ((maxLen - (THRESHOLD + 1)) & 0x0F)));
            for (int k = 0; k < maxLen; ++k) {
                uint8_t c = inData[inPos++];
                buf[r] = c;
                r = (r + 1) & (N - 1);
            }
        }

        flagMask <<= 1;
        if (flagMask == 0) {
            out.push_back(flags);
            out.insert(out.end(), chunk.begin(), chunk.end());
            flags = 0;
            flagMask = 1;
            chunk.clear();
        }
    }

    if (flagMask != 1) {
        out.push_back(flags);
        out.insert(out.end(), chunk.begin(), chunk.end());
    }

    return out;
}

} // namespace Assets
} // namespace SSV
