#include "cooked_bundle.h"
#include <cassert>
#include <iostream>
#include <fstream>
#include <vector>

void RunAllCookedBundleTests() {
    std::cout << "[TEST] Running Cooked Asset Bundle (.ssvpak) Reader Tests...\n";

    const char* path = "assets/main_game.bin";
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    assert(file.is_open() && "main_game.bin must exist in assets/");

    std::streamsize sz = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(sz);
    file.read(reinterpret_cast<char*>(buffer.data()), sz);

    SSV::CookedBundleReader reader;
    bool ok = reader.Open(buffer.data(), buffer.size());
    assert(ok && "Failed to open cooked bundle");
    assert(reader.GetResourceCount() == 4 && "Expected 4 resources in cooked bundle");

    // Verify Vehicle Mesh Entry
    const auto* meshEntry = reader.FindEntry("models/vehicles/musclecar_01");
    assert(meshEntry != nullptr && "models/vehicles/musclecar_01 must be in bundle");
    assert(meshEntry->type == SSV::CookedResourceType::Mesh);

    const uint8_t* meshBlob = reader.GetEntryData(meshEntry);
    const auto* meshHdr = reinterpret_cast<const SSV::GPUMeshHeader*>(meshBlob);
    assert(meshHdr->vertexCount == 2929 && "Expected 2929 vertices in Musclecar_01");
    assert(meshHdr->indexCount == 11616 && "Expected 11616 indices in Musclecar_01");
    assert(meshHdr->vertexStride == 48 && "Expected 48-byte vertex stride");

    // Verify Texture Entry
    const auto* texEntry = reader.FindEntry("textures/vehicles/musclecar_01_diffuse");
    assert(texEntry != nullptr && "textures/vehicles/musclecar_01_diffuse must be in bundle");
    assert(texEntry->type == SSV::CookedResourceType::Texture);

    const uint8_t* texBlob = reader.GetEntryData(texEntry);
    const auto* texHdr = reinterpret_cast<const SSV::GPUTextureHeader*>(texBlob);
    assert(texHdr->width == 512 && "Expected 512 width");
    assert(texHdr->height == 1024 && "Expected 1024 height");
    assert(texHdr->dataSize == 512 * 1024 * 4 && "Expected 2MB RGBA data");

    std::cout << "  -> Cooked bundle verified: Mesh (2,929 verts, 11,616 inds) & Diffuse Texture (512x1024 RGBA8).\n";
}
