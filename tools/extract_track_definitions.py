import struct
import os

def lzss_decompress(comp_data, uncomp_size):
    # Okumura LZSS sliding window (4096B window, 18B max match, F=18, N=4096)
    N = 4096
    F = 18
    THRESHOLD = 2
    text_buf = bytearray(N + F - 1)
    # Init text_buf with spaces (0x20)
    for i in range(N - F):
        text_buf[i] = 0x20
    r = N - F
    flags = 0
    in_idx = 0
    out_buf = bytearray()

    while in_idx < len(comp_data) and len(out_buf) < uncomp_size:
        flags >>= 1
        if (flags & 256) == 0:
            if in_idx >= len(comp_data): break
            c = comp_data[in_idx]
            in_idx += 1
            flags = c | 0xFF00

        if flags & 1:
            if in_idx >= len(comp_data): break
            c = comp_data[in_idx]
            in_idx += 1
            out_buf.append(c)
            text_buf[r] = c
            r = (r + 1) & (N - 1)
        else:
            if in_idx + 1 >= len(comp_data): break
            i = comp_data[in_idx]
            j = comp_data[in_idx + 1]
            in_idx += 2
            i |= (j & 0xF0) << 4
            j = (j & 0x0F) + THRESHOLD
            for k in range(j + 1):
                c = text_buf[(i + k) & (N - 1)]
                out_buf.append(c)
                text_buf[r] = c
                r = (r + 1) & (N - 1)
                if len(out_buf) >= uncomp_size: break

    return bytes(out_buf)

def brs_hash(s):
    h = 0
    for c in s:
        h = ((h * 31) + ord(c.lower())) & 0xFFFFFFFF
    return h

ark_path = r"D:\Codes\Win_codes\Split.Second.Velocity\data\resident.ark"
targets = {
    brs_hash("UI/Logic/Season.xml"): "UI_Logic_Season.xml",
    brs_hash("UI/Logic/TrackFiltration.xml"): "UI_Logic_TrackFiltration.xml"
}

with open(ark_path, "rb") as f:
    f.seek(12)
    toc_off = struct.unpack("<I", f.read(4))[0]
    f.seek(8)
    count = struct.unpack("<I", f.read(4))[0]
    f.seek(toc_off)
    
    entries = []
    for _ in range(count):
        off, name_hash, flags, uncomp, comp = struct.unpack("<5I", f.read(20))
        if name_hash in targets:
            entries.append((targets[name_hash], off, uncomp, comp, flags))

out_dir = r"D:\Codes\Win_codes\SSV\assets\extracted"
os.makedirs(out_dir, exist_ok=True)

with open(ark_path, "rb") as f:
    for name, off, uncomp, comp, flags in entries:
        f.seek(off)
        comp_data = f.read(comp)
        if flags & 1:
            raw = lzss_decompress(comp_data, uncomp)
        else:
            raw = comp_data
        
        target_file = os.path.join(out_dir, name)
        with open(target_file, "wb") as out_f:
            out_f.write(raw)
        print(f"Extracted {name} ({len(raw)} bytes) -> {target_file}")
        
        # Print first 25 lines of the XML
        text = raw.decode('latin1', errors='ignore')
        lines = text.splitlines()
        print(f"\n--- Preview of {name} (first 25 lines) ---")
        for line in lines[:25]:
            print(" ", line)
