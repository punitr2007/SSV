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

print("=== DISASSEMBLING WinMain 0x00A174A0 - 0x00A17650 ===")
disasm_range(0x00A174A0, 100)

print("\n=== RESOLVING STRINGS & IAT ENTRIES ===")
ptrs = [0xc4aa64, 0xc4aa7c, 0xc0dba4, 0xc0db9c, 0xc0db8c, 0xc4aa4c]
for p in ptrs:
    try:
        rva = p - image_base
        s = pe.get_string_at_rva(rva, 64)
        print(f"  0x{p:08X}: {s.decode('utf-8', errors='ignore')}")
    except Exception as e:
        print(f"  0x{p:08X}: error {e}")

iat_ptrs = [0xb9b14c, 0xb9b188, 0xb9b020, 0xb9b024]
for p in iat_ptrs:
    for entry in pe.DIRECTORY_ENTRY_IMPORT:
        for imp in entry.imports:
            if imp.address == p:
                name = imp.name.decode('utf-8', errors='ignore') if imp.name else str(imp.ordinal)
                print(f"  IAT 0x{p:08X}: {entry.dll.decode('utf-8', errors='ignore')}!{name}")

