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
        if insn.mnemonic == 'ret' and insn.address > va + 10:
            break

print("=== DISASSEMBLING 0x00405C80 ===")
disasm_func(0x00405C80, 500)

print("\n=== DISASSEMBLING 0x0044F970 (MESSAGE PUMP) ===")
disasm_func(0x0044F970, 300)

