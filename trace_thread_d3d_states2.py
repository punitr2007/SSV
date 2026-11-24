import pefile
import capstone
import struct

pe_path = r"D:\Codes\Win_codes\Split.Second.Velocity\SplitSecond.exe"
pe = pefile.PE(pe_path)
image_base = pe.OPTIONAL_HEADER.ImageBase

md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
text_sec = [s for s in pe.sections if s.Name.decode('utf-8', errors='ignore').strip('\x00') == '.text'][0]
rdata_sec = [s for s in pe.sections if s.Name.decode('utf-8', errors='ignore').strip('\x00') == '.rdata'][0]
text = text_sec.get_data()
rdata = rdata_sec.get_data()

print("=== 1. DISASSEMBLING GAME THREAD PROC (0x00405E90) ===")
d = pe.get_data(0x00405E90 - image_base, 180)
for insn in md.disasm(d, 0x00405E90):
    print(f"  0x{insn.address:08X}: {insn.mnemonic:<8} {insn.op_str}")

print("\n=== 2. CALLS TO Direct3DCreate9 THUNK (0x00B189BA) ===")
# search for call 0x00B189BA (relative call: E8 <disp32>)
# disp32 = 0x00B189BA - (call_va + 5)
idx = 0
found_d3d_calls = []
while True:
    idx = text.find(b"\xe8", idx)
    if idx == -1:
        break
    if idx + 5 <= len(text):
        disp = struct.unpack("<i", text[idx+1:idx+5])[0]
        cur_va = image_base + text_sec.VirtualAddress + idx
        target_va = cur_va + 5 + disp
        if target_va == 0x00B189BA:
            found_d3d_calls.append(cur_va)
    idx += 1

print(f"Calls to Direct3DCreate9 thunk at: {[hex(x) for x in found_d3d_calls]}")
for cva in found_d3d_calls:
    print(f"\nContext around {hex(cva)}:")
    d_ctx = pe.get_data(cva - 25 - image_base, 60)
    for insn in md.disasm(d_ctx, cva - 25):
        print(f"  0x{insn.address:08X}: {insn.mnemonic:<8} {insn.op_str}")

print("\n=== 3. SEARCHING REFERENCES TO GameStates STRINGS ===")
# Search where the string ".?AVcBootupState@GameStates@@" is referenced in the whole PE
for s_name in [".?AVcBootupState@GameStates@@", ".?AVcInGameRunState@GameStates@@", ".?AVcGameState@GameStates@@"]:
    s_idx = rdata.find(s_name.encode('utf-8'))
    if s_idx != -1:
        s_va = image_base + rdata_sec.VirtualAddress + s_idx
        print(f"String '{s_name}' at 0x{s_va:08X}")
        # Search for s_va in all sections
        for s in pe.sections:
            sec_name = s.Name.decode('utf-8', errors='ignore').strip('\x00')
            sec_data = s.get_data()
            pat = struct.pack("<I", s_va)
            f_idx = 0
            while True:
                f_idx = sec_data.find(pat, f_idx)
                if f_idx == -1:
                    break
                ref_va = image_base + s.VirtualAddress + f_idx
                print(f"  Referenced in section {sec_name} at 0x{ref_va:08X}")
                f_idx += 4

