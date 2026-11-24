import re

def inspect_file(path):
    print("==================================================")
    print("File:", path)
    with open(path, "rb") as f:
        data = f.read()
    print("Total length:", len(data))
    words = re.findall(b"[a-zA-Z0-9_<>/=\":\\.\\-\\?]{3,}", data)
    print(f"Found {len(words)} ASCII strings >= 3 chars:")
    for w in words[:50]:
        print(" ", w.decode('latin1'))

inspect_file(r"D:\Codes\Win_codes\SSV\assets\extracted\UI_Logic_TrackFiltration.xml")
inspect_file(r"D:\Codes\Win_codes\SSV\assets\extracted\UI_Logic_Season.xml")
