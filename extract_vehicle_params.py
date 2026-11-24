import pefile, capstone, struct, re

pe = pefile.PE(r"D:\Codes\Win_codes\Split.Second.Velocity\SplitSecond.exe")
md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)

rva = 0x0082a350 - 0x00400000
code = pe.get_data(rva, 2500)

current_param = "Unknown"
for insn in md.disasm(code, 0x0082a350):
    # Check if pushing string pointer
    if insn.mnemonic == 'push' and insn.op_str.startswith('0x'):
        target = int(insn.op_str, 16)
        if 0x00BA0000 <= target <= 0x00D00000:
            try:
                s = pe.get_data(target - 0x00400000, 40).split(b'\x00')[0].decode('latin1', 'ignore')
                if len(s) > 1 and all(32 <= ord(c) <= 126 for c in s):
                    current_param = s
            except: pass
    
    # Check if loading float
    if 'fld' in insn.mnemonic and '[' in insn.op_str:
        m = re.search(r'0x([0-9a-f]+)', insn.op_str)
        if m:
            target = int(m.group(1), 16)
            if 0x00BA0000 <= target <= 0x00D00000:
                try:
                    fval = struct.unpack('<f', pe.get_data(target - 0x00400000, 4))[0]
                    print(f"Param: {current_param:30} -> Float at 0x{target:08X} = {fval}")
                except: pass
