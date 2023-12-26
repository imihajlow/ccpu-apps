#!/usr/bin/env python3
import subprocess
import sys
import random

def fp_mul(a, b):
    a = [(a >> x) & 0xf for x in range(0,16,4)]
    b = [(b >> x) & 0xf for x in range(0,16,4)]
    r = ((a[3]*b[3]) << 12) + \
        ((a[3]*b[2] + a[2]*b[3]) << 8) + \
        ((a[3]*b[1] + a[2]*b[2] + a[1]*b[3]) << 4) + \
        (a[3]*b[0] + a[2]*b[1] + a[1]*b[2] + a[0]*b[3]) + \
        ((a[2]*b[0] + a[1]*b[1] + a[0]*b[2]) >> 4)
    return r & 0xffff

def test_mul(binary, a, b):
    r = fp_mul(a,b)
    commands = ['u main', f'poke w fooa {a}', f'poke w foob {b}', 'u main_exit', f'check w foor {r}', 'quit']
    args = ['rsim']
    for cmd in commands:
        args += ['-c', cmd]
    args += [binary]
    print(f"Testing {a:04X}x{b:04X}...", end='')
    ret = subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if ret.returncode != 0:
        print("FAIL")
        print(ret.stdout.decode())
        sys.exit(1)
    else:
        print("OK")


binary = sys.argv[1]

# bits
for ba in range(15):
    a = 1 << ba
    for bb in range(15):
        b = 1 << bb
        test_mul(binary, a, b)

# random values
for _ in range(1000):
    a = random.randrange(0, 0x8000)
    b = random.randrange(0, 0x8000)
    test_mul(binary, a, b)
