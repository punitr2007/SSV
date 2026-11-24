import pefile
import capstone
import struct
import os

pe_path = r"D:\Codes\Win_codes\Split.Second.Velocity\SplitSecond.exe"
pe = pefile.PE(pe_path)
image_base = pe.OPTIONAL_HEADER.ImageBase

md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
rdata_sec = [s for s in pe.sections if s.Name.decode('utf-8', errors='ignore').strip('\x00') == '.rdata'][0]
data_sec = [s for s in pe.sections if s.Name.decode('utf-8', errors='ignore').strip('\x00') == '.data'][0]
text_sec = [s for s in pe.sections if s.Name.decode('utf-8', errors='ignore').strip('\x00') == '.text'][0]

rdata = rdata_sec.get_data()
data = data_sec.get_data()

print("=== 1. ArkLoader & ArkFactory VTABLES ===")
ark_classes = [
    ("ArkFactory", b".?AVArkFactory@@"),
    ("ArkLoader", b".?AVArkLoader@@"),
    ("ArkFileFinder", b".?AVArkFileFinder@@"),
    ("ParasiteLoader", b".?AVParasiteLoader@@"),
    ("lzssUnpackFactory", b".?AVlzssUnpackFactory@@"),
    ("WinFileFactory", b".?AVWinFileFactory@@")
]

for name, s_bytes in ark_classes:
    s_idx = data.find(s_bytes)
    if s_idx != -1:
        td_va = image_base + data_sec.VirtualAddress + s_idx - 8
        pat = struct.pack("<I", td_va)
        col_offset = rdata.find(pat)
        if col_offset != -1:
            col_va = image_base + rdata_sec.VirtualAddress + col_offset - 12
            vt_pat = struct.pack("<I", col_va)
            vt_offset = rdata.find(vt_pat)
            if vt_offset != -1:
                vtable_va = image_base + rdata_sec.VirtualAddress + vt_offset + 4
                print(f"\nClass: {name}")
                print(f"  TypeDesc VA: 0x{td_va:08X}, COL VA: 0x{col_va:08X}, VTABLE: 0x{vtable_va:08X}")
                vt_rva = vtable_va - image_base
                methods = struct.unpack("<8I", pe.get_data(vt_rva, 32))
                for idx, m in enumerate(methods):
                    print(f"    vtable[{idx}] = 0x{m:08X}")

print("\n=== 2. PARSING resident.ark HEADER & ENTRIES ===")
ark_file = r"D:\Codes\Win_codes\Split.Second.Velocity\data\resident.ark"
if os.path.exists(ark_file):
    with open(ark_file, "rb") as f:
        # Read header
        header = f.read(64)
        # 0x00: version / magic? (0x00010000 = 1.0)
        # 0x04: header size / TOC offset?
        # 0x08: entry count?
        fields = struct.unpack("<16I", header)
        print("Header 16 uint32s:")
        for i, val in enumerate(fields):
            print(f"  [{i:2d}] 0x{val:08X} ({val})")
        
        toc_offset = fields[3] # 0x10 = 16 bytes? Or fields[1] = 0x2174c = 137036 bytes?
        # Let's inspect data around 0x10 and 0x20
        f.seek(0)
        sample = f.read(512)
        # Search for filenames or string table in ark
        f.seek(0)
        first_mb = f.read(1024 * 1024)
        # find ascii strings > 4 chars
        strings = []
        for m in re.finditer(rb"[a-zA-Z0-9_\-\.\/\\]{5,}\x00", first_mb[:65536]):
            strings.append((m.start(), m.group(0).decode('utf-8', errors='ignore').strip('\x00')))
        print(f"\nStrings in first 64KB of {os.path.basename(ark_file)}: {len(strings)}")
        for off, s in strings[:15]:
            print(f"  Offset 0x{off:06X}: {s}")

