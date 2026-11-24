import os
import struct

# Split/Second BRS hash function: hash = hash * 31 + tolower(c) (32-bit unsigned)
def brs_hash(s):
    h = 0
    for c in s:
        c_val = ord(c.lower())
        h = ((h * 31) + c_val) & 0xFFFFFFFF
    return h

# Known and candidate file names to look for
candidates = [
    "TrackDatabase.xml",
    "Tracks.xml",
    "EventDatabase.xml",
    "Championship.xml",
    "Tracks/TrackDatabase.xml",
    "Tracks/Tracks.xml",
    "Database/TrackDatabase.xml",
    "Database/Tracks.xml",
    "Game/Tracks.xml",
    "Game/TrackDatabase.xml",
    "Events/EventDatabase.xml",
    "EnvironmentDatabase.xml",
    "Environments.xml",
    "Environments/Environments.xml",
    "Season.xml",
    "Routes.xml",
    "Tracks/Routes.xml"
]

print("Candidate BRS hashes:")
for name in candidates:
    h = brs_hash(name)
    print(f"  {h:08X} ({h:10d}) -> {name}")

# Read resident.ark TOC
ark_path = r"D:\Codes\Win_codes\Split.Second.Velocity\data\resident.ark"
if os.path.exists(ark_path):
    print(f"\nOpening {ark_path}...")
    with open(ark_path, "rb") as f:
        magic = f.read(4)
        print("Magic:", magic)
        version, num_files, toc_offset = struct.unpack("<III", f.read(12))
        print(f"Version: {version}, Num Files: {num_files}, TOC Offset: {toc_offset}")
        
        f.seek(toc_offset)
        toc_entries = []
        for i in range(num_files):
            # 20-byte TOC entry: nameHash (4B), offset (4B), uncompressedSize (4B), compressedSize (4B), flags (4B)
            name_hash, offset, uncomp_sz, comp_sz, flags = struct.unpack("<IIIII", f.read(20))
            toc_entries.append((name_hash, offset, uncomp_sz, comp_sz, flags))
        
        # Check if any candidate hashes match
        hash_to_entry = {e[0]: e for e in toc_entries}
        print(f"\nChecking candidate hashes against {len(toc_entries)} resident.ark entries:")
        for name in candidates:
            h = brs_hash(name)
            if h in hash_to_entry:
                e = hash_to_entry[h]
                print(f"  [FOUND!] {h:08X} -> {name}: uncomp_size={e[2]}, comp_size={e[3]}, offset={e[1]}")

# Also check environments directory
env_dir = r"D:\Codes\Win_codes\Split.Second.Velocity\data\environments"
if os.path.exists(env_dir):
    print(f"\nScanning {env_dir}:")
    for root, dirs, files in os.walk(env_dir):
        rel = os.path.relpath(root, env_dir)
        print(f"  Directory: {rel}")
        for file in files[:10]: # Print first 10 files in each dir
            print(f"    {file}")
        if len(files) > 10:
            print(f"    ... ({len(files)} total files)")
