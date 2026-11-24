import struct

with open(r'D:\Codes\Win_codes\SSV\assets\extracted\Musclecar_01\Musclecar_01_High.model', 'rb') as f:
    data = f.read()

# Let's inspect records around 5836 to 6140
print("=== SUBMESH RECORDS IN Musclecar_01_High.model ===")
for off in range(5836, 6140, 16):
    chunk = data[off:off+16]
    f1, f2, f3, f4 = struct.unpack('<4I', chunk)
    print(f"Offset 0x{off:04X} ({off}): f1=0x{f1:08X} ({f1:8d}), f2=0x{f2:08X} ({f2:8d}), f3=0x{f3:08X} ({f3:8d}), f4=0x{f4:08X} ({f4:8d})")
