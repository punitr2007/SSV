#pragma once
/**
 * @file cooked_bundle.h
 * @brief Zero-overhead GPU-ready Cooked Package Format (.ssvpak) for Split/Second on Mobile
 *
 * Designed for AAssetManager mmap:
 * - Direct pointer casting for Table of Contents
 * - Direct glBufferData / glCompressedTexImage2D from mmap'd pointer
 * - Zero CPU runtime decompression or memory allocations during gameplay
 */

#include <cstdint>
#include <cstddef>
#include <string>
#include <cctype>

namespace SSV {

#pragma pack(push, 1)

constexpr uint32_t SSVPAK_MAGIC = 0x50565353; // "SSVP" in Little Endian
constexpr uint32_t SSVPAK_VERSION = 0x00010000; // 1.0.0

enum class CookedResourceType : uint16_t {
    Unknown = 0,
    Mesh = 1,           // Interleaved VBO + EBO
    Texture = 2,        // ETC2 / ASTC / RGBA8 GPU texture
    Shader = 3,         // Precompiled SPIR-V / GLSL ES 3.00 source
    Material = 4,       // Material parameter block
    AudioBank = 5       // FMOD FSB / FEV bank
};

struct CookedBundleHeader {
    uint32_t magic;           // SSVPAK_MAGIC
    uint32_t version;         // SSVPAK_VERSION
    uint64_t tocOffset;       // Byte offset to CookedResourceEntry array
    uint32_t resourceCount;   // Number of resources in TOC
    uint32_t totalFileSize;   // Total bytes in package
    uint32_t flags;           // Global flags
    uint32_t reserved;        // Padding
};

struct CookedResourceEntry {
    uint32_t nameHash;        // FNV-1a 32-bit hash of asset path
    CookedResourceType type;  // Mesh, Texture, Shader, etc.
    uint16_t flags;           // Resource specific flags (compression, format)
    uint64_t dataOffset;      // File offset to start of data payload
    uint64_t dataSize;        // Size of data payload in bytes
    uint32_t uncompressedSize;// Original uncompressed size
    uint32_t reserved;        // Alignment padding
};

// GPU Mesh Header preceding raw VBO/EBO bytes
struct GPUMeshHeader {
    uint32_t vertexCount;
    uint32_t indexCount;
    uint32_t vertexStride;    // Bytes per vertex (e.g. 48 = Pos[3f], Norm[3f], Color[4f], UV[2f])
    uint32_t indexFormat;     // 0 = uint16_t, 1 = uint32_t
    uint32_t vboOffset;       // Relative to start of GPUMeshHeader
    uint32_t vboSize;
    uint32_t eboOffset;       // Relative to start of GPUMeshHeader
    uint32_t eboSize;
};

// GPU Texture Header preceding raw mipmap blocks
struct GPUTextureHeader {
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint32_t mipLevels;
    uint32_t glInternalFormat;// GL_RGBA8 (0x8058) or GL_RGBA (0x1908)
    uint32_t glFormat;
    uint32_t glType;
    uint32_t dataSize;
};

#pragma pack(pop)

inline uint32_t HashResourceName(const std::string& name) {
    uint32_t h = 0x811c9dc5;
    for (char c : name) {
        uint8_t b = static_cast<uint8_t>(std::tolower(static_cast<unsigned char>(c)));
        h = (h ^ b) * 0x01000193;
    }
    return h;
}

class CookedBundleReader {
public:
    CookedBundleReader() = default;

    bool Open(const uint8_t* data, size_t size) {
        if (!data || size < sizeof(CookedBundleHeader)) return false;
        m_data = data;
        m_size = size;

        const auto* header = reinterpret_cast<const CookedBundleHeader*>(m_data);
        if (header->magic != SSVPAK_MAGIC || header->version != SSVPAK_VERSION) {
            return false;
        }

        if (header->tocOffset + header->resourceCount * sizeof(CookedResourceEntry) > m_size) {
            return false;
        }

        m_entries = reinterpret_cast<const CookedResourceEntry*>(m_data + header->tocOffset);
        m_count = header->resourceCount;
        return true;
    }

    const CookedResourceEntry* FindEntry(const std::string& name) const {
        return FindEntry(HashResourceName(name));
    }

    const CookedResourceEntry* FindEntry(uint32_t nameHash) const {
        for (uint32_t i = 0; i < m_count; ++i) {
            if (m_entries[i].nameHash == nameHash) {
                return &m_entries[i];
            }
        }
        return nullptr;
    }

    const uint8_t* GetEntryData(const CookedResourceEntry* entry) const {
        if (!entry || !m_data) return nullptr;
        if (entry->dataOffset + entry->dataSize > m_size) return nullptr;
        return m_data + entry->dataOffset;
    }

    uint32_t GetResourceCount() const { return m_count; }

private:
    const uint8_t* m_data = nullptr;
    size_t m_size = 0;
    const CookedResourceEntry* m_entries = nullptr;
    uint32_t m_count = 0;
};

} // namespace SSV
