#pragma once
/**
 * @file ark_parser.h
 * @brief Reverse engineered parser and extractor for Black Rock Studio .ARK archives.
 *
 * Implements binary specification verified from SplitSecond.exe:
 *  - 16-byte container header (0x10)
 *  - 20-byte Table of Contents (TOC) records (0x14)
 *  - Transparent real-time LZSS payload decompression
 */

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <fstream>

namespace SSV {
namespace Assets {

#pragma pack(push, 1)
struct ArkHeader {
    uint32_t version;     // 0x00010000
    uint32_t tocSize;     // Size of Table of Contents in bytes
    uint32_t entryCount;  // Number of files in archive
    uint32_t tocOffset;   // 0x00000010 (Start of TOC entries)
};

struct ArkEntry {
    uint32_t offset;            // Absolute file offset within .ark container
    uint32_t compressedSize;    // Size of stored stream in bytes
    uint32_t uncompressedSize;  // Original uncompressed byte count
    uint32_t flags;             // Bit 0: 1 = LZSS Compressed, 0 = Raw
    uint32_t nameHash;          // 32-bit hash of normalized asset path
};
#pragma pack(pop)

class ArkArchive {
    std::string m_filePath;
    ArkHeader m_header = {};
    std::vector<ArkEntry> m_entries;
    std::unordered_map<uint32_t, size_t> m_hashToIndex;
    mutable std::ifstream m_stream;
    bool m_isOpen = false;

public:
    ArkArchive() = default;
    ~ArkArchive();

    /**
     * Mounts an .ark container file and parses its Table of Contents.
     */
    bool Open(const std::string& filepath);

    /**
     * Closes the mounted container file.
     */
    void Close();

    bool IsOpen() const { return m_isOpen; }
    const ArkHeader& GetHeader() const { return m_header; }
    size_t GetEntryCount() const { return m_entries.size(); }
    const ArkEntry& GetEntry(size_t index) const { return m_entries.at(index); }

    /**
     * Computes the 32-bit Black Rock Studio path hash for lookup.
     */
    static uint32_t ComputePathHash(const std::string& path);

    /**
     * Finds a file entry by its 32-bit path hash.
     */
    const ArkEntry* FindEntry(uint32_t nameHash) const;

    /**
     * Decompresses and extracts a file entry by index.
     */
    std::vector<uint8_t> ExtractEntry(size_t index) const;

    /**
     * Decompresses and extracts a file entry by path.
     */
    std::vector<uint8_t> ExtractFile(const std::string& path) const;
};

} // namespace Assets
} // namespace SSV
