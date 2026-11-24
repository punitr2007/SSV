import struct
import os

def lzss_decompress(comp_data, expected_size):
    in_data = comp_data[4:] # Skip 4-byte header!
    in_size = len(in_data)
    N = 4096
    buf = bytearray(N)
    r = 0xFEE # 4096 - 18
    in_pos = 0
    flags = 0
    out = bytearray()
    while in_pos < in_size and len(out) < expected_size:
        flags >>= 1
        if (flags & 0x100) == 0:
            if in_pos >= in_size: break
            flags = in_data[in_pos] | 0xFF00
            in_pos += 1
        if flags & 1:
            if in_pos >= in_size: break
            c = in_data[in_pos]
            in_pos += 1
            out.append(c)
            buf[r] = c
            r = (r + 1) & (N - 1)
        else:
            if in_pos + 1 >= in_size: break
            b1 = in_data[in_pos]
            b2 = in_data[in_pos + 1]
            in_pos += 2
            match_pos = b1 | ((b2 & 0xF0) << 4)
            match_len = (b2 & 0x0F) + 3
            for k in range(match_len):
                if len(out) >= expected_size: break
                c = buf[(match_pos + k) & (N - 1)]
                out.append(c)
                buf[r] = c
                r = (r + 1) & (N - 1)
    return bytes(out)

def brs_hash(s):
    h = 0
    for c in s:
        h = ((h * 31) + ord(c.lower())) & 0xFFFFFFFF
    return h

ark_path = r"D:\Codes\Win_codes\Split.Second.Velocity\data\resident.ark"
with open(ark_path, "rb") as f:
    f.seek(8)
    count1, off1, count2, off2 = struct.unpack("<4I", f.read(16))
    f.seek(8 + off1)
    toc = {}
    for i in range(count1):
        flg, off, csz, usz, h = struct.unpack("<5I", f.read(20))
        toc[h] = (flg, off, csz, usz)

for target in ["UI/Logic/Season.xml", "UI/Logic/TrackFiltration.xml"]:
    h = brs_hash(target)
    if h in toc:
        flg, off, csz, usz = toc[h]
        with open(ark_path, "rb") as f:
            f.seek(off)
            raw = f.read(csz)
        data = lzss_decompress(raw, usz) if (flg & 1) else raw
        out_path = os.path.join(r"D:\Codes\Win_codes\SSV\assets\extracted", os.path.basename(target))
        with open(out_path, "wb") as out_f:
            out_f.write(data)
        print(f"Extracted {target} ({len(data)} bytes) -> {out_path}")
        print("First 300 chars:")
        print(data[:300].decode("latin1"))
        print("--------------------------------------------------")
