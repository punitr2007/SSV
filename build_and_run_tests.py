import subprocess
import sys
import os

clang = r"C:\Program Files\Windhawk\Compiler\bin\clang++.exe"
ssv_dir = r"D:\Codes\Win_codes\SSV"

includes = [
    "-I", os.path.join(ssv_dir, "include"),
    "-I", os.path.join(ssv_dir, "src", "core"),
    "-I", os.path.join(ssv_dir, "src", "assets")
]

sources = [
    os.path.join(ssv_dir, "src", "platform", "platform_null.cpp"),
    os.path.join(ssv_dir, "src", "assets", "lzss_decomp.cpp"),
    os.path.join(ssv_dir, "src", "assets", "ark_parser.cpp"),
    os.path.join(ssv_dir, "src", "core", "state_machine.cpp"),
    os.path.join(ssv_dir, "src", "core", "game_engine.cpp"),
    os.path.join(ssv_dir, "src", "core", "vehicle_physics.cpp"),
    os.path.join(ssv_dir, "src", "core", "camera_rig.cpp"),
    os.path.join(ssv_dir, "tests", "test_math.cpp"),
    os.path.join(ssv_dir, "tests", "test_state_machine.cpp"),
    os.path.join(ssv_dir, "tests", "test_ark_parser.cpp"),
    os.path.join(ssv_dir, "tests", "test_vehicle_physics.cpp"),
    os.path.join(ssv_dir, "tests", "test_runner.cpp")
]

out_exe = os.path.join(ssv_dir, "test_runner.exe")

cmd = [clang, "-std=c++17", "-O2", "-static"] + includes + sources + ["-o", out_exe]
print(f"Compiling with Clang: {' '.join(cmd)}")

res = subprocess.run(cmd, capture_output=True, text=True)
if res.returncode != 0:
    print(f"Compilation FAILED with code {res.returncode}:")
    print(res.stderr)
    sys.exit(1)

print("Compilation SUCCESSFUL! Running test_runner.exe...\n")
run_res = subprocess.run([out_exe], capture_output=True, text=True)
print(run_res.stdout)
if run_res.stderr:
    print("Stderr:", run_res.stderr)

sys.exit(run_res.returncode)
