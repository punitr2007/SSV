with open(r'D:\Codes\Win_codes\SSV\assets\extracted\Musclecar_01\Musclecar_01_High.model', 'rb') as f:
    f.seek(1080)
    data = f.read(400)

print("Offset 1080..1480:")
for i in range(0, 400, 16):
    chunk = data[i:i+16]
    hex_str = chunk.hex(' ')
    print(f"  {1080+i:4d} (0x{1080+i:04X}): {hex_str}")
