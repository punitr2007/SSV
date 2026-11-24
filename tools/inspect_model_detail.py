import struct
import os

model_path = r'D:\Codes\Win_codes\SSV\assets\extracted\Musclecar_01\Musclecar_01_High.model'
with open(model_path, 'rb') as f:
    data = f.read()

total_sz = len(data)
print(f'Total size: {total_sz}')

# Inspect first 64 uint32s
u32s = struct.unpack('<64I', data[:256])
for i in range(0, 32, 4):
    items = [f'0x{u32s[j]:08X} ({u32s[j]})' for j in range(i, i+4)]
    print(f'[{i:2d}..{i+3:2d}]: {items}')

vbo_end = 111484 + 28*14574
print(f'\nvbo_end = {vbo_end} (0x{vbo_end:08X})')
print(f'Bytes remaining after vbo_end: {total_sz - vbo_end}')
u32_after_vbo = struct.unpack('<16I', data[vbo_end:vbo_end+64])
print('uint32s after vbo:', [f'0x{x:08X} ({x})' for x in u32_after_vbo])

# Search for strings or markers in the second half
import re
print('\nStrings after vbo_end:')
for m in re.finditer(rb'[a-zA-Z0-9_\-\.\/]{4,}', data[vbo_end:]):
    print(f'  at 0x{vbo_end + m.start():08X}: {m.group(0).decode("latin-1")}')
    if m.start() > 10000:
        break
