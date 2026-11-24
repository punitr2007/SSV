import struct
import os

ark_path = r"D:\Codes\Win_codes\Split.Second.Velocity\data\environments\levels\airport_test_03\sectors\airport_test_03.static.ark"

print(f"Inspecting {ark_path}...")
with open(ark_path, "rb") as f:
    hdr = f.read(16)
    ver, toc_sz, count, toc_off = struct.unpack("<4I", hdr)
    print(f"Version: 0x{ver:08X}, TOC Size: {toc_sz}, Entry Count: {count}, TOC Offset: 0x{toc_off:08X}")
    
    f.seek(toc_off)
    entries = []
    for i in range(count):
        off, name_hash, flags, uncomp, comp = struct.unpack("<5I", f.read(20))
        entries.append((off, name_hash, flags, uncomp, comp))

print(f"Total files in archive: {len(entries)}")
# Let's inspect the first 500 bytes of the largest file
largest = sorted(entries, key=lambda x: x[3], reverse=True)[0]
print(f"Largest file: Hash 0x{largest[1]:08X}, Offset: 0x{largest[0]:08X}, Uncomp: {largest[3]}, Comp: {largest[4]}, Flags: 0x{largest[2]:X}")

# Let's read first 128 bytes of largest file to check magic
with open(ark_path, "rb") as f:
    f.seek(largest[0])
    raw = f.read(min(largest[4], 256))
    print("First 32 bytes hex:", raw[:32].hex())
    print("First 32 bytes ascii:", "".join(chr(c) if 32 <= c <= 126 else "." for c in raw[:32]))
