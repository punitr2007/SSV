import pefile
import capstone
import struct

pe_path = r"D:\Codes\Win_codes\Split.Second.Velocity\SplitSecond.exe"
pe = pefile.PE(pe_path)
image_base = pe.OPTIONAL_HEADER.ImageBase
md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
rdata = [s for s in pe.sections if s.Name.decode('ascii', 'ignore').strip('\x00') == '.rdata'][0]
text = [s for s in pe.sections if s.Name.decode('ascii', 'ignore').strip('\x00') == '.text'][0]

rdata_bytes = rdata.get_data()
text_bytes = text.get_data()

for target_str in [b'%s.model\x00', b'%s/%s/%s.model.stream\x00', b'%s_%s.model\x00']:
    pos = rdata_bytes.find(target_str)
    if pos != -1:
        str_va = image_base + rdata.VirtualAddress + pos
        print(f"\nTarget string: {target_str} at VA 0x{str_va:08X}")
        pat = struct.pack('<I', str_va)
        idx = 0
        while True:
            t_idx = text_bytes.find(pat, idx)
            if t_idx == -1: break
            ref_va = image_base + text.VirtualAddress + t_idx
            print(f"  Referenced in .text at VA 0x{ref_va:08X}")
            func_search = max(0, t_idx - 60)
            chunk = text_bytes[func_search:t_idx + 80]
            start_va = image_base + text.VirtualAddress + func_search
            for insn in md.disasm(chunk, start_va):
                if ref_va - 20 <= insn.address <= ref_va + 40:
                    print(f"    0x{insn.address:08X}: {insn.mnemonic:<8} {insn.op_str}")
            idx = t_idx + 1
