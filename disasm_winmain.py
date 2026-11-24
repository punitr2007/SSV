import pefile
import capstone

pe_path = r"D:\Codes\Win_codes\Split.Second.Velocity\SplitSecond.exe"
pe = pefile.PE(pe_path)
image_base = pe.OPTIONAL_HEADER.ImageBase

md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)

def disasm_func(va, max_len=600):
    rva = va - image_base
    data = pe.get_data(rva, max_len)
    instructions = list(md.disasm(data, va))
    for insn in instructions:
        print(f"  0x{insn.address:08X}: {insn.mnemonic:<8} {insn.op_str}")
        if insn.mnemonic == 'ret':
            break

print("=== DISASSEMBLY OF WinMain (0x00A172B0) ===")
disasm_func(0x00A172B0, 500)

