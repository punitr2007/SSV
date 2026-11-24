import pefile
import capstone
import struct

pe_path = r"D:\Codes\Win_codes\Split.Second.Velocity\SplitSecond.exe"
pe = pefile.PE(pe_path)
image_base = pe.OPTIONAL_HEADER.ImageBase

md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
text_sec = [s for s in pe.sections if s.Name.decode('utf-8', errors='ignore').strip('\x00') == '.text'][0]
text = text_sec.get_data()

def find_iat_calls(dll_match, func_names):
    results = {}
    for entry in pe.DIRECTORY_ENTRY_IMPORT:
        dll = entry.dll.decode('utf-8', errors='ignore')
        if dll_match.lower() in dll.lower():
            for imp in entry.imports:
                if imp.name:
                    fn = imp.name.decode('utf-8', errors='ignore')
                    if any(fn == target or target == "*" for target in func_names):
                        # search for calls in text
                        pat = struct.pack("<I", imp.address)
                        refs = []
                        idx = 0
                        while True:
                            idx = text.find(pat, idx)
                            if idx == -1:
                                break
                            if idx >= 2 and text[idx-2:idx] == b"\xff\x15":
                                refs.append(image_base + text_sec.VirtualAddress + idx - 2)
                            idx += 4
                        results[f"{dll}!{fn}"] = (imp.address, refs)
    return results

print("=== 1. SUBSYSTEM API CALL LOCATIONS ===")
d3d_calls = find_iat_calls("d3d9", ["*"])
for fn, (iat, refs) in d3d_calls.items():
    print(f"{fn} (IAT: 0x{iat:08X}): calls at {[hex(r) for r in refs]}")

bink_calls = find_iat_calls("bink", ["*"])
for fn, (iat, refs) in bink_calls.items():
    print(f"{fn} (IAT: 0x{iat:08X}): calls at {[hex(r) for r in refs]}")

fmod_calls = find_iat_calls("fmod", ["*"])
for fn, (iat, refs) in fmod_calls.items():
    if refs:
        print(f"{fn} (IAT: 0x{iat:08X}): calls at {[hex(r) for r in refs]}")

thread_calls = find_iat_calls("kernel32", ["CreateThread"])
for fn, (iat, refs) in thread_calls.items():
    print(f"{fn} (IAT: 0x{iat:08X}): calls at {[hex(r) for r in refs]}")

print("\n=== 2. GameStates VTABLES AND METHODS ===")
# Locate Complete Object Locators and vtables for GameStates classes
rdata_sec = [s for s in pe.sections if s.Name.decode('utf-8', errors='ignore').strip('\x00') == '.rdata'][0]
rdata = rdata_sec.get_data()

state_classes = [
    ".?AVcGameState@GameStates@@",
    ".?AVcBootupState@GameStates@@",
    ".?AVcIntroState@GameStates@@",
    ".?AVcFrontendLoadState@GameStates@@",
    ".?AVcFrontendRunState@GameStates@@",
    ".?AVcInGameLoadState@GameStates@@",
    ".?AVcInGameRunState@GameStates@@",
    ".?AVcInGameMovieState@GameStates@@",
    ".?AVcLoadDemoState@GameStates@@"
]

for cls in state_classes:
    idx = rdata.find(cls.encode('utf-8'))
    if idx != -1:
        td_va = image_base + rdata_sec.VirtualAddress + idx - 8 # TypeDescriptor starts 8 bytes before name
        # Search for references to TypeDescriptor in rdata (COL references TD by RVA)
        td_rva = td_va - image_base
        pat = struct.pack("<I", td_rva)
        col_idx = 0
        while True:
            col_idx = rdata.find(pat, col_idx)
            if col_idx == -1:
                break
            # In COL, type_desc_rva is at offset +12
            if col_idx >= 12:
                col_va = image_base + rdata_sec.VirtualAddress + col_idx - 12
                # Search for reference to col_va in rdata (vtable[-1] points to COL)
                vt_pat = struct.pack("<I", col_va)
                vt_idx = rdata.find(vt_pat)
                if vt_idx != -1:
                    vtable_va = image_base + rdata_sec.VirtualAddress + vt_idx + 4
                    print(f"Class: {cls}")
                    print(f"  TypeDescriptor VA: 0x{td_va:08X}, COL VA: 0x{col_va:08X}, VTABLE VA: 0x{vtable_va:08X}")
                    # Read first 8 virtual methods
                    vt_rva = vtable_va - image_base
                    vt_methods = struct.unpack("<8I", pe.get_data(vt_rva, 32))
                    for m_idx, m_va in enumerate(vt_methods):
                        print(f"    vtable[{m_idx}] = 0x{m_va:08X}")
            col_idx += 4

