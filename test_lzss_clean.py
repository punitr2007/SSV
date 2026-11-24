def test_lzss():
    N = 4096
    F = 18
    THRESHOLD = 2

    def decompress(comp, out_len):
        buf = bytearray(N)
        r = N - F
        out = bytearray()
        pos = 0
        flags = 0
        while pos < len(comp) and len(out) < out_len:
            flags >>= 1
            if (flags & 0x100) == 0:
                if pos >= len(comp): break
                flags = comp[pos] | 0xFF00
                pos += 1
            if flags & 1:
                if pos >= len(comp): break
                c = comp[pos]
                pos += 1
                out.append(c)
                buf[r] = c
                r = (r + 1) & (N - 1)
            else:
                if pos + 1 >= len(comp): break
                b1 = comp[pos]
                b2 = comp[pos + 1]
                pos += 2
                i = b1 | ((b2 & 0xF0) << 4)
                j = (b2 & 0x0F) + THRESHOLD + 1
                for k in range(j):
                    if len(out) >= out_len: break
                    c = buf[(i + k) & (N - 1)]
                    out.append(c)
                    buf[r] = c
                    r = (r + 1) & (N - 1)
        return bytes(out)

    def compress(src):
        buf = bytearray(N)
        r = N - F
        out = bytearray()
        src_pos = 0
        
        flags = 0
        flag_mask = 1
        chunk = bytearray()

        while src_pos < len(src):
            # find longest match in buf[0..N-1]
            max_len = 0
            max_pos = 0
            avail = min(F, len(src) - src_pos)

            for i in range(N):
                l = 0
                while l < avail and buf[(i + l) & (N - 1)] == src[src_pos + l]:
                    l += 1
                if l > max_len:
                    max_len = l
                    max_pos = i

            if max_len <= THRESHOLD:
                # literal
                c = src[src_pos]
                src_pos += 1
                flags |= flag_mask
                chunk.append(c)
                buf[r] = c
                r = (r + 1) & (N - 1)
            else:
                # match
                chunk.append(max_pos & 0xFF)
                chunk.append(((max_pos >> 4) & 0xF0) | ((max_len - (THRESHOLD + 1)) & 0x0F))
                for _ in range(max_len):
                    c = src[src_pos]
                    src_pos += 1
                    buf[r] = c
                    r = (r + 1) & (N - 1)

            flag_mask <<= 1
            if flag_mask == 256:
                out.append(flags)
                out.extend(chunk)
                flags = 0
                flag_mask = 1
                chunk = bytearray()

        if flag_mask != 1:
            out.append(flags)
            out.extend(chunk)

        return bytes(out)

    msg = b"Split Second Velocity Powerplays! Split Second Velocity Powerplays! 1234567890 1234567890"
    comp = compress(msg)
    print(f"Original len: {len(msg)}, compressed len: {len(comp)}")
    decomp = decompress(comp, len(msg))
    print(f"Decompress matches: {decomp == msg}")
    assert decomp == msg

test_lzss()
