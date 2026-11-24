import pefile
import capstone
import struct
import re

pe_path = r"D:\Codes\Win_codes\Split.Second.Velocity\SplitSecond.exe"
pe = pefile.PE(pe_path)
image_base = pe.OPTIONAL_HEADER.ImageBase
md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)

print("=== 1. Searching for .model strings in .rdata ===")
rdata = [s for s in pe.sections if s.Name.decode('ascii', 'ignore').strip('\x00') == '.rdata'][0]
data = [s for s in pe.sections if s.Name.decode('ascii', 'ignore').strip('\x00') == '.data'][0]
text = [s for s in pe.sections if s.Name.decode('ascii', 'ignore').strip('\x00') == '.text'][0]

rdata_bytes = rdata.get_data()
text_bytes = text.get_data()

for m in re.finditer(rb'\.model\b', rdata_bytes, re.IGNORECASE):
    va = image_base + rdata.VirtualAddress + m.start()
    s = rdata_bytes[max(0, m.start()-20):m.start()+20]
    print(f"Found at VA 0x{va:08X}: {s}")

# Find references in code to DrawIndexedPrimitive
# In D3D9, DrawIndexedPrimitive is IDirect3DDevice9::DrawIndexedPrimitive (vtable index 82 = 0x52 * 4 = 0x148)
print("\n=== 2. Searching for DrawIndexedPrimitive calls ([eax + 0x148] or [edx + 0x148]) ===")
# call dword ptr [eax + 0x148] is: FF 90 48 01 00 00 or FF 92 48 01 00 00 or FF 91 ...
dip_patterns = [
    (b'\xFF\x90\x48\x01\x00\x00', 'call [eax + 0x148]'),
    (b'\xFF\x91\x48\x01\x00\x00', 'call [ecx + 0x148]'),
    (b'\xFF\x92\x48\x01\x00\x00', 'call [edx + 0x148]'),
    (b'\xFF\x50\x52', 'call [eax + 0x52]'),
]

for pat, desc in dip_patterns:
    idx = 0
    while True:
        pos = text_bytes.find(pat, idx)
        if pos == -1: break
        call_va = image_base + text.VirtualAddress + pos
        print(f"  {desc} at 0x{call_va:08X}")
        idx = pos + 1
