#!/usr/bin/env python3
import subprocess
import sys
import random

def fp_mul(a, b):
    a = [(a >> x) & 0xf for x in range(0,32,4)]
    b = [(b >> x) & 0xf for x in range(0,32,4)]
    r = ((a[7]*b[7]) << 28) + \
        ((a[6]*b[7] + a[7]*b[6]) << 24) + \
        ((a[5]*b[7] + a[6]*b[6] + a[7]*b[5]) << 20) + \
        ((a[4]*b[7] + a[5]*b[6] + a[6]*b[5] + a[7]*b[4]) << 16) + \
        ((a[3]*b[7] + a[4]*b[6] + a[5]*b[5] + a[6]*b[4] + a[7]*b[3]) << 12) + \
        ((a[2]*b[7] + a[3]*b[6] + a[4]*b[5] + a[5]*b[4] + a[6]*b[3] + a[7]*b[2]) << 8) + \
        ((a[1]*b[7] + a[2]*b[6] + a[3]*b[5] + a[4]*b[4] + a[5]*b[3] + a[6]*b[2] + a[7]*b[1]) << 4) + \
        (a[0]*b[7] + a[1]*b[6] + a[2]*b[5] + a[3]*b[4] + a[4]*b[3] + a[5]*b[2] + a[6]*b[1] + a[7]*b[0]) + \
        ((a[0]*b[6] + a[1]*b[5] + a[2]*b[4] + a[3]*b[3] + a[4]*b[2] + a[5]*b[1] + a[6]*b[0]) >> 4)
    return r & 0xffffffff

def test_mul(binary, a, b):
    r = fp_mul(a,b)
    commands = ['u main', f'poke d fooa {a}', f'poke d foob {b}', 'u main_exit', f'check d foor {r}', 'quit']
    args = ['rsim']
    for cmd in commands:
        args += ['-c', cmd]
    args += [binary]
    print(f"Testing {a:08X}x{b:08X}...", end='')
    ret = subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if ret.returncode != 0:
        print("FAIL")
        print(ret.stdout.decode())
        sys.exit(1)
    else:
        print("OK")


binary = sys.argv[1]

# bits
for ba in range(32):
    a = 1 << ba
    for bb in range(32):
        b = 1 << bb
        test_mul(binary, a, b)

# random values
for _ in range(1000):
    a = random.randrange(0, 0x100000000)
    b = random.randrange(0, 0x100000000)
    test_mul(binary, a, b)
