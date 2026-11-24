import os
import struct

def brs_hash(s):
    h = 0
    for c in s:
        h = ((h * 31) + ord(c.lower())) & 0xFFFFFFFF
    return h

targets = [
    "UI/Logic/Season.xml",
    "UI/Logic/TrackFiltration.xml",
    "Cameras/CommonCameras/CameraGroups.xml",
    "Cameras/CommonCameras/CameraSplines.xml",
    "Cameras/CommonCameras/Cameras.xml",
    "Cameras/UtilCameras/Cameras.xml",
    "UI/Frontend/Cameras/CameraSplines.xml",
    "UI/Frontend/Cameras/Cameras.xml",
    "Vehicles/VehicleDatabase.xml"
]

ark_path = r"D:\Codes\Win_codes\Split.Second.Velocity\data\resident.ark"
with open(ark_path, "rb") as f:
    f.seek(16) # Skip header
    # resident.ark has num_files at offset 8
    f.seek(8)
    num_files = struct.unpack("<I", f.read(4))[0]
    f.seek(16)
    
    entries = {}
    for i in range(num_files):
        h, off, uncomp, comp, flags = struct.unpack("<IIIII", f.read(20))
        entries[h] = (off, uncomp, comp, flags)

for t in targets:
    h = brs_hash(t)
    if h in entries:
        e = entries[h]
        print(f"[FOUND!] 0x{h:08X} -> {t} (uncomp={e[1]} B, comp={e[2]} B, offset=0x{e[0]:08X})")
    else:
        print(f"[-] 0x{h:08X} -> {t} NOT FOUND")
