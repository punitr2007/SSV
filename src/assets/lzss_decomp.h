#pragma once
/**
 * @file lzss_decomp.h
 * @brief Reverse engineered LZSS sliding-window decompressor for Black Rock Studio ARK containers.
 *
 * Reconstructed from disassembly of lzssUnpackFactory (0x00BA0410) and lzssLoader (0x00BA0864).
 */

#include <cstdint>
#include <vector>
#include <cstddef>

namespace SSV {
namespace Assets {

class LZSSDecompressor {
public:
    static constexpr size_t RING_BUFFER_SIZE = 4096;
    static constexpr size_t F = 18;          // Max match length
    static constexpr size_t THRESHOLD = 2;   // Match threshold

    /**
     * Decompresses an LZSS compressed payload buffer.
     * @param compressedData Pointer to compressed byte stream.
     * @param compressedSize Size of compressed stream.
     * @param uncompressedSize Expected uncompressed size.
     * @return Decompressed byte vector.
     */
    static std::vector<uint8_t> Decompress(const uint8_t* compressedData, size_t compressedSize, size_t uncompressedSize);

    /**
     * Compresses a raw byte buffer using sliding window LZSS.
     */
    static std::vector<uint8_t> Compress(const uint8_t* inputData, size_t inputSize);
};

} // namespace Assets
} // namespace SSV
