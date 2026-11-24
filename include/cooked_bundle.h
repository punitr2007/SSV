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
    uint32_t vertexStride;    // Bytes per vertex (e.g. 32 = Pos[3f], Norm[3f], UV[2f])
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
    uint32_t glInternalFormat;// GL_COMPRESSED_RGBA8_ETC2_EAC (0x9278) or GL_RGBA8 (0x8058)
    uint32_t glFormat;
    uint32_t glType;
    uint32_t dataSize;
};

#pragma pack(pop)

} // namespace SSV
