import pefile
import struct

pe_path = r"D:\Codes\Win_codes\Split.Second.Velocity\SplitSecond.exe"
pe = pefile.PE(pe_path)
image_base = pe.OPTIONAL_HEADER.ImageBase

rdata_sec = [s for s in pe.sections if s.Name.decode('utf-8', errors='ignore').strip('\x00') == '.rdata'][0]
data_sec = [s for s in pe.sections if s.Name.decode('utf-8', errors='ignore').strip('\x00') == '.data'][0]
rdata = rdata_sec.get_data()
data = data_sec.get_data()

for vt_va in [0x00BA0864, 0x00BA08BC]:
    # In 32-bit MSVC, COL pointer is at vtable - 4
    col_ptr = struct.unpack("<I", pe.get_data(vt_va - 4 - image_base, 4))[0]
    # COL: signature(4), offset(4), cdOffset(4), pTypeDescriptor(4)
    td_ptr = struct.unpack("<I", pe.get_data(col_ptr + 12 - image_base, 4))[0]
    name = pe.get_string_at_rva(td_ptr + 8 - image_base, 64)
    print(f"Vtable 0x{vt_va:08X}: Class = {name.decode('utf-8', errors='ignore')}")

