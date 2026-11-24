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

print("=== DISASSEMBLING 0x00A17FE0 ===")
disasm_func(0x00A17FE0, 200)

print("\n=== DISASSEMBLING 0x00A18020 ===")
disasm_func(0x00A18020, 200)

print("\n=== INSPECTING VTABLE AT 0x00C4A618 ===")
rva_vt = 0x00C4A618 - image_base
vt_data = pe.get_data(rva_vt, 64)
import struct
for i in range(10):
    fn_ptr = struct.unpack_from("<I", vt_data, i*4)[0]
    print(f"  vtable[{i}] = 0x{fn_ptr:08X}")

# Check RTTI for 0x00C4A618 (RTTI Complete Object Locator is at vtable - 4)
col_ptr = struct.unpack("<I", pe.get_data(rva_vt - 4, 4))[0]
print(f"  RTTI Complete Object Locator: 0x{col_ptr:08X}")
if col_ptr > image_base:
    col_data = pe.get_data(col_ptr - image_base, 24)
    sig, offset, cd_offset, type_desc_rva, hier_rva = struct.unpack("<IIIII", col_data[:20])
    type_desc_va = image_base + type_desc_rva
    td_name = pe.get_string_at_rva(type_desc_rva + 8, 64)
    print(f"  Type Descriptor VA: 0x{type_desc_va:08X}, Class Name: {td_name.decode('utf-8', errors='ignore')}")

