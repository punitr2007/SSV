import struct

with open(r'D:\Codes\Win_codes\SSV\assets\extracted\Musclecar_01\Musclecar_01_High.model', 'rb') as f:
    f.seek(5500)
    data = f.read(1500)

print("Offset 5500..7000 (16-byte records):")
for i in range(64, 64 + 42*16, 16):
    rec = data[i:i+16]
    off = 5500 + i
    u32s = struct.unpack('<4I', rec)
    u16s = struct.unpack('<8H', rec)
    print(f"  [{off:4d} / 0x{off:04X}]: u32={u32s} | u16={u16s}")
