import pefile
import capstone
import struct

pe_path = r"D:\Codes\Win_codes\Split.Second.Velocity\SplitSecond.exe"
pe = pefile.PE(pe_path)
image_base = pe.OPTIONAL_HEADER.ImageBase

md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)

def disasm_func(va, count=50):
    rva = va - image_base
    data = pe.get_data(rva, count * 6)
    c = 0
    for insn in md.disasm(data, va):
        print(f"  0x{insn.address:08X}: {insn.mnemonic:<8} {insn.op_str}")
        c += 1
        if c >= count:
            break

print("=== DISASSEMBLING ArkFactory::Create (0x005712B0) ===")
disasm_func(0x005712B0, 40)

print("\n=== DISASSEMBLING ArkLoader Init / Open (0x00571390) ===")
disasm_func(0x00571390, 50)

print("\n=== DISASSEMBLING lzssUnpackFactory (0x00572130) ===")
disasm_func(0x00572130, 40)

