import struct
import numpy as np

model_path = r"D:\Codes\Win_codes\SSV\assets\extracted\Musclecar_01\Musclecar_01_High.model"

with open(model_path, "rb") as f:
    data = f.read()

print(f"Total file size: {len(data)} bytes")

# Read Buffer Section Table at offset 0x0410 (1040)
# Let's inspect 0x0400 to 0x0460
table_offset = 1040
print("\n--- Buffer Section Table (from 1040) ---")
for i in range(12):
    off = table_offset + i * 16
    entry = data[off:off+16]
    vals = struct.unpack("<4I", entry)
    print(f"Slice {i:2d} (at {off}): offset={vals[0]:8d} (0x{vals[0]:06X}), size={vals[1]:8d} (0x{vals[1]:06X}), unk2={vals[2]:8d}, unk3={vals[3]:8d}")

# Let's inspect the buffers at 521070, 766830, 889710
offsets = [521070, 766830, 889710]
for idx, o in enumerate(offsets):
    print(f"\n--- Stream at offset {o} (0x{o:06X}) ---")
    chunk = data[o:o+128]
    print("First 64 bytes hex:", chunk[:64].hex())
    # Try interpreting as float16 (half precision)
    try:
        h = np.frombuffer(chunk[:64], dtype=np.float16)
        print("As float16:", h[:16])
    except Exception as e:
        print("float16 decode err:", e)
    # Try interpreting as float32
    try:
        f32 = np.frombuffer(chunk[:64], dtype=np.float32)
        print("As float32:", f32[:8])
    except Exception as e:
        print("float32 decode err:", e)
    # Try interpreting as int16
    try:
        i16 = np.frombuffer(chunk[:64], dtype=np.int16)
        print("As int16:", i16[:16])
    except Exception as e:
        pass
    # Try interpreting as uint8
    try:
        u8 = np.frombuffer(chunk[:32], dtype=np.uint8)
        print("As uint8:", u8[:16])
    except Exception as e:
        pass
