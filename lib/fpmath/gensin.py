#!/usr/bin/env python3

import math
for i in range(128):
    angle = i * math.pi / 128.0
    v = math.sin(angle)
    x = round(v * 2**12)
    print(f"0x{x:04X},", end="")
    if i % 16 == 15:
        print()
