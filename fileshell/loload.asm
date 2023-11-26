    .export copy_to_loram
    .export loload_buf

    .const frameb_arg1 = 0xc800 + 0 * 8
    .const ret         = 0xc800 + 1 * 8

    .const dst_hi      = 0xc800 + 0 * 8

    .section ramtext.loload
copy_to_loram:
    mov a, pl
    mov b, a
    mov a, ph
    ldi pl, lo(ret)
    ldi ph, hi(ret)
    st b
    inc pl
    st a

    ldi pl, lo(0xff02)
    ldi ph, hi(0xff02)
    ldi a, 0x1f ; enable segments A-B, both boards, lo RAM
    st a
    mov a, 0 ; counter
copy_loop:
        ldi ph, hi(loload_buf)
        mov pl, a
        ld b
        ldi ph, hi(dst_hi)
        ldi pl, lo(dst_hi)
        ld ph
        mov pl, a
        st b
        inc a
        ldi pl, lo(copy_loop)
        ldi ph, hi(copy_loop)
        jnc

    ldi pl, lo(0xff02)
    ldi ph, hi(0xff02)
    ldi a, 0x1e ; enable segments A-B, both boards, disble lo RAM
    st a

    ldi pl, lo(ret)
    ldi ph, hi(ret)
    ld a
    inc pl
    ld ph
    mov pl, a
    jmp

    .section bss.loload

    .align 256
loload_buf:
    res 256
