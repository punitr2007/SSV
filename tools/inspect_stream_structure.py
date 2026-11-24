import struct
import os

def inspect_header_fields(path):
    print(f"\n==========================================")
    print(f"Inspecting: {os.path.basename(path)}")
    print(f"==========================================")
    with open(path, 'rb') as f:
        data = f.read()

    # Read the full 9052 bytes or header
    print(f"Total size: {len(data)}")
    u32s = struct.unpack(f'<{min(len(data)//4, 100)}I', data[:min(len(data), 400)])
    print("Header uint32s (first 40):")
    for i in range(0, 40, 4):
        print(f"  [{i:2d}..{i+3:2d}]: {[f'0x{u32s[j]:08X} ({u32s[j]})' for j in range(i, i+4)]}")

    # Search for pointers to 80-byte records or submesh tables
    # In Musclecar_01.model, table at 3156 had (1, 23, 2288, 2368, 2448, ...)
    # What is at byte 0x00..0x38 in Musclecar_01.model?
    # Let's inspect bytes 56..2288
    # Notice: 2288 - 56 = 2232 bytes.
    # 2232 / 72 = 31 records? Or 2232 / 41 = 54.4?
    # Let's check what repeats between 56 and 2288.

inspect_header_fields(r'D:\Codes\Win_codes\SSV\assets\extracted\Musclecar_01\Musclecar_01.model')
inspect_header_fields(r'D:\Codes\Win_codes\SSV\assets\extracted\Musclecar_01\Musclecar_01_High.model')
