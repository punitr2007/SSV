import struct

with open(r'D:\Codes\Win_codes\SSV\assets\extracted\Musclecar_01\Musclecar_01_High.model', 'rb') as f:
    hdr_data = f.read(9480)

# Check degenerate pairs in index buffer
with open(r'D:\Codes\Win_codes\SSV\assets\extracted\Musclecar_01\Musclecar_01_High.model', 'rb') as f:
    f.seek(9480)
    ibo = struct.unpack(f'<{50998}H', f.read(50998*2))

degen_pairs = []
for i in range(len(ibo) - 1):
    if ibo[i] == ibo[i+1]:
        degen_pairs.append(i)

print(f"Number of degenerate pairs (ibo[i] == ibo[i+1]): {len(degen_pairs)}")
print(f"First 20 degenerate pair locations: {degen_pairs[:20]}")

# Let's see if the strip lengths between degenerate pairs match any numbers in header!
strip_lens = []
last = 0
for p in degen_pairs:
    if p > last:
        strip_lens.append(p - last)
    last = p + 2
print(f"Total strips separated by degenerates: {len(strip_lens)}")
print(f"First 10 strip lengths: {strip_lens[:10]}")

# Now let's search hdr_data (0..9480) for integers that match these strip lengths or index offsets!
print("\nSearching hdr_data for strip lengths:")
for idx, l in enumerate(strip_lens[:10]):
    for off in range(0, 9480 - 4, 4):
        val = struct.unpack('<I', hdr_data[off:off+4])[0]
        if val == l:
            print(f"  Strip {idx} length {l} found in header at offset 0x{off:04X} ({off})")
