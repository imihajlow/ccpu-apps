    .global object_char_l   ; aligned 256
    .global object_char_r   ; aligned 256
    .global object_color    ; aligned 256
    .global map             ; aligned 256

    .export engine_render

    .const ret = 0xc800 + 8 * 0
    .const col = 0xc800 + 8 * 1
    .const row = 0xc800 + 8 * 2
    .const pobj = 0xc800 + 8 * 3
    .const pchar = 0xc800 + 8 * 4
    .const pcolor = 0xc800 + 8 * 5
    .const obj = 0xc800 + 8 * 6

    .section text.engine_render
engine_render:
    mov a, pl
    mov b, a
    mov a, ph
    ldi ph, hi(ret)
    ldi pl, lo(ret)
    st  b
    inc pl
    st  a

    ; pcolor = VGA_COLOR_SEG
    ldi pl, lo(pcolor)
    mov a, 0
    st  a
    inc pl
    ldi b, 0xd0 ; VGA color segment hi address
    st  b

    ; pchar = VGA_CHAR_SEG
    ldi pl, lo(pchar)
    st  a
    inc pl
    ldi b, 0xe0 ; VGA char segment hi address
    st  b

    ; pobj = map
    ldi pl, lo(pobj)
    st  a
    inc pl
    ldi b, hi(map)
    st  b

    ldi a, 30
    ldi pl, lo(row)
    st  a
engine_render_row_loop:
        ldi a, 32
        ldi ph, hi(col)
        ldi pl, lo(col)
        st  a

engine_render_col_loop:
            ; obj = *pobj
            ldi ph, hi(pobj)
            ldi pl, lo(pobj)
            ld  a
            inc pl
            ld  ph
            mov pl, a
            ld  a
            ldi ph, hi(obj)
            ldi pl, lo(obj)
            st  a

            ; pcolor[0] = object_color[obj]
            ; pcolor[1] = object_color[obj]
            mov pl, a
            ldi ph, hi(object_color)
            ld  b
            ldi ph, hi(pcolor)
            ldi pl, lo(pcolor)
            ld  a
            inc pl
            ld  ph
            mov pl, a
            st  b
            inc pl
            st  b

            ; pchar[0] = object_char_l[obj]
            ldi ph, hi(obj)
            ldi pl, lo(obj)
            ld  pl
            ldi ph, hi(object_char_l)
            ld  b
            ldi ph, hi(pchar)
            ldi pl, lo(pchar)
            ld  a
            inc pl
            ld  ph
            mov pl, a
            st  b

            ; pchar[1] = object_char_r[obj]
            ldi ph, hi(obj)
            ldi pl, lo(obj)
            ld  pl
            ldi ph, hi(object_char_r)
            ld  b
            ldi ph, hi(pchar)
            ldi pl, lo(pchar)
            ld  a
            inc pl
            ld  ph
            mov pl, a
            inc pl
            st  b

            ; pcolor += 2
            ; overflow cannot happen
            ldi ph, hi(pcolor)
            ldi pl, lo(pcolor)
            ld  a
            inc a
            inc a
            st  a

            ; pchar += 2
            ; overflow cannot happen
            ldi pl, lo(pchar)
            ld  a
            inc a
            inc a
            st  a

            ; pobj += 1
            ldi pl, lo(pobj)
            ld  b
            inc pl
            ld  a
            inc b
            adc a, 0
            st  a
            dec pl
            st  b

            ldi ph, hi(col)
            ldi pl, lo(col)
            ld  a
            dec a
            st  a
            ldi ph, hi(engine_render_col_loop)
            ldi pl, lo(engine_render_col_loop)
            jnz

        ; pcolor += 128 - 64
        ldi ph, hi(pcolor)
        ldi pl, lo(pcolor)
        ldi a, 64
        ld  b
        inc pl
        add b, a
        ld  a
        adc a, 0
        st  a
        dec pl
        st  b

        ; pchar += 128 - 64
        ldi pl, lo(pchar)
        ldi a, 64
        ld  b
        inc pl
        add b, a
        ld  a
        adc a, 0
        st  a
        dec pl
        st  b

        ldi ph, hi(row)
        ldi pl, lo(row)
        ld  a
        dec a
        st  a
        ldi ph, hi(engine_render_row_loop)
        ldi pl, lo(engine_render_row_loop)
        jnz

    ldi ph, hi(ret)
    ldi pl, lo(ret)
    ld  a
    inc pl
    ld  ph
    mov pl, a
    jmp
