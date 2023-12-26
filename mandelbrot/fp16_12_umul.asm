    ; Fixed point number multiplication (16 bit, 12 bit shift)
    .export fp16_12_umul
    .global multable16x16
    .global __cc_ret

    .const param_a  = 0xc800 + 8 * 0
    .const param_b  = 0xc800 + 8 * 1
    .const ret      = 0xc800 + 8 * 2
    .const a_odd    = 0xc800 + 8 * 3
    .const a_even   = 0xc800 + 8 * 0
    .const b_odd    = 0xc800 + 8 * 1
    .const b_even   = 0xc800 + 8 * 4
    .const tmp      = 0xc800 + 8 * 5

    ; (a3b3) << 12 +
    ; (a3b2 + a2b3) << 8 +
    ; (a3b1 + a2b2 + a1b3) << 4 +
    ; (a3b0 + a2b1 + a1b2 + a0b3) +
    ; (a2b0 + a1b1 + a0b2) >> 4

    .section text.fp16_12_umul
fp16_12_umul:
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
    ; clear low nibbles in A_odd
    ldi a, 0xf0
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

    ; copy B to B_even
    ldi pl, lo(param_b)
    ld  a
    inc pl
    ld  b
    ldi pl, lo(b_even)
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

    ; r = 0
    ldi pl, lo(__cc_ret)
    ldi ph, hi(__cc_ret)
    mov a, 0
    st  a
    inc pl
    st  a

    ; (a2b0 + a1b1 + a0b2) >> 4
offset_neg4:
    ldi ph, hi(a_odd)
    ldi pl, lo(a_even + 0)   ; a0
    ld  a
    ldi pl, lo(b_even + 1)   ; b2
    ld  pl
    or  pl, a
    ldi ph, hi(multable16x16)
    ld  a
    ldi ph, hi(tmp)
    ldi pl, lo(tmp)
    st  a

    ldi pl, lo(a_odd + 0)   ; a1
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
    mov a, 0
    adc a, 0
    inc pl
    st  a

    ldi pl, lo(a_even + 1)   ; a2
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

    ; (a3b0 + a2b1 + a1b2 + a0b3)
offset_0:
    ldi ph, hi(a_odd)
    ldi pl, lo(a_even + 0)   ; a0
    ld  a
    ldi pl, lo(b_odd + 1)   ; b3
    ld  pl
    or  pl, a
    ldi ph, hi(multable16x16)
    ld  a
    ldi ph, hi(tmp)
    ldi pl, lo(tmp)
    st  a

    ldi pl, lo(a_odd + 0)   ; a1
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
    mov a, 0
    adc a, 0
    inc pl
    st  a

    ldi pl, lo(a_even + 1)   ; a2
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

    ldi pl, lo(a_odd + 1)   ; a3
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

    ; (a3b1 + a2b2 + a1b3) << 4
offset_4:
    ldi ph, hi(a_odd)
    ldi pl, lo(a_odd + 0)   ; a1
    ld  a
    ldi pl, lo(b_odd + 1)   ; b3
    ld  pl
    or  pl, a
    ldi ph, hi(multable16x16)
    ld  a
    ldi ph, hi(tmp)
    ldi pl, lo(tmp)
    st  a

    ldi pl, lo(a_even + 1)   ; a2
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
    mov a, 0
    adc a, 0
    inc pl
    st  a

    ldi pl, lo(a_odd + 1)   ; a3
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
    shr b
    shr b
    shr b
    shr b
    or  b, a
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
    ldi ph, hi(__cc_ret)
    ldi pl, lo(__cc_ret + 1)
    st  b

    ; (a3b2 + a2b3) << 8
offset_8:
    ldi ph, hi(a_odd)
    ldi pl, lo(a_even + 1)   ; a2
    ld  a
    ldi pl, lo(b_odd + 1)   ; b3
    ld  pl
    or  pl, a
    ldi ph, hi(multable16x16)
    ld  a
    ldi ph, hi(tmp)
    ldi pl, lo(tmp)
    st  a

    ldi pl, lo(a_odd + 1)   ; a3
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

    ; R[1..] += tmp
    ldi ph, hi(__cc_ret + 1)
    ldi pl, lo(__cc_ret + 1)
    ld  a
    add b, a
    st  b

    ; (a3b3) << 12
offset_12:
    ldi ph, hi(a_odd)
    ldi pl, lo(a_odd + 1)   ; a3
    ld  a
    ldi pl, lo(b_odd + 1)   ; b3
    ld  pl
    or  pl, a
    ldi ph, hi(multable16x16)
    ld  b

    ; R[1..] += tmp << 4
    shl b
    shl b
    shl b
    shl b
    ldi ph, hi(__cc_ret)
    ldi pl, lo(__cc_ret + 1)
    ld  a
    add b, a
    st  b

    ldi ph, hi(ret)
    ldi pl, lo(ret)
    ld  a
    inc pl
    ld  ph
    mov pl, a
    jmp
