import pefile
import capstone
import struct

pe_path = r"D:\Codes\Win_codes\Split.Second.Velocity\SplitSecond.exe"
pe = pefile.PE(pe_path)
image_base = pe.OPTIONAL_HEADER.ImageBase

md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
text_sec = [s for s in pe.sections if s.Name.decode('utf-8', errors='ignore').strip('\x00') == '.text'][0]
rdata_sec = [s for s in pe.sections if s.Name.decode('utf-8', errors='ignore').strip('\x00') == '.rdata'][0]
data_sec = [s for s in pe.sections if s.Name.decode('utf-8', errors='ignore').strip('\x00') == '.data'][0]

text = text_sec.get_data()
rdata = rdata_sec.get_data()
data = data_sec.get_data()

print("=== 1. DISASSEMBLING CreateThread AT 0x00405F17 ===")
rva = 0x00405F00 - image_base
d = pe.get_data(rva, 64)
for insn in md.disasm(d, 0x00405F00):
    print(f"  0x{insn.address:08X}: {insn.mnemonic:<8} {insn.op_str}")

print("\n=== 2. REFERENCES TO Direct3DCreate9 (IAT 0x00B9B3BC) ===")
pat = struct.pack("<I", 0x00B9B3BC)
idx = 0
while True:
    idx = text.find(pat, idx)
    if idx == -1:
        break
    va = image_base + text_sec.VirtualAddress + idx
    print(f"  Found reference at 0x{va:08X}")
    # disassemble 30 bytes before and after
    d_ctx = pe.get_data(va - 15 - image_base, 35)
    for insn in md.disasm(d_ctx, va - 15):
        print(f"    0x{insn.address:08X}: {insn.mnemonic:<8} {insn.op_str}")
    idx += 4

print("\n=== 3. GameStates 32-bit MSVC VTABLES ===")
state_classes = [
    ".?AVcGameState@GameStates@@",
    ".?AVcBootupState@GameStates@@",
    ".?AVcIntroState@GameStates@@",
    ".?AVcFrontendLoadState@GameStates@@",
    ".?AVcFrontendRunState@GameStates@@",
    ".?AVcInGameLoadState@GameStates@@",
    ".?AVcInGameRunState@GameStates@@",
    ".?AVcInGameMovieState@GameStates@@",
    ".?AVcLoadDemoState@GameStates@@",
    ".?AVcIntroSequence@GameStates@@"
]

for cls in state_classes:
    idx = rdata.find(cls.encode('utf-8'))
    if idx != -1:
        td_va = image_base + rdata_sec.VirtualAddress + idx - 8
        pat = struct.pack("<I", td_va) # in 32-bit x86 MSVC, COL stores 32-bit VA of TypeDescriptor
        col_idx = 0
        while True:
            col_idx = rdata.find(pat, col_idx)
            if col_idx == -1:
                break
            # In 32-bit COL: signature(4), offset(4), cdOffset(4), pTypeDescriptor(4), pClassHierarchyDescriptor(4)
            if col_idx >= 12:
                col_va = image_base + rdata_sec.VirtualAddress + col_idx - 12
                # Look for col_va in rdata
                vt_pat = struct.pack("<I", col_va)
                vt_idx = rdata.find(vt_pat)
                if vt_idx != -1:
                    vtable_va = image_base + rdata_sec.VirtualAddress + vt_idx + 4
                    print(f"\nClass: {cls}")
                    print(f"  TypeDesc VA: 0x{td_va:08X}, COL VA: 0x{col_va:08X}, VTABLE: 0x{vtable_va:08X}")
                    vt_rva = vtable_va - image_base
                    vt_methods = struct.unpack("<8I", pe.get_data(vt_rva, 32))
                    for m_idx, m_va in enumerate(vt_methods):
                        print(f"    vtable[{m_idx}] = 0x{m_va:08X}")
                    break
            col_idx += 4

