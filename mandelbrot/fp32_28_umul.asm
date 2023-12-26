    ; Fixed point number multiplication (32 bit, 28 bit shift)
    .export fp32_28_umul
    .export multable16x16
    .global __cc_ret

    .const param_a  = 0xc800 + 8 * 0
    .const param_b  = 0xc800 + 8 * 1
    .const ret      = 0xc800 + 8 * 2
    .const a_odd    = 0xc800 + 8 * 3
    .const a_even   = 0xc800 + 8 * 0
    .const b_odd    = 0xc800 + 8 * 1
    .const b_even   = 0xc800 + 8 * 4
    .const tmp      = 0xc800 + 8 * 5

    ; (a7b7) << 28 +
    ; (a6b7 + a7b6) << 24 +
    ; (a5b7 + a6b6 + a7b5) << 20 +
    ; (a4b7 + a5b6 + a6b5 + a7b4) << 16 +
    ; (a3b7 + a4b6 + a5b5 + a6b4 + a7b3) << 12 +
    ; (a2b7 + a3b6 + a4b5 + a5b4 + a6b3 + a7b2) << 8 +
    ; (a1b7 + a2b6 + a3b5 + a4b4 + a5b3 + a6b2 + a7b1) << 4 +
    ; (a0b7 + a1b6 + a2b5 + a3b4 + a4b3 + a5b2 + a6b1 + a7b0) +
    ; (a0b6 + a1b5 + a2b4 + a3b3 + a4b2 + a5b1 + a6b0) >> 4

    .section text.fp32_28_umul
fp32_28_umul:
    mov a, pl
    mov b, a
    mov a, ph
    ldi ph, hi(ret)
    ldi pl, lo(ret)
    st  b
    inc pl
    st  a

    ; Prepare A and B: separate nibbles, for A nibbles fill high halves, for B - low halves
    ; copy A to A_odd
    ldi pl, lo(param_a)
    ld  a
    inc pl
    ld  b
    ldi pl, lo(a_odd)
    st  a
    inc pl
    st  b
    ldi pl, lo(param_a + 2)
    ld  a
    inc pl
    ld  b
    ldi pl, lo(a_odd + 2)
    st  a
    inc pl
    ; clear low nibbles in A_odd
    ldi a, 0xf0
    and b, a
    st  b
    dec pl
    ld  b
    and b, a
    st  b
    dec pl
    ld  b
    and b, a
    st  b
    dec pl
    ld  b
    and b, a
    st  b
    ; shift A_even
    ldi pl, lo(a_even)
    ld  a
    shl a
    shl a
    shl a
    shl a
    st  a
    inc pl
    ld  a
    shl a
    shl a
    shl a
    shl a
    st  a
    inc pl
    ld  a
    shl a
    shl a
    shl a
    shl a
    st  a
    inc pl
    ld  a
    shl a
    shl a
    shl a
    shl a
    st  a

    ; copy B to B_even
    ldi pl, lo(param_b)
    ld  a
    inc pl
    ld  b
    ldi pl, lo(b_even)
    st  a
    inc pl
    st  b
    ldi pl, lo(param_b + 2)
    ld  a
    inc pl
    ld  b
    ldi pl, lo(b_even + 2)
    st  a
    inc pl
    ; clear high nibbles in B_even
    ldi a, 0x0f
    and b, a
    st  b
    dec pl
    ld  b
    and b, a
    st  b
    dec pl
    ld  b
    and b, a
    st  b
    dec pl
    ld  b
    and b, a
    st  b
    ; shift B_odd
    ldi pl, lo(b_odd)
    ld  a
    shr a
    shr a
    shr a
    shr a
    st  a
    inc pl
    ld  a
    shr a
    shr a
    shr a
    shr a
    st  a
    inc pl
    ld  a
    shr a
    shr a
    shr a
    shr a
    st  a
    inc pl
    ld  a
    shr a
    shr a
    shr a
    shr a
    st  a

    ; r = 0
    ldi pl, lo(__cc_ret)
    ldi ph, hi(__cc_ret)
    mov a, 0
    st  a
    inc pl
    st  a
    inc pl
    st  a
    inc pl
    st  a

    ; (a0b6 + a1b5 + a2b4 + a3b3 + a4b2 + a5b1 + a6b0) >> 4
offset_neg4:
    ldi ph, hi(a_odd)
    ldi pl, lo(a_even + 0)   ; a0
    ld  a
    ldi pl, lo(b_even + 3)   ; b6
    ld  pl
    or  pl, a
    ldi ph, hi(multable16x16)
    ld  a
    ldi ph, hi(tmp)
    ldi pl, lo(tmp)
    st  a

    ldi pl, lo(a_odd + 0)   ; a1
    ld  a
    ldi pl, lo(b_odd + 2)   ; b5
    ld  pl
    or  pl, a
    ldi ph, hi(multable16x16)
    ld  a
    ldi ph, hi(tmp)
    ldi pl, lo(tmp)
    ld  b
    add b, a
    st  b
    mov a, 0
    adc a, 0
    inc pl
    st  a

    ldi pl, lo(a_even + 1)   ; a2
    ld  a
    ldi pl, lo(b_even + 2)   ; b4
    ld  pl
    or  pl, a
    ldi ph, hi(multable16x16)
    ld  a
    ldi ph, hi(tmp)
    ldi pl, lo(tmp)
    ld  b
    add b, a
    st  b
    ldi pl, lo(tmp + 1)
    ld  a
    adc a, 0
    st  a

    ldi pl, lo(a_odd + 1)   ; a3
    ld  a
    ldi pl, lo(b_odd + 1)   ; b3
    ld  pl
    or  pl, a
    ldi ph, hi(multable16x16)
    ld  a
    ldi ph, hi(tmp)
    ldi pl, lo(tmp)
    ld  b
    add b, a
    st  b
    ldi pl, lo(tmp + 1)
    ld  a
    adc a, 0
    st  a

    ldi pl, lo(a_even + 2)   ; a4
    ld  a
    ldi pl, lo(b_even + 1)   ; b2
    ld  pl
    or  pl, a
    ldi ph, hi(multable16x16)
    ld  a
    ldi ph, hi(tmp)
    ldi pl, lo(tmp)
    ld  b
    add b, a
    st  b
    ldi pl, lo(tmp + 1)
    ld  a
    adc a, 0
    st  a

    ldi pl, lo(a_odd + 2)   ; a5
    ld  a
    ldi pl, lo(b_odd + 0)   ; b1
    ld  pl
    or  pl, a
    ldi ph, hi(multable16x16)
    ld  a
    ldi ph, hi(tmp)
    ldi pl, lo(tmp)
    ld  b
    add b, a
    st  b
    ldi pl, lo(tmp + 1)
    ld  a
    adc a, 0
    st  a

    ldi pl, lo(a_even + 3)   ; a6
    ld  a
    ldi pl, lo(b_even + 0)   ; b0
    ld  pl
    or  pl, a
    ldi ph, hi(multable16x16)
    ld  a
    ldi ph, hi(tmp)
    ldi pl, lo(tmp)
    ld  b
    add b, a
    st  b
    ldi pl, lo(tmp + 1)
    ld  a
    adc a, 0

    ; R = tmp >> 4
    shl a
    shl a
    shl a
    shl a
    dec pl
    ld  b
    shr b
    shr b
    shr b
    shr b
    or  b, a
    ldi ph, hi(__cc_ret)
    ldi pl, lo(__cc_ret + 0)
    st  b

    ; (a0b7 + a1b6 + a2b5 + a3b4 + a4b3 + a5b2 + a6b1 + a7b0)
offset_0:
    ldi ph, hi(a_odd)
    ldi pl, lo(a_even + 0)   ; a0
    ld  a
    ldi pl, lo(b_odd + 3)   ; b7
    ld  pl
    or  pl, a
    ldi ph, hi(multable16x16)
    ld  a
    ldi ph, hi(tmp)
    ldi pl, lo(tmp)
    st  a

    ldi pl, lo(a_odd + 0)   ; a1
    ld  a
    ldi pl, lo(b_even + 3)   ; b6
    ld  pl
    or  pl, a
    ldi ph, hi(multable16x16)
    ld  a
    ldi ph, hi(tmp)
    ldi pl, lo(tmp)
    ld  b
    add b, a
    st  b
    mov a, 0
    adc a, 0
    inc pl
    st  a

    ldi pl, lo(a_even + 1)   ; a2
    ld  a
    ldi pl, lo(b_odd + 2)   ; b5
    ld  pl
    or  pl, a
    ldi ph, hi(multable16x16)
    ld  a
    ldi ph, hi(tmp)
    ldi pl, lo(tmp)
    ld  b
    add b, a
    st  b
    ldi pl, lo(tmp + 1)
    ld  a
    adc a, 0
    st  a

    ldi pl, lo(a_odd + 1)   ; a3
    ld  a
    ldi pl, lo(b_even + 2)   ; b4
    ld  pl
    or  pl, a
    ldi ph, hi(multable16x16)
    ld  a
    ldi ph, hi(tmp)
    ldi pl, lo(tmp)
    ld  b
    add b, a
    st  b
    ldi pl, lo(tmp + 1)
    ld  a
    adc a, 0
    st  a

    ldi pl, lo(a_even + 2)   ; a4
    ld  a
    ldi pl, lo(b_odd + 1)   ; b3
    ld  pl
    or  pl, a
    ldi ph, hi(multable16x16)
    ld  a
    ldi ph, hi(tmp)
    ldi pl, lo(tmp)
    ld  b
    add b, a
    st  b
    ldi pl, lo(tmp + 1)
    ld  a
    adc a, 0
    st  a

    ldi pl, lo(a_odd + 2)   ; a5
    ld  a
    ldi pl, lo(b_even + 1)   ; b2
    ld  pl
    or  pl, a
    ldi ph, hi(multable16x16)
    ld  a
    ldi ph, hi(tmp)
    ldi pl, lo(tmp)
    ld  b
    add b, a
    st  b
    ldi pl, lo(tmp + 1)
    ld  a
    adc a, 0
    st  a

    ldi pl, lo(a_even + 3)   ; a6
    ld  a
    ldi pl, lo(b_odd + 0)   ; b1
    ld  pl
    or  pl, a
    ldi ph, hi(multable16x16)
    ld  a
    ldi ph, hi(tmp)
    ldi pl, lo(tmp)
    ld  b
    add b, a
    st  b
    ldi pl, lo(tmp + 1)
    ld  a
    adc a, 0
    st  a

    ldi pl, lo(a_odd + 3)   ; a7
    ld  a
    ldi pl, lo(b_even + 0)   ; b0
    ld  pl
    or  pl, a
    ldi ph, hi(multable16x16)
    ld  a
    ldi ph, hi(tmp)
    ldi pl, lo(tmp)
    ld  b
    add b, a
    st  b
    ldi pl, lo(tmp + 1)
    ld  a
    adc a, 0
    dec pl
    ld  b

    ; R += tmp
    ldi ph, hi(__cc_ret)
    ldi pl, lo(__cc_ret + 1)
    st  a
    dec pl
    ld  a
    add b, a
    st  b
    ldi pl, lo(__cc_ret + 1)
    ld  a
    adc a, 0
    st  a

    ; (a1b7 + a2b6 + a3b5 + a4b4 + a5b3 + a6b2 + a7b1) << 4
offset_4:
    ldi ph, hi(a_odd)
    ldi pl, lo(a_odd + 0)   ; a1
    ld  a
    ldi pl, lo(b_odd + 3)   ; b7
    ld  pl
    or  pl, a
    ldi ph, hi(multable16x16)
    ld  a
    ldi ph, hi(tmp)
    ldi pl, lo(tmp)
    st  a

    ldi pl, lo(a_even + 1)   ; a2
    ld  a
    ldi pl, lo(b_even + 3)   ; b6
    ld  pl
    or  pl, a
    ldi ph, hi(multable16x16)
    ld  a
    ldi ph, hi(tmp)
    ldi pl, lo(tmp)
    ld  b
    add b, a
    st  b
    mov a, 0
    adc a, 0
    inc pl
    st  a

    ldi pl, lo(a_odd + 1)   ; a3
    ld  a
    ldi pl, lo(b_odd + 2)   ; b5
    ld  pl
    or  pl, a
    ldi ph, hi(multable16x16)
    ld  a
    ldi ph, hi(tmp)
    ldi pl, lo(tmp)
    ld  b
    add b, a
    st  b
    ldi pl, lo(tmp + 1)
    ld  a
    adc a, 0
    st  a

    ldi pl, lo(a_even + 2)   ; a4
    ld  a
    ldi pl, lo(b_even + 2)   ; b4
    ld  pl
    or  pl, a
    ldi ph, hi(multable16x16)
    ld  a
    ldi ph, hi(tmp)
    ldi pl, lo(tmp)
    ld  b
    add b, a
    st  b
    ldi pl, lo(tmp + 1)
    ld  a
    adc a, 0
    st  a

    ldi pl, lo(a_odd + 2)   ; a5
    ld  a
    ldi pl, lo(b_odd + 1)   ; b3
    ld  pl
    or  pl, a
    ldi ph, hi(multable16x16)
    ld  a
    ldi ph, hi(tmp)
    ldi pl, lo(tmp)
    ld  b
    add b, a
    st  b
    ldi pl, lo(tmp + 1)
    ld  a
    adc a, 0
    st  a

    ldi pl, lo(a_even + 3)   ; a6
    ld  a
    ldi pl, lo(b_even + 1)   ; b2
    ld  pl
    or  pl, a
    ldi ph, hi(multable16x16)
    ld  a
    ldi ph, hi(tmp)
    ldi pl, lo(tmp)
    ld  b
    add b, a
    st  b
    ldi pl, lo(tmp + 1)
    ld  a
    adc a, 0
    st  a

    ldi pl, lo(a_odd + 3)   ; a7
    ld  a
    ldi pl, lo(b_odd + 0)   ; b1
    ld  pl
    or  pl, a
    ldi ph, hi(multable16x16)
    ld  a
    ldi ph, hi(tmp)
    ldi pl, lo(tmp)
    ld  b
    add b, a
    st  b
    ldi pl, lo(tmp + 1)
    ld  a
    adc a, 0

    ; R += tmp << 4
    shl a
    shl a
    shl a
    shl a
    dec pl
    ld  b
    shr b
    shr b
    shr b
    shr b
    or  b, a
    inc pl
    st  b
    dec pl
    ld  a
    shl a
    shl a
    shl a
    shl a
    ldi ph, hi(__cc_ret)
    ldi pl, lo(__cc_ret + 0)
    ld  b
    add b, a
    st  b
    ldi pl, lo(__cc_ret + 1)
    ld  a
    ldi ph, hi(tmp)
    ldi pl, lo(tmp + 1)
    ld  b
    adc b, a
    mov a, 0
    adc a, 0
    ldi ph, hi(__cc_ret)
    ldi pl, lo(__cc_ret + 1)
    st  b
    inc pl
    st  a

    ; (a2b7 + a3b6 + a4b5 + a5b4 + a6b3 + a7b2) << 8
offset_8:
    ldi ph, hi(a_odd)
    ldi pl, lo(a_even + 1)   ; a2
    ld  a
    ldi pl, lo(b_odd + 3)   ; b7
    ld  pl
    or  pl, a
    ldi ph, hi(multable16x16)
    ld  a
    ldi ph, hi(tmp)
    ldi pl, lo(tmp)
    st  a

    ldi pl, lo(a_odd + 1)   ; a3
    ld  a
    ldi pl, lo(b_even + 3)   ; b6
    ld  pl
    or  pl, a
    ldi ph, hi(multable16x16)
    ld  a
    ldi ph, hi(tmp)
    ldi pl, lo(tmp)
    ld  b
    add b, a
    st  b
    mov a, 0
    adc a, 0
    inc pl
    st  a

    ldi pl, lo(a_even + 2)   ; a4
    ld  a
    ldi pl, lo(b_odd + 2)   ; b5
    ld  pl
    or  pl, a
    ldi ph, hi(multable16x16)
    ld  a
    ldi ph, hi(tmp)
    ldi pl, lo(tmp)
    ld  b
    add b, a
    st  b
    ldi pl, lo(tmp + 1)
    ld  a
    adc a, 0
    st  a

    ldi pl, lo(a_odd + 2)   ; a5
    ld  a
    ldi pl, lo(b_even + 2)   ; b4
    ld  pl
    or  pl, a
    ldi ph, hi(multable16x16)
    ld  a
    ldi ph, hi(tmp)
    ldi pl, lo(tmp)
    ld  b
    add b, a
    st  b
    ldi pl, lo(tmp + 1)
    ld  a
    adc a, 0
    st  a

    ldi pl, lo(a_even + 3)   ; a6
    ld  a
    ldi pl, lo(b_odd + 1)   ; b3
    ld  pl
    or  pl, a
    ldi ph, hi(multable16x16)
    ld  a
    ldi ph, hi(tmp)
    ldi pl, lo(tmp)
    ld  b
    add b, a
    st  b
    ldi pl, lo(tmp + 1)
    ld  a
    adc a, 0
    st  a

    ldi pl, lo(a_odd + 3)   ; a7
    ld  a
    ldi pl, lo(b_even + 1)   ; b2
    ld  pl
    or  pl, a
    ldi ph, hi(multable16x16)
    ld  a
    ldi ph, hi(tmp)
    ldi pl, lo(tmp)
    ld  b
    add b, a
    st  b
    ldi pl, lo(tmp + 1)
    ld  a
    adc a, 0
    st  a

    ; R[1..] += tmp
    ldi ph, hi(__cc_ret + 1)
    ldi pl, lo(__cc_ret + 1)
    ld  a
    add b, a
    st  b
    ldi ph, hi(tmp)
    ldi pl, lo(tmp + 1)
    ld  a
    adc a, 0
    ldi ph, hi(__cc_ret + 2)
    ldi pl, lo(__cc_ret + 2)
    st  a


    ; (a3b7 + a4b6 + a5b5 + a6b4 + a7b3) << 12
offset_12:
    ldi ph, hi(a_odd)
    ldi pl, lo(a_odd + 1)   ; a3
    ld  a
    ldi pl, lo(b_odd + 3)   ; b7
    ld  pl
    or  pl, a
    ldi ph, hi(multable16x16)
    ld  a
    ldi ph, hi(tmp)
    ldi pl, lo(tmp)
    st  a

    ldi pl, lo(a_even + 2)   ; a4
    ld  a
    ldi pl, lo(b_even + 3)   ; b6
    ld  pl
    or  pl, a
    ldi ph, hi(multable16x16)
    ld  a
    ldi ph, hi(tmp)
    ldi pl, lo(tmp)
    ld  b
    add b, a
    st  b
    mov a, 0
    adc a, 0
    inc pl
    st  a

    ldi pl, lo(a_odd + 2)   ; a5
    ld  a
    ldi pl, lo(b_odd + 2)   ; b5
    ld  pl
    or  pl, a
    ldi ph, hi(multable16x16)
    ld  a
    ldi ph, hi(tmp)
    ldi pl, lo(tmp)
    ld  b
    add b, a
    st  b
    ldi pl, lo(tmp + 1)
    ld  a
    adc a, 0
    st  a

    ldi pl, lo(a_even + 3)   ; a6
    ld  a
    ldi pl, lo(b_even + 2)   ; b4
    ld  pl
    or  pl, a
    ldi ph, hi(multable16x16)
    ld  a
    ldi ph, hi(tmp)
    ldi pl, lo(tmp)
    ld  b
    add b, a
    st  b
    ldi pl, lo(tmp + 1)
    ld  a
    adc a, 0
    st  a

    ldi pl, lo(a_odd + 3)   ; a7
    ld  a
    ldi pl, lo(b_odd + 1)   ; b3
    ld  pl
    or  pl, a
    ldi ph, hi(multable16x16)
    ld  a
    ldi ph, hi(tmp)
    ldi pl, lo(tmp)
    ld  b
    add b, a
    st  b
    ldi pl, lo(tmp + 1)
    ld  a
    adc a, 0

    ; R[1..] += tmp << 4
    shl a
    shl a
    shl a
    shl a
    dec pl
    ld  b
    shr b
    shr b
    shr b
    shr b
    or  b, a
    inc pl
    st  b
    dec pl
    ld  a
    shl a
    shl a
    shl a
    shl a
    ldi ph, hi(__cc_ret)
    ldi pl, lo(__cc_ret + 1)
    ld  b
    add b, a
    st  b
    ldi pl, lo(__cc_ret + 2)
    ld  a
    ldi ph, hi(tmp)
    ldi pl, lo(tmp + 1)
    ld  b
    adc b, a
    mov a, 0
    adc a, 0
    ldi ph, hi(__cc_ret)
    ldi pl, lo(__cc_ret + 2)
    st  b
    inc pl
    st  a

    ; (a4b7 + a5b6 + a6b5 + a7b4) << 16
offset_16:
    ldi ph, hi(a_odd)
    ldi pl, lo(a_even + 2)   ; a4
    ld  a
    ldi pl, lo(b_odd + 3)   ; b7
    ld  pl
    or  pl, a
    ldi ph, hi(multable16x16)
    ld  a
    ldi ph, hi(tmp)
    ldi pl, lo(tmp)
    st  a

    ldi pl, lo(a_odd + 2)   ; a5
    ld  a
    ldi pl, lo(b_even + 3)   ; b6
    ld  pl
    or  pl, a
    ldi ph, hi(multable16x16)
    ld  a
    ldi ph, hi(tmp)
    ldi pl, lo(tmp)
    ld  b
    add b, a
    st  b
    mov a, 0
    adc a, 0
    inc pl
    st  a

    ldi pl, lo(a_even + 3)   ; a6
    ld  a
    ldi pl, lo(b_odd + 2)   ; b5
    ld  pl
    or  pl, a
    ldi ph, hi(multable16x16)
    ld  a
    ldi ph, hi(tmp)
    ldi pl, lo(tmp)
    ld  b
    add b, a
    st  b
    ldi pl, lo(tmp + 1)
    ld  a
    adc a, 0
    st  a

    ldi pl, lo(a_odd + 3)   ; a7
    ld  a
    ldi pl, lo(b_even + 2)   ; b4
    ld  pl
    or  pl, a
    ldi ph, hi(multable16x16)
    ld  a
    ldi ph, hi(tmp)
    ldi pl, lo(tmp)
    ld  b
    add b, a
    st  b
    ldi pl, lo(tmp + 1)
    ld  a
    adc a, 0
    st  a

    ; R[2..] += tmp
    ldi ph, hi(__cc_ret + 2)
    ldi pl, lo(__cc_ret + 2)
    ld  a
    add b, a
    st  b
    ldi ph, hi(tmp)
    ldi pl, lo(tmp + 1)
    ld  a
    adc a, 0
    ldi ph, hi(__cc_ret + 3)
    ldi pl, lo(__cc_ret + 3)
    st  a

    ; (a5b7 + a6b6 + a7b5) << 20
offset_20:
    ldi ph, hi(a_odd)
    ldi pl, lo(a_odd + 2)   ; a5
    ld  a
    ldi pl, lo(b_odd + 3)   ; b7
    ld  pl
    or  pl, a
    ldi ph, hi(multable16x16)
    ld  a
    ldi ph, hi(tmp)
    ldi pl, lo(tmp)
    st  a

    ldi pl, lo(a_even + 3)   ; a6
    ld  a
    ldi pl, lo(b_even + 3)   ; b6
    ld  pl
    or  pl, a
    ldi ph, hi(multable16x16)
    ld  a
    ldi ph, hi(tmp)
    ldi pl, lo(tmp)
    ld  b
    add b, a
    st  b
    mov a, 0
    adc a, 0
    inc pl
    st  a

    ldi pl, lo(a_odd + 3)   ; a7
    ld  a
    ldi pl, lo(b_odd + 2)   ; b5
    ld  pl
    or  pl, a
    ldi ph, hi(multable16x16)
    ld  a
    ldi ph, hi(tmp)
    ldi pl, lo(tmp)
    ld  b
    add b, a
    st  b
    ldi pl, lo(tmp + 1)
    ld  a
    adc a, 0

    ; R[2..] += tmp << 4
    shl a
    shl a
    shl a
    shl a
    dec pl
    ld  b
    shr b
    shr b
    shr b
    shr b
    or  b, a
    inc pl
    st  b
    dec pl
    ld  a
    shl a
    shl a
    shl a
    shl a
    ldi ph, hi(__cc_ret)
    ldi pl, lo(__cc_ret + 2)
    ld  b
    add b, a
    st  b
    ldi pl, lo(__cc_ret + 3)
    ld  a
    ldi ph, hi(tmp)
    ldi pl, lo(tmp + 1)
    ld  b
    adc b, a
    mov a, 0
    adc a, 0
    ldi ph, hi(__cc_ret)
    ldi pl, lo(__cc_ret + 3)
    st  b

    ; (a6b7 + a7b6) << 24
offset_24:
    ldi ph, hi(a_odd)
    ldi pl, lo(a_even + 3)   ; a6
    ld  a
    ldi pl, lo(b_odd + 3)   ; b7
    ld  pl
    or  pl, a
    ldi ph, hi(multable16x16)
    ld  a
    ldi ph, hi(tmp)
    ldi pl, lo(tmp)
    st  a

    ldi pl, lo(a_odd + 3)   ; a7
    ld  a
    ldi pl, lo(b_even + 3)   ; b6
    ld  pl
    or  pl, a
    ldi ph, hi(multable16x16)
    ld  a
    ldi ph, hi(tmp)
    ldi pl, lo(tmp)
    ld  b
    add b, a

    ; R[3] += tmp
    ldi ph, hi(__cc_ret + 3)
    ldi pl, lo(__cc_ret + 3)
    ld  a
    add b, a
    st  b

    ; (a7b7) << 28
offset_28:
    ldi ph, hi(a_odd)
    ldi pl, lo(a_odd + 3)   ; a7
    ld  a
    ldi pl, lo(b_odd + 3)   ; b7
    ld  pl
    or  pl, a
    ldi ph, hi(multable16x16)
    ld  a

    shl a
    shl a
    shl a
    shl a
    ldi ph, hi(__cc_ret + 3)
    ldi pl, lo(__cc_ret + 3)
    ld  b
    add b, a
    st  b

    ldi ph, hi(ret)
    ldi pl, lo(ret)
    ld  a
    inc pl
    ld  ph
    mov pl, a
    jmp

    ; Nibble multiplication table
    .section rodata.multable16x16
    .align 256
multable16x16:
    db 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
    db 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F
    db 0x00,0x02,0x04,0x06,0x08,0x0A,0x0C,0x0E,0x10,0x12,0x14,0x16,0x18,0x1A,0x1C,0x1E
    db 0x00,0x03,0x06,0x09,0x0C,0x0F,0x12,0x15,0x18,0x1B,0x1E,0x21,0x24,0x27,0x2A,0x2D
    db 0x00,0x04,0x08,0x0C,0x10,0x14,0x18,0x1C,0x20,0x24,0x28,0x2C,0x30,0x34,0x38,0x3C
    db 0x00,0x05,0x0A,0x0F,0x14,0x19,0x1E,0x23,0x28,0x2D,0x32,0x37,0x3C,0x41,0x46,0x4B
    db 0x00,0x06,0x0C,0x12,0x18,0x1E,0x24,0x2A,0x30,0x36,0x3C,0x42,0x48,0x4E,0x54,0x5A
    db 0x00,0x07,0x0E,0x15,0x1C,0x23,0x2A,0x31,0x38,0x3F,0x46,0x4D,0x54,0x5B,0x62,0x69
    db 0x00,0x08,0x10,0x18,0x20,0x28,0x30,0x38,0x40,0x48,0x50,0x58,0x60,0x68,0x70,0x78
    db 0x00,0x09,0x12,0x1B,0x24,0x2D,0x36,0x3F,0x48,0x51,0x5A,0x63,0x6C,0x75,0x7E,0x87
    db 0x00,0x0A,0x14,0x1E,0x28,0x32,0x3C,0x46,0x50,0x5A,0x64,0x6E,0x78,0x82,0x8C,0x96
    db 0x00,0x0B,0x16,0x21,0x2C,0x37,0x42,0x4D,0x58,0x63,0x6E,0x79,0x84,0x8F,0x9A,0xA5
    db 0x00,0x0C,0x18,0x24,0x30,0x3C,0x48,0x54,0x60,0x6C,0x78,0x84,0x90,0x9C,0xA8,0xB4
    db 0x00,0x0D,0x1A,0x27,0x34,0x41,0x4E,0x5B,0x68,0x75,0x82,0x8F,0x9C,0xA9,0xB6,0xC3
    db 0x00,0x0E,0x1C,0x2A,0x38,0x46,0x54,0x62,0x70,0x7E,0x8C,0x9A,0xA8,0xB6,0xC4,0xD2
    db 0x00,0x0F,0x1E,0x2D,0x3C,0x4B,0x5A,0x69,0x78,0x87,0x96,0xA5,0xB4,0xC3,0xD2,0xE1
