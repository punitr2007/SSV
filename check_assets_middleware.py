import pefile
import re
import os

pe_path = r"D:\Codes\Win_codes\Split.Second.Velocity\SplitSecond.exe"
pe = pefile.PE(pe_path)
image_base = pe.OPTIONAL_HEADER.ImageBase
rdata = [s for s in pe.sections if s.Name.decode('utf-8', errors='ignore').strip('\x00') == '.rdata'][0].get_data()
data = [s for s in pe.sections if s.Name.decode('utf-8', errors='ignore').strip('\x00') == '.data'][0].get_data()

print("=== 1. CHECKING ASSET DIRECTORIES IN data/ ===")
base_dir = r"D:\Codes\Win_codes\Split.Second.Velocity\data"
for root, dirs, files in os.walk(base_dir):
    rel = os.path.relpath(root, base_dir)
    if files:
        sample_files = files[:8]
        exts = set(os.path.splitext(f)[1].lower() for f in files)
        print(f"Directory: {rel}")
        print(f"  Files ({len(files)} total), Extensions: {exts}")
        print(f"  Samples: {sample_files}")

print("\n=== 2. HAVOK VERSION & STRINGS ===")
for m in re.finditer(rb"(Havok[^\x00\r\n]{4,80}|hkBase[^\x00\r\n]{4,80}|version[^\x00\r\n]{0,20}Havok[^\x00\r\n]{0,40})", rdata, re.IGNORECASE):
    print(m.group(0).decode('utf-8', errors='ignore'))

print("\n=== 3. FMOD VERSION & STRINGS ===")
for m in re.finditer(rb"(FMOD[^\x00\r\n]{4,60}|EventSystem[^\x00\r\n]{4,60})", rdata):
    s = m.group(0).decode('utf-8', errors='ignore')
    if any(k in s.lower() for k in ["version", "event", "sound", "fmod"]):
        print(s)

print("\n=== 4. UI ENGINE (Scaleform / Flash / Custom) ===")
ui_matches = []
for m in re.finditer(rb"(Scaleform|GFx|Flash|swf|ActionScript|cUI[a-zA-Z0-9_]+)", rdata + data, re.IGNORECASE):
    ui_matches.append(m.group(0).decode('utf-8', errors='ignore'))

print(f"UI engine matches: {len(ui_matches)}")
for u in set(ui_matches[:30]):
    print(f"  {u}")

