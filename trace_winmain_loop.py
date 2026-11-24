import pefile
import capstone

pe_path = r"D:\Codes\Win_codes\Split.Second.Velocity\SplitSecond.exe"
pe = pefile.PE(pe_path)
image_base = pe.OPTIONAL_HEADER.ImageBase

md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)

def disasm_range(start_va, count=120):
    rva = start_va - image_base
    data = pe.get_data(rva, count * 6)
    c = 0
    for insn in md.disasm(data, start_va):
        print(f"  0x{insn.address:08X}: {insn.mnemonic:<8} {insn.op_str}")
        c += 1
        if c >= count:
            break

print("=== DISASSEMBLING WinMain 0x00A175CD - 0x00A17750 ===")
disasm_range(0x00A175CD, 100)

