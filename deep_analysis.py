import pefile
import capstone
import re
import os
import struct

pe_path = r"D:\Codes\Win_codes\Split.Second.Velocity\SplitSecond.exe"
pe = pefile.PE(pe_path)

image_base = pe.OPTIONAL_HEADER.ImageBase

def rva_to_va(rva):
    return image_base + rva

def va_to_rva(va):
    return va - image_base

# 1. Search for RTTI type descriptors ".?AV"
print("=== 1. SEARCHING FOR C++ RTTI CLASS NAMES ===")
rtti_classes = []
for s in pe.sections:
    name = s.Name.decode('utf-8', errors='ignore').strip('\x00')
    if name in ['.rdata', '.data']:
        data = s.get_data()
        offset = 0
        while True:
            idx = data.find(b".?AV", offset)
            if idx == -1:
                break
            # read null-terminated string
            end_idx = data.find(b"\x00", idx)
            if end_idx != -1 and end_idx - idx < 200:
                cls_name = data[idx:end_idx].decode('utf-8', errors='ignore')
                va = image_base + s.VirtualAddress + idx
                rtti_classes.append((va, cls_name))
                offset = end_idx + 1
            else:
                offset = idx + 4

print(f"Total C++ RTTI classes found: {len(rtti_classes)}")
with open(r"D:\Codes\Win_codes\SSV\rtti_classes.txt", "w", encoding="utf-8") as f:
    for va, cls in rtti_classes:
        f.write(f"0x{va:08X} {cls}\n")

# Sample RTTI classes
print("Sample RTTI classes:")
for va, cls in rtti_classes[:30]:
    print(f"  0x{va:08X} {cls}")

# 2. Search for Engine, Black Rock, Physics, Sound, Asset strings
keywords = [
    "black rock", "blackrock", "disney", "engine", "archive", ".ark", "pack", "stream",
    "havok", "physx", "fmod", "bink", "gamestate", "state", "world", "level", "track",
    "render", "directx", "d3d", "camera", "vehicle", "car", "shader", "scene"
]

print("\n=== 2. SEARCHING FOR KEY ARCHITECTURAL STRINGS ===")
found_strings = {}
for kw in keywords:
    found_strings[kw] = []

for s in pe.sections:
    name = s.Name.decode('utf-8', errors='ignore').strip('\x00')
    if name in ['.rdata', '.data', '.text']:
        data = s.get_data()
        for kw in keywords:
            pattern = re.compile(re.escape(kw.encode('utf-8')), re.IGNORECASE)
            for m in pattern.finditer(data):
                start = max(0, m.start() - 20)
                end = min(len(data), m.end() + 60)
                # extract printable string around it
                snippet = data[start:end]
                # find null or newline boundaries
                va = image_base + s.VirtualAddress + m.start()
                # extract full string if null-terminated
                str_start = data.rfind(b"\x00", 0, m.start())
                str_start = 0 if str_start == -1 else str_start + 1
                str_end = data.find(b"\x00", m.end())
                str_end = len(data) if str_end == -1 else str_end
                if str_end - str_start < 300:
                    try:
                        full_str = data[str_start:str_end].decode('utf-8', errors='ignore')
                        found_strings[kw].append((va, full_str))
                    except:
                        pass

with open(r"D:\Codes\Win_codes\SSV\architectural_strings.txt", "w", encoding="utf-8") as f:
    for kw, matches in found_strings.items():
        f.write(f"\n=================== KEYWORD: {kw} ({len(matches)} matches) ===================\n")
        # unique matches
        seen = set()
        for va, s in matches:
            if s not in seen:
                seen.add(s)
                f.write(f"0x{va:08X}: {s}\n")

for kw in ["black rock", "havok", ".ark", "gamestate"]:
    print(f"Keyword '{kw}': {len(found_strings[kw])} matches")
    seen = set()
    for va, s in found_strings[kw][:10]:
        if s not in seen:
            seen.add(s)
            print(f"  0x{va:08X}: {s}")

