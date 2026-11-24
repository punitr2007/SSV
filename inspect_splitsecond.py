import pefile
import capstone
import os
import sys
import json
import struct
import re

exe_path = r"D:\Codes\Win_codes\Split.Second.Velocity\SplitSecond.exe"
pe = pefile.PE(exe_path)

print("="*60)
print("1. PE HEADER & GENERAL INFO")
print("="*60)
print(f"Machine: 0x{pe.FILE_HEADER.Machine:04X} ({pefile.MACHINE_TYPE.get(pe.FILE_HEADER.Machine, 'Unknown')})")
print(f"Number of Sections: {pe.FILE_HEADER.NumberOfSections}")
print(f"TimeDateStamp: 0x{pe.FILE_HEADER.TimeDateStamp:08X}")
print(f"Characteristics: 0x{pe.FILE_HEADER.Characteristics:04X}")
print(f"ImageBase: 0x{pe.OPTIONAL_HEADER.ImageBase:08X}")
print(f"AddressOfEntryPoint: 0x{pe.OPTIONAL_HEADER.AddressOfEntryPoint:08X} (VA: 0x{pe.OPTIONAL_HEADER.ImageBase + pe.OPTIONAL_HEADER.AddressOfEntryPoint:08X})")
print(f"Subsystem: {pe.OPTIONAL_HEADER.Subsystem} ({pefile.SUBSYSTEM_TYPE.get(pe.OPTIONAL_HEADER.Subsystem, 'Unknown')})")
print(f"MajorLinkerVersion: {pe.OPTIONAL_HEADER.MajorLinkerVersion}.{pe.OPTIONAL_HEADER.MinorLinkerVersion}")
print(f"SizeOfImage: 0x{pe.OPTIONAL_HEADER.SizeOfImage:08X}")
print(f"SizeOfHeaders: 0x{pe.OPTIONAL_HEADER.SizeOfHeaders:08X}")

print("\n" + "="*60)
print("2. SECTIONS")
print("="*60)
for s in pe.sections:
    name = s.Name.decode('utf-8', errors='ignore').strip('\x00')
    print(f"Section: {name:<8} VA: 0x{s.VirtualAddress:08X} VSize: 0x{s.Misc_VirtualSize:08X} RawOffset: 0x{s.PointerToRawData:08X} RawSize: 0x{s.SizeOfRawData:08X} Chars: 0x{s.Characteristics:08X}")

print("\n" + "="*60)
print("3. RICH HEADER (COMPILER TOOLS)")
print("="*60)
rich_header = pe.parse_rich_header()
if rich_header:
    print(f"Rich Header Key: 0x{rich_header.get('key', 0):08X}")
    values = rich_header.get('values', [])
    for i in range(0, len(values), 2):
        if i + 1 < len(values):
            compid = values[i]
            count = values[i+1]
            prod_id = compid >> 16
            build_id = compid & 0xFFFF
            print(f"  Tool ID: {prod_id} (0x{prod_id:04X}), Build: {build_id}, Count: {count}")
else:
    print("No Rich Header found or parsed.")

print("\n" + "="*60)
print("4. DEBUG DIRECTORY & PDB")
print("="*60)
if hasattr(pe, 'DIRECTORY_ENTRY_DEBUG'):
    for dbg in pe.DIRECTORY_ENTRY_DEBUG:
        print(f"Debug Type: {dbg.struct.Type} ({pefile.DEBUG_TYPE.get(dbg.struct.Type, 'Unknown')})")
        print(f"TimeDateStamp: 0x{dbg.struct.TimeDateStamp:08X}")
        print(f"Major/Minor: {dbg.struct.MajorVersion}.{dbg.struct.MinorVersion}")
        if hasattr(dbg, 'entry') and dbg.entry:
            if hasattr(dbg.entry, 'PdbFileName'):
                pdb = dbg.entry.PdbFileName.decode('utf-8', errors='ignore').strip('\x00')
                print(f"PDB Path: {pdb}")
            if hasattr(dbg.entry, 'Signature_Data'):
                print(f"PDB Signature/GUID: {dbg.entry.Signature_Data.hex()}")
            if hasattr(dbg.entry, 'Age'):
                print(f"PDB Age: {dbg.entry.Age}")
else:
    print("No Debug Directory found.")

print("\n" + "="*60)
print("5. IMPORTED DLLs & SUMMARY")
print("="*60)
dll_imports = {}
if hasattr(pe, 'DIRECTORY_ENTRY_IMPORT'):
    for entry in pe.DIRECTORY_ENTRY_IMPORT:
        dll_name = entry.dll.decode('utf-8', errors='ignore')
        funcs = []
        for imp in entry.imports:
            fn_name = imp.name.decode('utf-8', errors='ignore') if imp.name else f"Ordinal({imp.ordinal})"
            funcs.append((imp.address, imp.hint, fn_name))
        dll_imports[dll_name] = funcs
        print(f"DLL: {dll_name} ({len(funcs)} imports)")

with open(r"D:\Codes\Win_codes\SSV\imports_detailed.txt", "w", encoding="utf-8") as f:
    for dll, funcs in dll_imports.items():
        f.write(f"=== {dll} ({len(funcs)} functions) ===\n")
        for addr, hint, fn in funcs:
            f.write(f"  0x{addr:08X} Hint:{hint:<5} {fn}\n")
        f.write("\n")
print(f"Detailed imports written to D:\\Codes\\Win_codes\\SSV\\imports_detailed.txt")

