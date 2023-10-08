#!/usr/bin/env python3
from PIL import Image
import argparse
import sys
from pathlib import Path

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Snake map generator')
    parser.add_argument('file', help='input file')
    args = parser.parse_args()

    image = Image.open(args.file)
    width, height = image.size
    if width != 80 or height != 60:
        print("Must be a 80x60 image")
        sys.exit(1)
    data = []
    for (r,g,b) in image.getdata():
        if r + g + b == 0:
            data += [0]
        else:
            data += [1]

    path = Path(args.file)
    out_filename = path.with_suffix('.txt')

    with open(out_filename, "w") as f:
        print("{", file=f)
        for r in range(0, 60):
            print("    ", file=f, end='')
            for c in range(0, 80, 8):
                print("0b", file=f, end='')
                for bit in range(0, 8):
                    x = data[r * 80 + c + bit]
                    print(x, file=f, end='')
                print(", ", file=f, end='')
            print("", file=f)
        print("};", file=f)

