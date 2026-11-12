import struct, zlib

def chunk(t, d):
    c = t + d
    return struct.pack(">I", len(d)) + c + struct.pack(">I", zlib.crc32(c) & 0xffffffff)

# 2x2 红色 PNG
raw = b"\x00\xff\x00\x00\x00\xff\x00\x00"
ihdr = struct.pack(">IIBBBBB", 2, 2, 8, 2, 0, 0, 0)
png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", ihdr) + chunk(b"IDAT", zlib.compress(raw)) + chunk(b"IEND", b"")
with open("/tmp/test.png", "wb") as f:
    f.write(png)
print("png created", len(png))
