import pefile
import capstone
import struct
import os

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

print("=== 1. DISASSEMBLING 0x00405FA0 (CORE RUN/LOOP) ===")
disasm_func(0x00405FA0, 300)

print("\n=== 2. SEARCHING FOR PeekMessage / DispatchMessage CALLS IN .text ===")
user32_msgs = {}
for entry in pe.DIRECTORY_ENTRY_IMPORT:
    if entry.dll.decode('utf-8', errors='ignore').upper() == 'USER32.DLL':
        for imp in entry.imports:
            if imp.name and imp.name.decode('utf-8', errors='ignore') in ['PeekMessageA', 'PeekMessageW', 'GetMessageA', 'DispatchMessageA', 'TranslateMessage']:
                user32_msgs[imp.name.decode('utf-8', errors='ignore')] = imp.address

print(f"USER32 message APIs in IAT: {user32_msgs}")

text_sec = [s for s in pe.sections if s.Name.decode('utf-8', errors='ignore').strip('\x00') == '.text'][0]
text = text_sec.get_data()

for api_name, iat_addr in user32_msgs.items():
    pat = struct.pack("<I", iat_addr)
    idx = 0
    refs = []
    while True:
        idx = text.find(pat, idx)
        if idx == -1:
            break
        if idx >= 2 and text[idx-2:idx] == b"\xff\x15":
            va = image_base + text_sec.VirtualAddress + idx - 2
            refs.append(va)
        idx += 4
    print(f"Calls to {api_name} at: {[hex(x) for x in refs]}")
    for ref in refs:
        print(f"\n--- Context around {hex(ref)} ({api_name}) ---")
        rva = ref - 20 - image_base
        d = pe.get_data(rva, 60)
        for insn in md.disasm(d, ref - 20):
            print(f"    0x{insn.address:08X}: {insn.mnemonic:<8} {insn.op_str}")

print("\n=== 3. INSPECTING .ark ARCHIVE HEADERS ===")
ark_path = r"D:\Codes\Win_codes\Split.Second.Velocity\data\resident.ark"
if os.path.exists(ark_path):
    with open(ark_path, "rb") as f:
        hdr = f.read(256)
        print(f"File size: {os.path.getsize(ark_path)} bytes")
        print(f"First 32 bytes (hex): {hdr[:32].hex()}")
        print(f"First 32 bytes (ascii): {repr(hdr[:32])}")
        # Check if there is magic like 'ARK', 'PACK', or struct
        magic = hdr[:4]
        print(f"Magic: {magic} (hex: {magic.hex()})")
        # Try unpacking standard header fields (32-bit uints)
        unp = struct.unpack("<8I", hdr[:32])
        print(f"Header fields as 8 uint32: {[hex(x) for x in unp]}")

