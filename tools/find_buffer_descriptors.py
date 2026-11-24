import struct

model_path = r"D:\Codes\Win_codes\SSV\assets\extracted\Musclecar_01\Musclecar_01_High.model"
with open(model_path, "rb") as f:
    data = f.read()

# Let's search for known buffer sizes or offsets
# IBO size is 101,996 (or 50,998 * 2)
# VBO size is 408,072 (14,574 * 28)
# Stream 1 size is 245,760 (15,360 * 16)
# Stream 2 size is 122,880 (15,360 * 8)
# Stream 3 size is 122,880 (15,360 * 8)

targets = [101996, 408072, 245760, 122880, 14574, 50998]
for t in targets:
    pos = 0
    found = []
    while True:
        pos = data.find(struct.pack("<I", t), pos)
        if pos == -1 or pos > 10000:
            break
        found.append(pos)
        pos += 4
    print(f"Target {t} (0x{t:X}) found at offsets: {found}")
