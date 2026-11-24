import pefile
import os

files = [
    r"D:\Codes\Win_codes\Split.Second.Velocity\SplitSecond.exe",
    r"D:\Codes\Win_codes\Split.Second.Velocity\SplitSecond.exe.rld",
    r"D:\Codes\Win_codes\Split.Second.Velocity\SplitSecond-patched.exe"
]

for fpath in files:
    if not os.path.exists(fpath):
        continue
    print(f"\n==========================================")
    print(f"ANALYZING: {os.path.basename(fpath)}")
    print(f"File size: {os.path.getsize(fpath)} bytes")
    try:
        pe = pefile.PE(fpath)
        print(f"Machine: 0x{pe.FILE_HEADER.Machine:04X}")
        print(f"TimeDateStamp: 0x{pe.FILE_HEADER.TimeDateStamp:08X}")
        print(f"ImageBase: 0x{pe.OPTIONAL_HEADER.ImageBase:08X}")
        print(f"EntryPoint: 0x{pe.OPTIONAL_HEADER.AddressOfEntryPoint:08X} (VA: 0x{pe.OPTIONAL_HEADER.ImageBase + pe.OPTIONAL_HEADER.AddressOfEntryPoint:08X})")
        print(f"Subsystem: {pe.OPTIONAL_HEADER.Subsystem}")
        print(f"Linker: {pe.OPTIONAL_HEADER.MajorLinkerVersion}.{pe.OPTIONAL_HEADER.MinorLinkerVersion}")
        print(f"Sections ({len(pe.sections)}):")
        for s in pe.sections:
            name = s.Name.decode('utf-8', errors='ignore').strip('\x00')
            print(f"  {name:<10} VA: 0x{s.VirtualAddress:08X} VSize: 0x{s.Misc_VirtualSize:08X} RawOffset: 0x{s.PointerToRawData:08X} RawSize: 0x{s.SizeOfRawData:08X} Chars: 0x{s.Characteristics:08X}")
    except Exception as e:
        print(f"Error parsing {fpath}: {e}")
