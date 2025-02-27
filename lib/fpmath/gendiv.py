#!/usr/bin/env python3

for x in range(128):
    val = float((x << 8) / 2**12)
    if val == 0:
        val = 1 / 2**12
    rc = 1.0 / val
    fp = min(round(rc * 2**12), 0x7fff)
    print(f"0x{fp:04X},", end="")
    if x % 16 == 15:
        print()
