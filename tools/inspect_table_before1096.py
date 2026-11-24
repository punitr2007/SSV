import struct

with open(r'D:\Codes\Win_codes\SSV\assets\extracted\Musclecar_01\Musclecar_01_High.model', 'rb') as f:
    f.seek(500)
    data = f.read(600)

print("uint32s in 500..1100:")
u32s = struct.unpack(f'<{len(data)//4}I', data)
for i in range(0, len(u32s), 4):
    off = 500 + i*4
    print(f"  {off:4d} (0x{off:04X}): {[f'0x{x:08X} ({x})' for x in u32s[i:i+4]]}")
