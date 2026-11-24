import re

exe_path = r"D:\Codes\Win_codes\Split.Second.Velocity\SplitSecond.exe"

with open(exe_path, "rb") as f:
    data = f.read()

# Search for strings containing xml, track, route, circuit, or level
strings = re.findall(b"[a-zA-Z0-9_\\\\/\\-\\.]+\\.xml", data)
print(f"Found {len(strings)} XML string references in SplitSecond.exe:")
unique_xmls = sorted(set(s.decode('latin1', errors='ignore') for s in strings))
for s in unique_xmls:
    if any(k in s.lower() for k in ["track", "route", "circuit", "race", "event", "env", "level", "game", "database", "season", "hud", "cam"]):
        print("  XML:", s)

# Search for environment names
env_matches = re.findall(b"environments/[a-zA-Z0-9_\\\\/\\-\\.]+", data)
unique_envs = sorted(set(s.decode('latin1', errors='ignore') for s in env_matches))
print(f"\nFound {len(unique_envs)} environments/ paths in SplitSecond.exe:")
for s in unique_envs[:25]:
    print("  Env:", s)

# Search for sector / track model formats
model_matches = re.findall(b"[a-zA-Z0-9_\\\\/\\-\\.]+\\.(?:model|stream|mesh|scene|spline|road)", data)
unique_models = sorted(set(s.decode('latin1', errors='ignore') for s in model_matches))
print(f"\nFound {len(unique_models)} model/stream/road extensions in SplitSecond.exe:")
for s in unique_models[:25]:
    print("  Format:", s)
