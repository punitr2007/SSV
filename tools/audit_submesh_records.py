import struct

model_path = r'D:\Codes\Win_codes\SSV\assets\extracted\Musclecar_01\Musclecar_01_High.model'
with open(model_path, 'rb') as f:
    data = f.read()

# Let's inspect records from 5852 to 6140
print("Detailed breakdown of records from 5852 to 6140:")
total_f3 = 0
for off in range(5852, 6140, 16):
    f1, f2, f3, f4 = struct.unpack('<4I', data[off:off+16])
    print(f"Offset 0x{off:04X} ({off}): f1={f1:8d} (0x{f1:08X}), f2={f2:8d} (0x{f2:08X}), f3={f3:8d} (0x{f3:08X}), f4={f4:8d} (0x{f4:08X})")
    if f3 < 50000:
        total_f3 += f3

print(f"Total f3 sum: {total_f3}")
