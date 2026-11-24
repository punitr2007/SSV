#include "ark_parser.h"
#include <iostream>
#include <fstream>

uint32_t brs_hash(const std::string& s) {
    uint32_t h = 0;
    for (char c : s) {
        h = (h * 31) + std::tolower((unsigned char)c);
    }
    return h;
}

int main() {
    SSV::Assets::ArkArchive ark;
    if (!ark.Open("D:\\Codes\\Win_codes\\Split.Second.Velocity\\data\\resident.ark")) {
        std::cerr << "Failed to open resident.ark\n";
        return 1;
    }

    std::vector<std::string> names = {
        "UI/Logic/Season.xml",
        "UI/Logic/TrackFiltration.xml"
    };

    for (const auto& name : names) {
        uint32_t h = SSV::Assets::ArkArchive::ComputePathHash(name);
        const auto* entry = ark.FindEntry(h);
        if (entry) {
            // Find index of entry
            size_t idx = 0;
            for (size_t i = 0; i < ark.GetEntryCount(); ++i) {
                if (ark.GetEntry(i).nameHash == h) { idx = i; break; }
            }
            std::vector<uint8_t> data = ark.ExtractEntry(idx);
            std::string outName = name;
            for (char& c : outName) if (c == '/') c = '_';
            std::string outPath = "assets/extracted/" + outName;
            std::ofstream out(outPath, std::ios::binary);
            out.write(reinterpret_cast<const char*>(data.data()), data.size());
            std::cout << "Successfully extracted " << name << " (" << data.size() << " bytes) to " << outPath << "\n";

            // Print first 400 characters
            std::string text(reinterpret_cast<const char*>(data.data()), std::min(data.size(), (size_t)400));
            std::cout << "--- Preview ---\n" << text << "\n---------------\n\n";
        } else {
            std::cerr << "Failed to find entry for " << name << "\n";
        }
    }
    return 0;
}
