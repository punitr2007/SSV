# Split/Second: Velocity — Core Logic Reconstruction

Clean, modular, cross-platform C/C++ reconstruction of the core game logic, state machine, physics/powerplay systems, and asset container pipelines for *Split/Second: Velocity* (Black Rock Studio / Disney Interactive Studios).

Based on byte-level reverse engineering and memory layout verification of `SplitSecond.exe` (SHA-256: `d11fc7f48dd504bdefb16b548980bd41101f6c1a23d29c03487a81d4de42173e`).

---

## Architecture Overview

```
D:\Codes\Win_codes\SSV\
├── include/
│   └── platform_abstraction.h      # Hardware/OS Abstraction Layer (HAL)
├── src/
│   ├── platform/
│   │   ├── platform_null.cpp       # Headless / deterministic platform implementation
│   │   └── platform_win32.cpp      # Native Win32 / Direct3D / XInput platform implementation
│   ├── core/
│   │   ├── math_types.h            # 3D Math: Vectors, Matrices (DirectX LH), Quaternions, AABBs
│   │   ├── powerplay_system.h      # Split/Second Powerplay trigger and detonation mechanics
│   │   ├── state_machine.h         # GameStates namespace (10 verified concrete state classes)
│   │   ├── state_machine.cpp       # State transitions and virtual lifecycle methods
│   │   ├── game_engine.h           # Decoupled multithreaded simulation engine singleton
│   │   └── game_engine.cpp         # Fixed-timestep simulation tick (60 Hz) and render loop
│   └── assets/
│       ├── ark_parser.h            # .ARK archive container format parser
│       ├── ark_parser.cpp          # Table of Contents reader and stream loader
│       ├── lzss_decomp.h           # Sliding-window LZSS compression / decompression
│       └── lzss_decomp.cpp         # Bitstream LZSS implementation matching engine vtables
├── assets/
│   ├── ark_tool.py                 # Standalone Python CLI to inspect and unpack .ARK archives
│   └── README.md                   # Asset format specifications (.ark, .fsb, .fev, .bik)
├── tests/
│   ├── test_runner.cpp             # Master test suite entry point
│   ├── test_math.cpp               # Unit tests: 3D vector, matrix, quaternion, ray-box tests
│   ├── test_state_machine.cpp      # Unit tests: State transitions, race progression, powerplays
│   └── test_ark_parser.cpp         # Unit tests: LZSS round-trip and real game archive parsing
├── architecture_map.md             # Forensic reverse-engineering breakdown
├── architectural_evidence.json     # Complete structured binary addresses, vtables, and hashes
└── CMakeLists.txt                  # Modern cross-platform CMake build configuration
```

---

## 1. Hardware Abstraction Layer (`platform_abstraction.h`)

All operating system and hardware-specific dependencies are isolated into clean interface contracts:
- **`IWindow`**: Win32 window creation, DPI scaling, and decoupled event polling (`PeekMessageA`).
- **`IGraphicsDevice`**: Rendering device interface abstracting Direct3D 9, ready for Vulkan, Metal, or OpenGL.
- **`IAudioSubsystem`**: Audio playback interface abstracting FMOD Ex / FMOD Event System.
- **`IInputSystem`**: Polling keyboard and gamepads (XInput triggers/vibration, DirectInput wheels).
- **`IFileSystem`**: Safe relative file I/O and user save path resolution.
- **`ITimeProvider`**: High-resolution microsecond clock (`timeGetTime` / `std::chrono`).

---

## 2. Asset Container Subsystem (`.ARK`)

The proprietary Black Rock Studio `.ARK` container format is fully supported:
- **Header (16 bytes / `0x10`):** `version` (`0x00010000`), `toc_size`, `entry_count`, `toc_offset`.
- **TOC Records (20 bytes / `0x14`):** `offset`, `compressed_size`, `uncompressed_size`, `flags`, `name_hash`.
- **Decompression:** Hardware-accurate LZSS sliding-window decompressor (`lzss_decomp.cpp`).
- **Tested directly against:** `frontendpowerplays.ark` (21 entries) and `resident.ark` (6,851 entries).

### Using the Python ARK Tool
```bash
# List contents of an archive
python assets/ark_tool.py "D:\Codes\Win_codes\Split.Second.Velocity\data\frontendpowerplays.ark" --list

# Inspect main resident container
python assets/ark_tool.py "D:\Codes\Win_codes\Split.Second.Velocity\data\resident.ark"
```

---

## 3. Game State Machine (`GameStates` Namespace)

Directly mirrors the class hierarchy, virtual methods, and transitions identified from binary vtables:
1. **`cBootupState`** (`0x00BA2AA4`): Mounts global archives and boots Direct3D/FMOD.
2. **`cIntroState`** (`0x00BA32DC`): Plays studio video logos via Bink.
3. **`cIntroSequence`** (`0x00BA37EC`): Cinematic game opening.
4. **`cFrontendLoadState`** (`0x00BA3398`): Loads Scaleform Flash menu assets.
5. **`cFrontendRunState`** (`0x00BA38C0`): Menu navigation, garage, and multiplayer lobby.
6. **`cInGameLoadState`** (`0x00BA3110`): Prepares track geometry, physics, and vehicles.
7. **`cInGameMovieState`** (`0x00BA3518`): Track fly-through and pre-race sequences.
8. **`cInGameRunState`** (`0x00BA36A4`): Main race simulation, AI, and Powerplay detonations.
9. **`cLoadDemoState`** (`0x00BA3894`): Attract mode demo loop.

---

## 4. Building and Running the Unit Test Suite

The test suite validates linear algebra, matrix projection, ray-AABB collision, state machine transitions, powerplay trigger charging, LZSS round-trip compression, and real game archive mounting.

### Building with CMake
```bash
mkdir build && cd build
cmake ..
cmake --build .
ctest --verbose
```

### Direct Compilation with Clang / MSVC
```bash
clang++ -static -std=c++17 -O2 -I include -I src/core -I src/assets \
    src/platform/platform_null.cpp \
    src/assets/lzss_decomp.cpp \
    src/assets/ark_parser.cpp \
    src/core/state_machine.cpp \
    src/core/game_engine.cpp \
    tests/test_runner.cpp \
    tests/test_math.cpp \
    tests/test_state_machine.cpp \
    tests/test_ark_parser.cpp \
    -o test_runner.exe

./test_runner.exe
```

### Verification Result
```
============================================================
   SPLIT/SECOND: VELOCITY RECONSTRUCTION TEST SUITE         
============================================================
[TEST] Running Math: Vector Operations...
  -> Vector operations passed.
[TEST] Running Math: Matrix Transformations...
  -> Matrix transformations passed.
[TEST] Running Math: Ray & AABB Intersections...
  -> Ray-AABB intersection passed.
[TEST] Running Math: Quaternion Rotations...
  -> Quaternion rotations passed.
[TEST] Running State Machine Transitions...
  -> All 8 state transitions verified successfully.
[TEST] Running Powerplay Trigger Mechanics...
  -> Powerplay trigger and detonation mechanics passed.
[TEST] Running LZSS Compression / Decompression Round-Trip...
  -> LZSS round-trip verified (190 bytes -> 114 bytes -> 190 bytes).
[TEST] Running Real ARK Archive Parsing Against Game Data...
  -> Real frontendpowerplays.ark parsed and verified.

============================================================
   ALL UNIT TESTS PASSED SUCCESSFULLY! (100% VERIFIED)      
============================================================
```
