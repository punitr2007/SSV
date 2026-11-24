# Split/Second: Velocity — Asset Architecture & Formats

This directory contains specifications, tools, and parsers for the proprietary game asset formats used by *Split/Second: Velocity*.

---

## 1. Container Format: `.ARK`

Black Rock Studio packages all non-streamed and streamed assets into `.ARK` container files.

### Binary Header (16 bytes = 0x10)
| Offset | Type | Description | Observed Value in Game |
| :--- | :--- | :--- | :--- |
| `0x00` | `uint32` | Container Version / Magic | `0x00010000` (Version 1.0) |
| `0x04` | `uint32` | Table of Contents Size in bytes | Size of all entries combined |
| `0x08` | `uint32` | Number of Entries (Files) | e.g., 6,851 in `resident.ark` |
| `0x0C` | `uint32` | Table of Contents Offset | `0x00000010` (Always 16 bytes) |

### Table of Contents Record (20 bytes = 0x14)
| Offset | Type | Description | Notes |
| :--- | :--- | :--- | :--- |
| `0x00` | `uint32` | File Offset | Byte offset within the `.ark` file |
| `0x04` | `uint32` | Stored / Compressed Size | Size of raw payload stream |
| `0x08` | `uint32` | Uncompressed Size | Target decompressed buffer size |
| `0x0C` | `uint32` | Flags | Bit 0: `1` = LZSS Compressed, `0` = Raw |
| `0x10` | `uint32` | Path Name Hash | 32-bit hash of normalized asset path |

### Compression: LZSS
- **Window Size:** 4,096 bytes (12 bits).
- **Match Length:** 3 to 18 bytes (4 bits + 2 threshold).
- **Control Flags:** 8 bits per chunk (1 = literal byte, 0 = match offset & length).

---

## 2. Asset Types & Middleware Mapping

| Category | File Extension | Middleware / Engine | Storage Location |
| :--- | :--- | :--- | :--- |
| **Track / Levels** | `.ark`, `.static.ark`, `.stream.ark` | In-house Proprietary | `data\environments\levels\` |
| **Powerplay Events**| `*powerplays.ark` | In-house Proprietary | `data\` |
| **Audio Events** | `.fev` | FMOD Event Designer 4.x | `data\audio\release\` |
| **Audio SoundBanks**| `.fsb` | FMOD SoundBank (Vorbis/PCM) | `data\audio\release\` |
| **Cinematics / FMV**| `.bik` | RAD Game Tools Bink Video | `data\movies\` |
| **UI & Menus** | `.swf`, `.gfx` | Scaleform GFx | `data\ui\` |
| **Shaders** | `.bin` | Compiled DirectX 9 HLSL | `data\shaders\`, `data\global_shader_combos\` |

---

## 3. CLI Tool Usage (`ark_tool.py`)

A standalone tool is provided to inspect archive contents without compiling the C++ engine:

```bash
# View summary and first 50 entries
python assets/ark_tool.py "D:\Codes\Win_codes\Split.Second.Velocity\data\frontendpowerplays.ark" --list
```
