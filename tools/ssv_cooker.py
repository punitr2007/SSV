#!/usr/bin/env python3
"""
Split/Second: Velocity - Offline Asset Cooker
============================================
Converts raw and extracted Split/Second game assets into zero-overhead GPU-ready
binary packages (.ssvpak / main_game.bin).
Features:
 - Interleaved VBO and EBO packing for direct glBufferData (zero-copy mmap)
 - Raw RGBA8888 and compressed GPU texture blobs
 - FNV-1a 32-bit name hashing for O(1) asset lookups
 - Zero runtime decompression overhead during gameplay ticks
"""

import os
import sys
import struct
import math
from PIL import Image

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

    def add_texture(self, name: str, width: int, height: int, rgba_bytes: bytes,
                    gl_internal_format: int = 0x8058, gl_format: int = 0x1908, gl_type: int = 0x1401):
        """Pack an uncompressed RGBA8888 GPU texture for direct zero-copy glTexImage2D."""
        data_size = len(rgba_bytes)
        tex_header = struct.pack(
            '<IIIIIIII',
            width,
            height,
            1,  # depth
            1,  # mipLevels
            gl_internal_format,
            gl_format,
            gl_type,
            data_size
        )
        blob = tex_header + rgba_bytes
        self._add_entry(name, RESOURCE_TYPE_TEXTURE, blob)

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
        print(f"  [+] Cooked {name:40s} (type={res_type}, size={size:8,d} bytes, hash=0x{name_hash:08X})")

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
                '<IHHQQII',
                e['name_hash'],
                e['type'],
                e['flags'],
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

def cook_musclecar(cooker: SSVPakWriter, base_dir: str):
    """Decodes original Split/Second Musclecar_01 model and texture, packing them into cooker."""
    model_path = os.path.join(base_dir, "assets", "extracted", "Musclecar_01", "Musclecar_01_High.model")
    tex_path = os.path.join(base_dir, "assets", "extracted", "Musclecar_01", "textures", "body_diffuse.png")

    if not os.path.exists(model_path):
        print(f"  [-] Model file not found: {model_path}")
        return

    # 1. Decode Musclecar_01_High.model
    with open(model_path, 'rb') as f:
        f.seek(9480)
        ibo_bytes = f.read(111476 - 9480)
        f.seek(111484)
        vbo_bytes = f.read(28 * 14574)

    indices = struct.unpack(f'<{len(ibo_bytes)//2}H', ibo_bytes)
    verts = []
    for i in range(14574):
        raw = vbo_bytes[i*28:(i+1)*28]
        w, z, x, y = struct.unpack('<4e', raw[0:8])
        u, v = struct.unpack('<2e', raw[16:20])
        nx, ny, nz = struct.unpack('<3e', raw[20:26])
        verts.append({
            'pos': (float(x), float(y) + 0.60, float(z)),
            'norm': (float(nx), float(ny), float(nz)),
            'uv': (float(u), float(v))
        })

    # Decode triangle strip and filter central vehicle envelope
    clean_tris = []
    wind = 0
    for i in range(len(indices) - 2):
        i0, i1, i2 = indices[i], indices[i+1], indices[i+2]
        if i0 == i1 or i1 == i2 or i0 == i2:
            wind = 0
            continue
        if i0 >= 14574 or i1 >= 14574 or i2 >= 14574:
            wind = 0
            continue
        
        p0, p1, p2 = verts[i0]['pos'], verts[i1]['pos'], verts[i2]['pos']
        # Filter realistic car body envelope
        if (abs(p0[0]) > 1.15 or abs(p1[0]) > 1.15 or abs(p2[0]) > 1.15 or
            p0[1] < -0.15 or p1[1] < -0.15 or p2[1] < -0.15 or
            p0[1] > 1.65 or p1[1] > 1.65 or p2[1] > 1.65 or
            abs(p0[2]) > 2.65 or abs(p1[2]) > 2.65 or abs(p2[2]) > 2.65):
            wind = 0
            continue
        d01 = math.dist(p0, p1); d12 = math.dist(p1, p2); d20 = math.dist(p2, p0)
        if max(d01, d12, d20) > 1.2:
            wind = 0
            continue
            
        if wind % 2 == 1:
            clean_tris.append((i0, i2, i1))
        else:
            clean_tris.append((i0, i1, i2))
        wind += 1

    # Remap vertices to 0..N-1
    used_map = {}
    compact_verts = []
    compact_indices = []

    for tri in clean_tris:
        tri_mapped = []
        for orig_idx in tri:
            if orig_idx not in used_map:
                new_idx = len(compact_verts)
                used_map[orig_idx] = new_idx
                compact_verts.append(verts[orig_idx])
            tri_mapped.append(used_map[orig_idx])
        compact_indices.extend(tri_mapped)

    # Pack 48-byte Vertex: pos[3f], norm[3f], color[4f], uv[2f]
    vbo_bytes_out = bytearray()
    for v in compact_verts:
        px, py, pz = v['pos']
        nx, ny, nz = v['norm']
        u, v_tex = v['uv']
        vbo_bytes_out.extend(struct.pack('<3f3f4f2f', px, py, pz, nx, ny, nz, 1.0, 1.0, 1.0, 1.0, u, v_tex))

    ebo_bytes_out = struct.pack(f'<{len(compact_indices)}H', *compact_indices)
    cooker.add_mesh("models/vehicles/musclecar_01", vbo_bytes_out, ebo_bytes_out, vertex_stride=48, is_32bit_index=False)

    # 2. Pack body diffuse texture
    if os.path.exists(tex_path):
        im = Image.open(tex_path)
        im_resized = im.resize((512, 1024), Image.Resampling.LANCZOS)
        rgba_bytes = im_resized.tobytes()
        cooker.add_texture("textures/vehicles/musclecar_01_diffuse", 512, 1024, rgba_bytes)
    else:
        print(f"  [-] Texture file not found: {tex_path}")

def main():
    print("======================================================")
    print("   SPLIT/SECOND: VELOCITY - GPU ASSET COOKER         ")
    print("======================================================")

    base_dir = r"D:\Codes\Win_codes\SSV"
    out_android = os.path.join(base_dir, "android", "app", "src", "main", "assets", "main_game.bin")
    out_desktop = os.path.join(base_dir, "assets", "main_game.bin")

    for target_path in [out_android, out_desktop]:
        print(f"\n--- Cooking target: {target_path} ---")
        cooker = SSVPakWriter(target_path)

        # 1. Cook translated GLSL shaders
        shader_dir = os.path.join(base_dir, "assets", "shaders")
        if os.path.exists(shader_dir):
            for sfile in os.listdir(shader_dir):
                path = os.path.join(shader_dir, sfile)
                if os.path.isfile(path):
                    with open(path, 'r', encoding='utf-8') as f:
                        cooker.add_shader(f"shaders/{sfile}", f.read())

        # 2. Cook original Musclecar_01 vehicle mesh & texture
        cook_musclecar(cooker, base_dir)

        cooker.build()

if __name__ == '__main__':
    main()
