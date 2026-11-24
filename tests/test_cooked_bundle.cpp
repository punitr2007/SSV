#include "cooked_bundle.h"
#include <cassert>
#include <iostream>
#include <fstream>
#include <vector>

void RunAllCookedBundleTests() {
    std::cout << "[TEST] Running Cooked Asset Bundle (.ssvpak) Reader Tests...\n";

    const char* path = "assets/main_game.bin";
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cout << "  (Note: assets/main_game.bin not present - skipping binary bundle tests for clean/CI build)\n";
        return;
    }

    std::streamsize sz = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(sz);
    file.read(reinterpret_cast<char*>(buffer.data()), sz);

    SSV::CookedBundleReader reader;
    bool ok = reader.Open(buffer.data(), buffer.size());
    assert(ok && "Failed to open cooked bundle");
    assert(reader.GetResourceCount() >= 10 && "Expected at least 10 resources in cooked bundle");

    // Verify Airport Track Sector Mesh Entry
    const auto* trackEntry = reader.FindEntry("models/tracks/airport_sector_0");
    assert(trackEntry != nullptr && "models/tracks/airport_sector_0 must be in bundle");
    assert(trackEntry->type == SSV::CookedResourceType::Mesh);
    const uint8_t* trackBlob = reader.GetEntryData(trackEntry);
    const auto* trackHdr = reinterpret_cast<const SSV::GPUMeshHeader*>(trackBlob);
    assert(trackHdr->vertexCount == 264 && "Expected 264 vertices in Airport Terminal Start Straight");
    assert(trackHdr->indexCount == 3666 && "Expected 3666 indices (1222 triangles) in Airport Terminal Start Straight");
    assert(trackHdr->vertexStride == 64 && "Expected 64-byte vertex stride in track mesh");

    // Verify Vehicle Mesh Entry with 64-byte vertex format
    const auto* meshEntry = reader.FindEntry("models/vehicles/musclecar_01");
    assert(meshEntry != nullptr && "models/vehicles/musclecar_01 must be in bundle");
    assert(meshEntry->type == SSV::CookedResourceType::Mesh);

    const uint8_t* meshBlob = reader.GetEntryData(meshEntry);
    const auto* meshHdr = reinterpret_cast<const SSV::GPUMeshHeader*>(meshBlob);
    assert(meshHdr->vertexCount > 0 && "Expected positive vertices in Musclecar_01");
    assert(meshHdr->indexCount > 0 && "Expected positive indices in Musclecar_01");
    assert(meshHdr->vertexStride == 64 && "Expected 64-byte vertex stride preserving deform weights");

    // Verify All 6 Textures
    const char* texNames[] = {
        "textures/vehicles/musclecar_01_diffuse",
        "textures/vehicles/musclecar_01_wheel",
        "textures/vehicles/musclecar_01_shadow",
        "textures/vehicles/musclecar_01_normal_spec",
        "textures/vehicles/musclecar_01_interior_lights",
        "textures/vehicles/musclecar_01_damage"
    };

    for (const char* name : texNames) {
        const auto* texEntry = reader.FindEntry(name);
        assert(texEntry != nullptr && "Expected texture in bundle");
        assert(texEntry->type == SSV::CookedResourceType::Texture);
        const uint8_t* texBlob = reader.GetEntryData(texEntry);
        const auto* texHdr = reinterpret_cast<const SSV::GPUTextureHeader*>(texBlob);
        assert(texHdr->width > 0 && texHdr->height > 0);
        assert(texHdr->dataSize == texHdr->width * texHdr->height * 4);
    }

    std::cout << "  -> Cooked bundle verified: Mesh (" << meshHdr->vertexCount << " verts, "
              << meshHdr->indexCount << " inds, 64B stride) & All 6 Textures.\n";
}
