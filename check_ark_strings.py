import pefile

pe_path = r"D:\Codes\Win_codes\Split.Second.Velocity\SplitSecond.exe"
pe = pefile.PE(pe_path)
image_base = pe.OPTIONAL_HEADER.ImageBase

rdata_sec = [s for s in pe.sections if s.Name.decode('utf-8', errors='ignore').strip('\x00') == '.rdata'][0]
rdata = rdata_sec.get_data()

for addr in [0x00BD2D10, 0x00BD2C40]:
    rva = addr - image_base
    s = pe.get_string_at_rva(rva, 120)
    print(f"0x{addr:08X}: {s.decode('utf-8', errors='ignore')}")

