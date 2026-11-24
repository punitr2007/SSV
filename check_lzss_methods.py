import pefile
import capstone
import struct

pe_path = r"D:\Codes\Win_codes\Split.Second.Velocity\SplitSecond.exe"
pe = pefile.PE(pe_path)
image_base = pe.OPTIONAL_HEADER.ImageBase

md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)

vt_rva = 0x00BA0864 - image_base
methods = struct.unpack("<10I", pe.get_data(vt_rva, 40))
for i, m in enumerate(methods):
    print(f"lzssLoader::vtable[{i}] = 0x{m:08X}")

# Disassemble Read / Decompress method (usually vtable[5] or vtable[6])
print("\n=== Disassembling lzssLoader::vtable[6] ===")
rva = methods[6] - image_base
d = pe.get_data(rva, 120)
for insn in md.disasm(d, methods[6]):
    print(f"  0x{insn.address:08X}: {insn.mnemonic:<8} {insn.op_str}")

