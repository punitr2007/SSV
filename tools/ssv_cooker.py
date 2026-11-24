#!/usr/bin/env python3
"""
Split/Second: Velocity - Offline Asset Cooker
============================================
Converts raw and .ARK assets into zero-overhead GPU-ready binary packages (.ssvpak).
Features:
 - Interleaved VBO and EBO packing for direct glBufferData
 - Zero runtime decompression overhead (mmap-compatible)
 - Name hashing (FNV-1a 32-bit) for O(1) asset lookups
"""

import os
import sys
import struct
import zlib

SSVPAK_MAGIC = 0x50565353  # "SSVP"
SSVPAK_VERSION = 0x00010000 # 1.0.0

RESOURCE_TYPE_MESH = 1
RESOURCE_TYPE_TEXTURE = 2
RESOURCE_TYPE_SHADER = 3
RESOURCE_TYPE_MATERIAL = 4
RESOURCE_TYPE_AUDIO = 5

def fnv1a_32(data: str) -> int:
    h = 0x811c9dc5
    for b in data.lower().encode('utf-8'):
        h = ((h ^ b) * 0x01000193) & 0xFFFFFFFF
    return h

class SSVPakWriter:
    def __init__(self, output_path: str):
        self.output_path = output_path
        self.entries = []
        self.payload = bytearray()

    def add_mesh(self, name: str, vertices_bytes: bytes, indices_bytes: bytes, vertex_stride: int, is_32bit_index: bool = False):
        """Pack an interleaved vertex buffer and index buffer into a GPU blob."""
        vertex_count = len(vertices_bytes) // vertex_stride
        index_size = 4 if is_32bit_index else 2
        index_count = len(indices_bytes) // index_size

        mesh_header_size = 32
        vbo_offset = mesh_header_size
        vbo_size = len(vertices_bytes)
        ebo_offset = vbo_offset + vbo_size
        ebo_size = len(indices_bytes)

        mesh_header = struct.pack(
            '<IIIIIIII',
            vertex_count,
            index_count,
            vertex_stride,
            1 if is_32bit_index else 0,
            vbo_offset,
            vbo_size,
            ebo_offset,
            ebo_size
        )

        blob = mesh_header + vertices_bytes + indices_bytes
        self._add_entry(name, RESOURCE_TYPE_MESH, blob)

    def add_shader(self, name: str, shader_source: str):
        blob = shader_source.encode('utf-8')
        self._add_entry(name, RESOURCE_TYPE_SHADER, blob)

    def add_raw(self, name: str, res_type: int, data: bytes):
        self._add_entry(name, res_type, data)

    def _add_entry(self, name: str, res_type: int, data: bytes):
        name_hash = fnv1a_32(name)
        offset = len(self.payload)
        size = len(data)
        self.payload.extend(data)
        self.entries.append({
            'name': name,
            'name_hash': name_hash,
            'type': res_type,
            'flags': 0,
            'data_offset': offset,
            'data_size': size,
            'uncompressed_size': size
        })
        print(f"  [+] Cooked {name} (type={res_type}, size={size} bytes, hash=0x{name_hash:08X})")

    def build(self):
        os.makedirs(os.path.dirname(os.path.abspath(self.output_path)), exist_ok=True)
        header_size = 32
        entry_size = 32
        toc_offset = header_size
        toc_size = len(self.entries) * entry_size
        payload_start = header_size + toc_size

        total_size = payload_start + len(self.payload)

        # 1. Build Header
        header = struct.pack(
            '<IIQIIII',
            SSVPAK_MAGIC,
            SSVPAK_VERSION,
            toc_offset,
            len(self.entries),
            total_size,
            0, # flags
            0  # reserved
        )

        # 2. Build TOC
        toc = bytearray()
        for e in self.entries:
            toc_entry = struct.pack(
                '<IHHIQQII',
                e['name_hash'],
                e['type'],
                e['flags'],
                0, # reserved
                payload_start + e['data_offset'],
                e['data_size'],
                e['uncompressed_size'],
                0  # reserved
            )
            toc.extend(toc_entry)

        with open(self.output_path, 'wb') as f:
            f.write(header)
            f.write(toc)
            f.write(self.payload)

        print(f"\n[SSV Cooker] Successfully built package: {self.output_path}")
        print(f"  Total resources: {len(self.entries)}")
        print(f"  Total package size: {total_size:,} bytes")

def main():
    print("======================================================")
    print("   SPLIT/SECOND: VELOCITY - GPU ASSET COOKER         ")
    print("======================================================")

    out_bundle = r"D:\Codes\Win_codes\SSV\android\app\src\main\assets\main_game.bin"
    cooker = SSVPakWriter(out_bundle)

    # 1. Cook translated GLSL shaders
    shader_dir = r"D:\Codes\Win_codes\SSV\assets\shaders"
    if os.path.exists(shader_dir):
        for sfile in os.listdir(shader_dir):
            path = os.path.join(shader_dir, sfile)
            with open(path, 'r', encoding='utf-8') as f:
                cooker.add_shader(f"shaders/{sfile}", f.read())

    # 2. Build sample vehicle mesh (Cobretti GT500 chassis test quad)
    # Stride: 32 bytes (Pos=3f, Norm=3f, UV=2f)
    verts = struct.pack(
        '<8f8f8f8f',
        -1.0, 0.0, -1.0,  0.0, 1.0, 0.0,  0.0, 0.0,
         1.0, 0.0, -1.0,  0.0, 1.0, 0.0,  1.0, 0.0,
         1.0, 0.0,  1.0,  0.0, 1.0, 0.0,  1.0, 1.0,
        -1.0, 0.0,  1.0,  0.0, 1.0, 0.0,  0.0, 1.0
    )
    indices = struct.pack('<6H', 0, 1, 2, 0, 2, 3)
    cooker.add_mesh("models/vehicles/cobretti_gt500_chassis", verts, indices, vertex_stride=32, is_32bit_index=False)

    cooker.build()

if __name__ == '__main__':
    main()
