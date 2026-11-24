import struct

with open(r'D:\Codes\Win_codes\SSV\assets\extracted\Musclecar_01\Musclecar_01_High.model', 'rb') as f:
    f.seek(9480)
    ibo_bytes = f.read(111476 - 9480)

indices = struct.unpack(f'<{len(ibo_bytes)//2}H', ibo_bytes)
print(f"Total indices: {len(indices)}")

# Check min, max index
print(f"Min index: {min(indices)}, Max index: {max(indices)}")

# Check for 0xFFFF restart indices
restart_count = indices.count(0xFFFF)
print(f"0xFFFF restart count: {restart_count}")

# Check for runs of degenerates or strip breaks
strips = []
cur_strip = []
for i in range(len(indices)):
    idx = indices[i]
    if idx == 0xFFFF:
        if cur_strip:
            strips.append(cur_strip)
            cur_strip = []
    else:
        cur_strip.append(idx)
if cur_strip:
    strips.append(cur_strip)

print(f"Number of strips by 0xFFFF: {len(strips)}")

# If no 0xFFFF, check for degenerate repeats (e.g. idx == next_idx)
degen_breaks = []
for i in range(len(indices) - 2):
    if indices[i] == indices[i+1] and indices[i+1] == indices[i+2]:
        degen_breaks.append(i)

print(f"Number of triple repeated index breaks: {len(degen_breaks)}")
