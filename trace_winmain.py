import pefile
import capstone
import struct
import os

pe_path = r"D:\Codes\Win_codes\Split.Second.Velocity\SplitSecond.exe"
pe = pefile.PE(pe_path)
image_base = pe.OPTIONAL_HEADER.ImageBase

md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)

def disasm_range(va, size):
    rva = va - image_base
    data = pe.get_data(rva, size)
    for insn in md.disasm(data, va):
        print(f"  0x{insn.address:08X}: {insn.mnemonic:<8} {insn.op_str}")

print("=== 1. DISASSEMBLING CRT STARTUP (0x00756000 - 0x00756200) ===")
# Let's inspect 0x00756000 to 0x00756200
rva = 0x00756000 - image_base
data = pe.get_data(rva, 512)
winmain_call = None
for insn in md.disasm(data, 0x00756000):
    print(f"  0x{insn.address:08X}: {insn.mnemonic:<8} {insn.op_str}")
    # In MSVC, WinMain call is preceded by 4 pushes (nShowCmd, lpCmdLine, hPrevInstance, hInstance)
    if insn.mnemonic == 'call' and winmain_call is None and insn.address > 0x00756120:
        winmain_call = insn.op_str

print(f"\nProbable WinMain: {winmain_call}")

# Also check before 0x00756000 to find WinMainCRTStartup entry
print("\n=== 2. FINDING WinMainCRTStartup (around 0x00755F50) ===")
disasm_range(0x00755F50, 180)

