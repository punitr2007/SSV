#include "lzss_decomp.h"
#include <iostream>
#include <string>

using namespace SSV::Assets;

int main() {
    std::string original = "The quick brown fox jumps over the lazy dog. "
                           "The quick brown fox jumps over the lazy dog! "
                           "Split Second Velocity Powerplays! "
                           "Split Second Velocity Powerplays! "
                           "1234567890 1234567890 1234567890";

    std::vector<uint8_t> inputBytes(original.begin(), original.end());
    std::vector<uint8_t> compressed = LZSSDecompressor::Compress(inputBytes.data(), inputBytes.size());
    std::vector<uint8_t> decompressed = LZSSDecompressor::Decompress(compressed.data(), compressed.size(), inputBytes.size());

    std::cout << "Original size: " << inputBytes.size() << std::endl;
    std::cout << "Compressed size: " << compressed.size() << std::endl;
    std::cout << "Decompressed size: " << decompressed.size() << std::endl;

    for (size_t i = 0; i < inputBytes.size(); ++i) {
        if (i >= decompressed.size()) {
            std::cout << "Truncated at index " << i << std::endl;
            break;
        }
        if (inputBytes[i] != decompressed[i]) {
            std::cout << "Mismatch at " << i << ": expected '" << (char)inputBytes[i] 
                      << "' (0x" << std::hex << (int)inputBytes[i] << "), got '" 
                      << (char)decompressed[i] << "' (0x" << (int)decompressed[i] << std::dec << ")" << std::endl;
            break;
        }
    }
    return 0;
}
