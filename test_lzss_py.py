def lzss_compress(data):
    # Okumura LZSS compression
    EI = 12
    EJ = 4
    N = 1 << EI # 4096
    F = (1 << EJ) + 1 # 18
    THRESHOLD = 2

    ring_buffer = bytearray(b' ' * (N - F))
    ring_pos = N - F

    out = bytearray()
    in_idx = 0
    look_ahead = bytearray()

    while len(look_ahead) < F and in_idx < len(data):
        look_ahead.append(data[in_idx])
        in_idx += 1

    flags = 0
    mask = 1
    chunk = bytearray()

    while look_ahead:
        match_len = 0
        match_off = 0

        # Search in ring_buffer
        for i in range(len(ring_buffer)):
            k = 0
            while k < len(look_ahead) and ring_buffer[(i + k) % N] == look_ahead[k]:
                k += 1
            if k > match_len:
                match_len = k
                match_off = i

        if match_len <= THRESHOLD:
            match_len = 1
            flags |= mask
            chunk.append(look_ahead[0])
        else:
            chunk.append(match_off & 0xFF)
            chunk.append(((match_off >> 4) & 0xF0) | ((match_len - (THRESHOLD + 1)) & 0x0F))

        mask <<= 1
        if mask == 256:
            out.append(flags)
            out.extend(chunk)
            flags = 0
            mask = 1
            chunk = bytearray()

        for _ in range(match_len):
            if in_idx < len(data):
                c = data[in_idx]
                in_idx += 1
                ring_buffer.append(c)
                if len(ring_buffer) > N:
                    ring_buffer.pop(0)
            if look_ahead:
                look_ahead.pop(0)
            if in_idx < len(data) and len(look_ahead) < F:
                look_ahead.append(data[in_idx])
                in_idx += 1

    if mask != 1:
        out.append(flags)
        out.extend(chunk)

    return bytes(out)

def lzss_decompress(comp, uncomp_len):
    EI = 12
    EJ = 4
    N = 1 << EI # 4096
    F = (1 << EJ) + 1 # 18
    THRESHOLD = 2

    ring_buffer = bytearray(b' ' * N)
    ring_pos = N - F

    out = bytearray()
    in_idx = 0
    flags = 0

    while in_idx < len(comp) and len(out) < uncomp_len:
        flags >>= 1
        if (flags & 0x100) == 0:
            if in_idx >= len(comp):
                break
            flags = comp[in_idx] | 0xFF00
            in_idx += 1

        if (flags & 1) != 0:
            if in_idx >= len(comp):
                break
            c = comp[in_idx]
            in_idx += 1
            out.append(c)
            ring_buffer[ring_pos] = c
            ring_pos = (ring_pos + 1) % N
        else:
            if in_idx + 1 >= len(comp):
                break
            b1 = comp[in_idx]
            b2 = comp[in_idx + 1]
            in_idx += 2
            match_off = b1 | ((b2 & 0xF0) << 4)
            match_len = (b2 & 0x0F) + THRESHOLD + 1

            for k in range(match_len):
                if len(out) >= uncomp_len:
                    break
                c = ring_buffer[(match_off + k) % N]
                out.append(c)
                ring_buffer[ring_pos] = c
                ring_pos = (ring_pos + 1) % N

    return bytes(out)

msg = b"Split Second Velocity Powerplays! Split Second Velocity Powerplays! 1234567890 1234567890"
c = lzss_compress(msg)
print(f"Original: {len(msg)}, Compressed: {len(c)}")
d = lzss_decompress(c, len(msg))
print(f"Roundtrip match: {d == msg}")
if d != msg:
    print(f"Diff: d={d}, msg={msg}")
