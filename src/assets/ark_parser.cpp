#include "ark_parser.h"
#include "lzss_decomp.h"
#include <algorithm>
#include <iostream>

namespace SSV {
namespace Assets {

ArkArchive::~ArkArchive() {
    Close();
}

bool ArkArchive::Open(const std::string& filepath) {
    Close();

    m_stream.open(filepath, std::ios::binary);
    if (!m_stream.is_open()) {
        return false;
    }

    m_stream.read(reinterpret_cast<char*>(&m_header), sizeof(ArkHeader));
    if (!m_stream.good()) {
        Close();
        return false;
    }

    // Verify header constraints
    if (m_header.version != 0x00010000 && m_header.version != 0x00000100) {
        // Support endianness check
        Close();
        return false;
    }

    if (m_header.entryCount == 0 || m_header.entryCount > 500000) {
        Close();
        return false;
    }

    m_entries.resize(m_header.entryCount);
    m_stream.seekg(m_header.tocOffset, std::ios::beg);
    m_stream.read(reinterpret_cast<char*>(m_entries.data()), m_header.entryCount * sizeof(ArkEntry));

    if (!m_stream.good()) {
        Close();
        return false;
    }

    m_hashToIndex.reserve(m_header.entryCount);
    for (size_t i = 0; i < m_entries.size(); ++i) {
        m_hashToIndex[m_entries[i].nameHash] = i;
    }

    m_filePath = filepath;
    m_isOpen = true;
    return true;
}

void ArkArchive::Close() {
    if (m_stream.is_open()) {
        m_stream.close();
    }
    m_entries.clear();
    m_hashToIndex.clear();
    m_isOpen = false;
}

uint32_t ArkArchive::ComputePathHash(const std::string& path) {
    // Normalization: lowercase and convert backslashes to forward slashes
    std::string norm = path;
    std::transform(norm.begin(), norm.end(), norm.begin(), [](unsigned char c) {
        if (c == '\\') return '/';
        return static_cast<char>(std::tolower(c));
    });

    // FNV-1a / Jenkins hybrid hash calculation
    uint32_t hash = 0x811c9dc5;
    for (char c : norm) {
        hash ^= static_cast<uint8_t>(c);
        hash *= 0x01000193;
    }
    return hash;
}

const ArkEntry* ArkArchive::FindEntry(uint32_t nameHash) const {
    auto it = m_hashToIndex.find(nameHash);
    if (it != m_hashToIndex.end()) {
        return &m_entries[it->second];
    }
    return nullptr;
}

std::vector<uint8_t> ArkArchive::ExtractEntry(size_t index) const {
    if (!m_isOpen || index >= m_entries.size()) {
        return {};
    }

    const auto& entry = m_entries[index];
    if (entry.compressedSize == 0) {
        return {};
    }

    std::vector<uint8_t> rawBuffer(entry.compressedSize);
    m_stream.seekg(entry.offset, std::ios::beg);
    m_stream.read(reinterpret_cast<char*>(rawBuffer.data()), entry.compressedSize);

    if (!m_stream.good()) {
        return {};
    }

    if ((entry.flags & 1) != 0 && entry.uncompressedSize > entry.compressedSize) {
        // Stream is LZSS compressed
        return LZSSDecompressor::Decompress(rawBuffer.data(), rawBuffer.size(), entry.uncompressedSize);
    }

    return rawBuffer;
}

std::vector<uint8_t> ArkArchive::ExtractFile(const std::string& path) const {
    uint32_t hash = ComputePathHash(path);
    const ArkEntry* entry = FindEntry(hash);
    if (!entry) {
        return {};
    }
    auto it = m_hashToIndex.find(hash);
    return ExtractEntry(it->second);
}

} // namespace Assets
} // namespace SSV
