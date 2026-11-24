import pefile
import capstone
import os

pe_path = r"D:\Codes\Win_codes\Split.Second.Velocity\SplitSecond.exe"
pe = pefile.PE(pe_path)

ep_rva = pe.OPTIONAL_HEADER.AddressOfEntryPoint
ep_offset = pe.get_offset_from_rva(ep_rva)
ep_data = pe.get_data(ep_rva, 64)

md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)

print(f"Disassembly at EntryPoint (RVA 0x{ep_rva:08X}, File Offset 0x{ep_offset:08X}):")
for insn in md.disasm(ep_data, pe.OPTIONAL_HEADER.ImageBase + ep_rva):
    print(f"  0x{insn.address:08X}: {insn.mnemonic:<8} {insn.op_str}")

# Check first bytes of .text
text_sec = None
for s in pe.sections:
    if s.Name.decode('utf-8', errors='ignore').strip('\x00') == '.text':
        text_sec = s
        break

if text_sec:
    text_data = pe.get_data(text_sec.VirtualAddress, 64)
    print(f"\nDisassembly at .text start (RVA 0x{text_sec.VirtualAddress:08X}, File Offset 0x{text_sec.PointerToRawData:08X}):")
    for insn in md.disasm(text_data, pe.OPTIONAL_HEADER.ImageBase + text_sec.VirtualAddress):
        print(f"  0x{insn.address:08X}: {insn.mnemonic:<8} {insn.op_str}")
