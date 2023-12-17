    .global map             ; aligned 256
    .global change
    .global change_count

    .export engine_collect_changed

    .const ret = 0xc800 + 8 * 0
    .const idx = 0xc800 + 8 * 1
    .const pchange = 0xc800 + 8 * 2

    .const FLAG_NEW = 0x10
    .const FLAG_MOVED = 0x20
    .const FLAG_EXPLOSION = 0x40

    .section text.engine_collect_changed
engine_collect_changed:
    mov a, pl
    mov b, a
    mov a, ph
    ldi ph, hi(ret)
    ldi pl, lo(ret)
    st  b
    inc pl
    st  a

    ; idx = W * H - 1
    ldi pl, lo(idx)
    ldi a, lo(30 * 32 - 1)
    st  a
    inc pl
    ldi a, hi(30 * 32 - 1)
    st  a

    ; pchange = change
    ldi pl, lo(pchange)
    ldi a, lo(change)
    st  a
    inc pl
    ldi a, hi(change)
    st  a

    ; change_count = 0
    ldi ph, hi(change_count)
    ldi pl, lo(change_count)
    mov a, 0
    st  a
    inc pl
    st  a

engine_collect_changed_loop:
        ; b = map[idx]
        ldi ph, hi(idx + 1)
        ldi pl, lo(idx + 1)
        ld  a
        dec pl
        ld  pl
        ldi ph, hi(map)
        add ph, a
        ld  b
        ; reset flags
        ldi a, ~(FLAG_NEW | FLAG_MOVED)
        and a, b
        st  a

        ; FLAG_NEW set?
        ldi a, FLAG_NEW
        and a, b
        ldi ph, hi(engine_collect_changed_not_new)
        ldi pl, lo(engine_collect_changed_not_new)
        jz

        ; explosion?
        ldi a, FLAG_EXPLOSION
        and a, b
        ldi ph, hi(engine_collect_changed_rec_new)
        ldi pl, lo(engine_collect_changed_rec_new)
        jnz

        ; empty cell?
        ldi a, ~(FLAG_NEW | FLAG_MOVED)
        and a, b
        ldi ph, hi(engine_collect_changed_not_new)
        ldi pl, lo(engine_collect_changed_not_new)
        jnz


engine_collect_changed_rec_new:

        ; *pchange = idx
        ldi ph, hi(idx)
        ldi pl, lo(idx)
        ld  b
        ldi ph, hi(pchange)
        ldi pl, lo(pchange)
        ld  a
        inc pl
        ld  ph
        mov pl, a
        st  b
        ldi ph, hi(idx + 1)
        ldi pl, lo(idx + 1)
        ld  b
        ldi ph, hi(pchange)
        ldi pl, lo(pchange)
        ld  a
        inc pl
        ld  ph
        mov pl, a
        inc pl
        st  b

        ; pchange += 2
        ldi ph, hi(pchange)
        ldi pl, lo(pchange)
        ld  b
        inc pl
        ld  a
        inc b
        inc b
        adc a, 0
        st  a
        dec pl
        st  b

        ; change_count += 1
        ldi ph, hi(change_count)
        ldi pl, lo(change_count)
        ld  b
        inc pl
        ld  a
        inc b
        adc a, 0
        st  a
        dec pl
        st  b

engine_collect_changed_not_new:

        ; idx -= 1
        ldi ph, hi(idx)
        ldi pl, lo(idx)
        ld  b
        inc pl
        ld  a
        dec b
        sbb a, 0
        st  a
        dec pl
        st  b
        add a, 0
        ldi ph, hi(engine_collect_changed_loop)
        ldi pl, lo(engine_collect_changed_loop)
        jns

engine_collect_changed_exit:

    ldi ph, hi(ret)
    ldi pl, lo(ret)
    ld  a
    inc pl
    ld  ph
    mov pl, a
    jmp
