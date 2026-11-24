#include "ark_parser.h"
#include "lzss_decomp.h"
#include <cassert>
#include <iostream>
#include <string>

using namespace SSV::Assets;

void TestLZSSRoundTrip() {
    std::cout << "[TEST] Running LZSS Compression / Decompression Round-Trip..." << std::endl;

    // Test data with repeating patterns
    std::string original = "The quick brown fox jumps over the lazy dog. "
                           "The quick brown fox jumps over the lazy dog! "
                           "Split Second Velocity Powerplays! "
                           "Split Second Velocity Powerplays! "
                           "1234567890 1234567890 1234567890";

    std::vector<uint8_t> inputBytes(original.begin(), original.end());
    std::vector<uint8_t> compressed = LZSSDecompressor::Compress(inputBytes.data(), inputBytes.size());

    assert(!compressed.empty());
    assert(compressed.size() < inputBytes.size()); // Verify compression happened

    std::vector<uint8_t> decompressed = LZSSDecompressor::Decompress(compressed.data(), compressed.size(), inputBytes.size());
    assert(decompressed.size() == inputBytes.size());
    assert(decompressed == inputBytes);

    std::cout << "  -> LZSS round-trip verified (" << inputBytes.size() << " bytes -> "
              << compressed.size() << " bytes -> " << decompressed.size() << " bytes)." << std::endl;
}

void TestRealArkArchiveParsing() {
    std::cout << "[TEST] Running Real ARK Archive Parsing Against Game Data..." << std::endl;

    std::string arkPath = "D:\\Codes\\Win_codes\\Split.Second.Velocity\\data\\frontendpowerplays.ark";
    ArkArchive ark;
    bool opened = ark.Open(arkPath);
    if (!opened) {
        std::cout << "  (Note: " << arkPath << " not opened, skipping disk test)" << std::endl;
        return;
    }

    assert(ark.IsOpen());
    assert(ark.GetHeader().version == 0x00010000);
    assert(ark.GetEntryCount() == 21);

    // Verify first entry
    const auto& e0 = ark.GetEntry(0);
    assert(e0.offset == 0x00000000);

    // Extract an entry
    std::vector<uint8_t> extracted = ark.ExtractEntry(0);
    assert(!extracted.empty());

    ark.Close();
    assert(!ark.IsOpen());
    std::cout << "  -> Real frontendpowerplays.ark parsed and verified." << std::endl;
}

void RunAllArkTests() {
    TestLZSSRoundTrip();
    TestRealArkArchiveParsing();
}
