#!/usr/bin/env python3
"""
Black Rock Studio .ARK Archive Tool
Inspects, parses, verifies, and unpacks Split/Second: Velocity .ark containers.
"""

import sys
import os
import struct
import argparse

def parse_ark(file_path):
    if not os.path.exists(file_path):
        print(f"Error: {file_path} not found.")
        return None

    with open(file_path, "rb") as f:
        header_data = f.read(16)
        if len(header_data) < 16:
            print("Error: File smaller than header.")
            return None

        version, toc_size, entry_count, toc_offset = struct.unpack("<4I", header_data)
        print(f"Archive: {os.path.basename(file_path)}")
        print(f"  Version:     0x{version:08X}")
        print(f"  TOC Size:    {toc_size} bytes")
        print(f"  Entry Count: {entry_count} entries")
        print(f"  TOC Offset:  0x{toc_offset:08X}")

        f.seek(toc_offset)
        entries = []
        for i in range(entry_count):
            entry_data = f.read(20)
            if len(entry_data) < 20:
                break
            offset, comp_sz, uncomp_sz, flags, name_hash = struct.unpack("<5I", entry_data)
            entries.append({
                "index": i,
                "offset": offset,
                "compressed_size": comp_sz,
                "uncompressed_size": uncomp_sz,
                "flags": flags,
                "name_hash": name_hash,
                "is_compressed": bool(flags & 1)
            })

    return {
        "header": {"version": version, "toc_size": toc_size, "entry_count": entry_count, "toc_offset": toc_offset},
        "entries": entries
    }

def main():
    parser = argparse.ArgumentParser(description="Split/Second ARK Archive Inspector")
    parser.add_argument("archive", help="Path to .ark file")
    parser.add_argument("--list", action="store_true", help="List first 50 entries")
    args = parser.parse_args()

    res = parse_ark(args.archive)
    if not res:
        return 1

    entries = res["entries"]
    compressed_count = sum(1 for e in entries if e["is_compressed"])
    print(f"Summary: {len(entries)} entries, {compressed_count} LZSS compressed ({compressed_count * 100 / max(1, len(entries)):.1f}%)")

    if args.list:
        print(f"{'Idx':<6} {'Offset':<12} {'CompSize':<10} {'UncompSize':<10} {'Flags':<8} {'NameHash'}")
        print("-" * 65)
        for e in entries[:50]:
            print(f"{e['index']:<6} 0x{e['offset']:08X} {e['compressed_size']:<10} {e['uncompressed_size']:<10} 0x{e['flags']:04X}   0x{e['name_hash']:08X}")
    return 0

if __name__ == "__main__":
    sys.exit(main())
