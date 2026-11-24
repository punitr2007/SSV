import os
import struct

def inspect_ark(path):
    print(f"\n=======================================================")
    print(f"Inspecting ARK: {path}")
    print(f"=======================================================")
    if not os.path.exists(path):
        print("File does not exist.")
        return

    with open(path, "rb") as f:
        magic = f.read(4)
        version, num_files, toc_offset = struct.unpack("<III", f.read(12))
        print(f"Magic: {magic}, Version: {version}, Num Files: {num_files}, TOC Offset: {toc_offset}")
        
        f.seek(toc_offset)
        entries = []
        for i in range(num_files):
            name_hash, offset, uncomp_sz, comp_sz, flags = struct.unpack("<IIIII", f.read(20))
            entries.append((name_hash, offset, uncomp_sz, comp_sz, flags))
        
        # Sort by uncompressed size descending to see the largest assets (meshes, textures, track geometry)
        sorted_by_sz = sorted(entries, key=lambda x: x[2], reverse=True)
        print(f"\nTop 15 Largest Files in {os.path.basename(path)}:")
        for e in sorted_by_sz[:15]:
            print(f"  Hash: 0x{e[0]:08X} | Uncomp: {e[2]:10d} B ({e[2]/1024/1024:6.2f} MB) | Comp: {e[3]:10d} B | Offset: 0x{e[1]:08X} | Flags: 0x{e[4]:04X}")

inspect_ark(r"D:\Codes\Win_codes\Split.Second.Velocity\data\environments\levels\downtown.ark")
inspect_ark(r"D:\Codes\Win_codes\Split.Second.Velocity\data\environments\levels\downtown\sectors\downtown.static.ark")
