import struct
import os

def analyze_model_records(path):
    print(f"\n==========================================")
    print(f"Analyzing records in: {os.path.basename(path)}")
    print(f"==========================================")
    with open(path, 'rb') as f:
        data = f.read()

    # Search for (1, count) pattern or submesh offset tables
    # In Musclecar_01.model, table was at 3156: (1, 23, 2288, 2368, ...)
    # Let's search data for uint32 sequences: where val0 == 1 and 10 <= val1 <= 100
    for i in range(0, min(len(data), 10000) - 8, 4):
        u0, u1 = struct.unpack('<2I', data[i:i+8])
        if u0 == 1 and 10 <= u1 <= 60:
            # Check if followed by valid offsets within the file
            cand = struct.unpack(f'<{u1}I', data[i+8:i+8+u1*4])
            if all(0 <= x < len(data) for x in cand):
                print(f"Found Submesh Offset Table at offset {i} (0x{i:04X}): count={u1}")
                print(f"  Offsets: {cand[:10]}...")
                # Analyze each record pointed to
                for idx, rec_off in enumerate(cand):
                    rec = data[rec_off:rec_off+80]
                    u = struct.unpack('<20I', rec)
                    f_vals = struct.unpack('<20f', rec)
                    # print record
                    print(f"  Submesh [{idx:2d}] at 0x{rec_off:04X}:")
                    print(f"    u32: {u[:10]}")
                    print(f"    f32: {[round(x, 2) for x in f_vals[:10]]}")

analyze_model_records(r'D:\Codes\Win_codes\SSV\assets\extracted\Musclecar_01\Musclecar_01.model')
analyze_model_records(r'D:\Codes\Win_codes\SSV\assets\extracted\Musclecar_01\Musclecar_01_High.model')
