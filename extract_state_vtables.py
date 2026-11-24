import pefile
import capstone
import struct

pe_path = r"D:\Codes\Win_codes\Split.Second.Velocity\SplitSecond.exe"
pe = pefile.PE(pe_path)
image_base = pe.OPTIONAL_HEADER.ImageBase

md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
rdata_sec = [s for s in pe.sections if s.Name.decode('utf-8', errors='ignore').strip('\x00') == '.rdata'][0]
data_sec = [s for s in pe.sections if s.Name.decode('utf-8', errors='ignore').strip('\x00') == '.data'][0]
text_sec = [s for s in pe.sections if s.Name.decode('utf-8', errors='ignore').strip('\x00') == '.text'][0]

rdata = rdata_sec.get_data()
data = data_sec.get_data()

print("=== 1. DISASSEMBLING 0x00405C00 ===")
d = pe.get_data(0x00405C00 - image_base, 120)
for insn in md.disasm(d, 0x00405C00):
    print(f"  0x{insn.address:08X}: {insn.mnemonic:<8} {insn.op_str}")

print("\n=== 2. EXACT GameStates VTABLES ===")
# List of known RTTI strings in .data
states = [
    ("cGameState", b".?AVcGameState@GameStates@@"),
    ("cBootupState", b".?AVcBootupState@GameStates@@"),
    ("cIntroState", b".?AVcIntroState@GameStates@@"),
    ("cIntroSequence", b".?AVcIntroSequence@GameStates@@"),
    ("cFrontendLoadState", b".?AVcFrontendLoadState@GameStates@@"),
    ("cFrontendRunState", b".?AVcFrontendRunState@GameStates@@"),
    ("cInGameLoadState", b".?AVcInGameLoadState@GameStates@@"),
    ("cInGameRunState", b".?AVcInGameRunState@GameStates@@"),
    ("cInGameMovieState", b".?AVcInGameMovieState@GameStates@@"),
    ("cLoadDemoState", b".?AVcLoadDemoState@GameStates@@")
]

for name, s_bytes in states:
    s_idx = data.find(s_bytes)
    if s_idx != -1:
        # TypeDescriptor starts 8 bytes before string (vtable ptr, spare ptr)
        td_va = image_base + data_sec.VirtualAddress + s_idx - 8
        # Look for td_va in rdata (COL field pTypeDescriptor)
        pat = struct.pack("<I", td_va)
        col_offset = rdata.find(pat)
        if col_offset != -1:
            col_va = image_base + rdata_sec.VirtualAddress + col_offset - 12
            # Look for col_va in rdata (vtable[-1] = col_va)
            col_pat = struct.pack("<I", col_va)
            vt_offset = rdata.find(col_pat)
            if vt_offset != -1:
                vtable_va = image_base + rdata_sec.VirtualAddress + vt_offset + 4
                print(f"\nState Class: {name}")
                print(f"  TypeDescriptor VA: 0x{td_va:08X}")
                print(f"  COL VA:            0x{col_va:08X}")
                print(f"  VTABLE VA:         0x{vtable_va:08X}")
                # Read vtable entries
                vt_rva = vtable_va - image_base
                methods = struct.unpack("<12I", pe.get_data(vt_rva, 48))
                for idx, m in enumerate(methods):
                    print(f"    vtable[{idx:2d}] = 0x{m:08X}")
            else:
                print(f"{name}: found COL at 0x{col_va:08X} but no vtable pointer")
        else:
            print(f"{name}: td_va 0x{td_va:08X} not found in rdata")

