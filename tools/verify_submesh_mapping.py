import struct

model_path = r'D:\Codes\Win_codes\SSV\assets\extracted\Musclecar_01\Musclecar_01_High.model'
with open(model_path, 'rb') as f:
    data = f.read()

# IBO offset = 9480, size = 101996
ibo_bytes = data[9480:111476]
indices = struct.unpack(f'<{len(ibo_bytes)//2}H', ibo_bytes)

# VBO offset = 111484, size = 408072
vbo_bytes = data[111484:111484+408072]
vertex_stride = 28 # pos(8), deform(8), uv(4), norm(6), pad(2)
total_verts = len(vbo_bytes) // vertex_stride
print(f"Total vertices in VBO: {total_verts}")

# Submesh table records from 5852 to 6124
submeshes = []
for off in range(5852, 6124, 16):
    v_byte_off, f2, idx_count, f4 = struct.unpack('<4I', data[off:off+16])
    v_start = v_byte_off // vertex_stride
    submeshes.append({
        'table_offset': off,
        'v_byte_off': v_byte_off,
        'v_start': v_start,
        'idx_count': idx_count,
        'flags': (f2, f4)
    })

print(f"\nParsed {len(submeshes)} submeshes:")
for i, sm in enumerate(submeshes):
    print(f"  Submesh [{i:2d}]: v_byte_off={sm['v_byte_off']:8d} (v_start={sm['v_start']:6d}), idx_count={sm['idx_count']:6d}")
