import subprocess
import sys
import os

clang = r"C:\Program Files\Windhawk\Compiler\bin\clang++.exe"
ssv_dir = r"D:\Codes\Win_codes\SSV"

includes = [
    "-I", os.path.join(ssv_dir, "include"),
    "-I", os.path.join(ssv_dir, "src", "core"),
    "-I", os.path.join(ssv_dir, "src", "assets"),
    "-I", os.path.join(ssv_dir, "src", "render")
]

sources = [
    os.path.join(ssv_dir, "desktop", "desktop_main.cpp"),
    os.path.join(ssv_dir, "src", "render", "vehicle_renderer.cpp"),
    os.path.join(ssv_dir, "src", "render", "gl_platform.cpp"),
    os.path.join(ssv_dir, "src", "core", "camera_rig.cpp"),
    os.path.join(ssv_dir, "src", "core", "vehicle_physics.cpp"),
    os.path.join(ssv_dir, "src", "core", "game_engine.cpp"),
    os.path.join(ssv_dir, "src", "core", "state_machine.cpp"),
    os.path.join(ssv_dir, "src", "assets", "lzss_decomp.cpp"),
    os.path.join(ssv_dir, "src", "assets", "ark_parser.cpp"),
    os.path.join(ssv_dir, "src", "platform", "platform_null.cpp"),
]

libs = ["-lopengl32", "-luser32", "-lgdi32"]

out_exe = os.path.join(ssv_dir, "ssv_desktop.exe")

cmd = [clang, "-std=c++17", "-O2", "-static"] + includes + sources + libs + ["-o", out_exe]
print(f"Compiling desktop harness with Clang...")
res = subprocess.run(cmd, capture_output=True, text=True)
if res.returncode != 0:
    print(f"Compilation FAILED with code {res.returncode}:")
    print(res.stderr)
    sys.exit(1)

print(f"Compilation SUCCESSFUL! Output: {out_exe}")
