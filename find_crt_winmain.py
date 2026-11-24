import pefile
import capstone
import re
import struct

pe_path = r"D:\Codes\Win_codes\Split.Second.Velocity\SplitSecond.exe"
pe = pefile.PE(pe_path)
image_base = pe.OPTIONAL_HEADER.ImageBase

rdata_sec = [s for s in pe.sections if s.Name.decode('utf-8', errors='ignore').strip('\x00') == '.rdata'][0]
data_sec = [s for s in pe.sections if s.Name.decode('utf-8', errors='ignore').strip('\x00') == '.data'][0]
text_sec = [s for s in pe.sections if s.Name.decode('utf-8', errors='ignore').strip('\x00') == '.text'][0]

rdata = rdata_sec.get_data()
data = data_sec.get_data()
text = text_sec.get_data()

# 1. Print all cpp files
print("=== 1. SOURCE TREE & CODEBASE PATHS ===")
cpp_pat = re.compile(rb"([a-zA-Z0-9_\\]+\.cpp)", re.IGNORECASE)
found_cpp = set()
for m in cpp_pat.finditer(rdata):
    s = m.group(1).decode('utf-8', errors='ignore')
    found_cpp.add(s)

print(f"Total cpp files: {len(found_cpp)}")
for s in sorted(found_cpp)[:40]:
    print(f"  {s}")

# 2. Check Compiler and CRT
print("\n=== 2. COMPILER & CRT ANALYSIS ===")
print(f"Linker Version: {pe.OPTIONAL_HEADER.MajorLinkerVersion}.{pe.OPTIONAL_HEADER.MinorLinkerVersion}")

# Check if MSVCR DLL is imported
imported_dlls = [entry.dll.decode('utf-8', errors='ignore') for entry in pe.DIRECTORY_ENTRY_IMPORT]
print(f"Imported CRT DLLs: {[d for d in imported_dlls if 'msvc' in d.lower() or 'ucrt' in d.lower()]}")
if not any('msvc' in d.lower() for d in imported_dlls):
    print("-> CRT is statically linked (libcmt.lib / libcpmt.lib)!")

# Look for MSVC CRT strings
crt_strings = []
for m in re.finditer(rb"(Microsoft Visual C\+\+[^\x00]+|Runtime Error![^\x00]+|vector<T> too long|string too long)", rdata):
    va = image_base + rdata_sec.VirtualAddress + m.start()
    crt_strings.append((va, m.group(0).decode('utf-8', errors='ignore')))

for va, s in crt_strings[:10]:
    print(f"  0x{va:08X}: {s}")

# 3. Locate WinMainCRTStartup and WinMain
print("\n=== 3. ENTRY POINT & WinMain DISCOVERY ===")
# In MSVC x86, WinMainCRTStartup typically initializes security cookie, calls __tmainCRTStartup, which calls WinMain.
# Let's search for calls to GetCommandLineA or GetStartupInfoA in .text to find CRT startup
kernel32_imports = {}
for entry in pe.DIRECTORY_ENTRY_IMPORT:
    if entry.dll.decode('utf-8', errors='ignore').upper() == 'KERNEL32.DLL':
        for imp in entry.imports:
            if imp.name:
                kernel32_imports[imp.name.decode('utf-8', errors='ignore')] = imp.address

print(f"KERNEL32 GetCommandLineA IAT: 0x{kernel32_imports.get('GetCommandLineA', 0):08X}")
print(f"KERNEL32 GetStartupInfoA IAT: 0x{kernel32_imports.get('GetStartupInfoA', 0):08X}")
print(f"KERNEL32 GetModuleHandleA IAT: 0x{kernel32_imports.get('GetModuleHandleA', 0):08X}")

# Search for references to GetCommandLineA in .text (CALL [IAT] or MOV reg, [IAT])
cmdline_iat = kernel32_imports.get('GetCommandLineA', 0)
startup_iat = kernel32_imports.get('GetStartupInfoA', 0)

md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)

def search_iat_refs(iat_addr):
    refs = []
    # bytes for FF 15 <iat_addr_le> (call dword ptr [iat])
    pat = struct.pack("<I", iat_addr)
    idx = 0
    while True:
        idx = text.find(pat, idx)
        if idx == -1:
            break
        # inspect preceding bytes
        if idx >= 2 and text[idx-2:idx] == b"\xff\x15":
            va = image_base + text_sec.VirtualAddress + idx - 2
            refs.append(va)
        idx += 4
    return refs

cmdline_refs = search_iat_refs(cmdline_iat)
print(f"Calls to GetCommandLineA in .text: {[hex(x) for x in cmdline_refs]}")

startup_refs = search_iat_refs(startup_iat)
print(f"Calls to GetStartupInfoA in .text: {[hex(x) for x in startup_refs]}")

