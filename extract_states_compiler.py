import pefile
import re
import os

pe_path = r"D:\Codes\Win_codes\Split.Second.Velocity\SplitSecond.exe"
pe = pefile.PE(pe_path)
image_base = pe.OPTIONAL_HEADER.ImageBase

# Read all sections data
rdata_sec = [s for s in pe.sections if s.Name.decode('utf-8', errors='ignore').strip('\x00') == '.rdata'][0]
data_sec = [s for s in pe.sections if s.Name.decode('utf-8', errors='ignore').strip('\x00') == '.data'][0]
text_sec = [s for s in pe.sections if s.Name.decode('utf-8', errors='ignore').strip('\x00') == '.text'][0]

rdata = rdata_sec.get_data()
data = data_sec.get_data()
text = text_sec.get_data()

print("=== 1. ALL CLASSES IN GameStates NAMESPACE ===")
gamestates = []
pattern = re.compile(rb"\.\?AV([a-zA-Z0-9_]+)@GameStates@@")
for m in pattern.finditer(rdata):
    va = image_base + rdata_sec.VirtualAddress + m.start()
    cls_name = m.group(0).decode('utf-8', errors='ignore')
    gamestates.append((va, cls_name))
for m in pattern.finditer(data):
    va = image_base + data_sec.VirtualAddress + m.start()
    cls_name = m.group(0).decode('utf-8', errors='ignore')
    gamestates.append((va, cls_name))

print(f"Total GameStates classes: {len(gamestates)}")
for va, name in sorted(gamestates, key=lambda x: x[0]):
    print(f"  0x{va:08X}: {name}")

print("\n=== 2. COMPILER & CRT SIGNATURES ===")
# Search for MSVC compiler strings, crt error messages, etc.
crt_patterns = [
    rb"Microsoft Visual C\+\+ Runtime Library",
    rb"Visual C\+\+ CRT: Not enough memory",
    rb"mscoree\.dll",
    rb"Runtime Error!",
    rb"MSVCR",
    rb"MSVCP",
    rb"d:\\.+?\.(cpp|c|h)",
    rb"c:\\.+?\.(cpp|c|h)",
    rb"\.\\.+?\.(cpp|c|h)"
]

print("Searching for C/C++ source file references (.cpp)...")
cpp_files = []
for p in [rb"[a-zA-Z0-9_\\]+\.cpp", rb"\w+:\\.+?\.cpp"]:
    pat = re.compile(p, re.IGNORECASE)
    for m in pat.finditer(rdata):
        va = image_base + rdata_sec.VirtualAddress + m.start()
        str_val = m.group(0).decode('utf-8', errors='ignore')
        if len(str_val) > 4:
            cpp_files.append((va, str_val))

with open(r"D:\Codes\Win_codes\SSV\cpp_source_files.txt", "w", encoding="utf-8") as f:
    seen = set()
    for va, fpath in cpp_files:
        if fpath not in seen:
            seen.add(fpath)
            f.write(f"0x{va:08X}: {fpath}\n")

print(f"Total distinct .cpp references found: {len(seen)}")
print("Sample source files:")
for va, fpath in list(seen)[:25]:
    print(f"  {fpath}")

# Check compiler version from string or header
print("\n=== 3. SEARCHING COMPILER VERSION STRINGS ===")
for m in re.finditer(rb"(Visual C\+\+|MSVC|Compiler|15\.00\.|14\.00\.|16\.00\.|8\.00\.|9\.00\.|10\.00\.)", rdata + data):
    print(m.group(0))

