    .export tcpip_chksum

    .const frameb_arg1 = 0xc800 + 8 * 0
    .const frameb_arg2 = 0xc800 + 8 * 1
    .const frameb_arg3 = 0xc800 + 8 * 2
    .const ret_addr    = 0xc800 + 8 * 3

    .const sum = 0xc800 + 8 * 0
    .const data = 0xc800 + 8 * 1
    .const length = 0xc800 + 8 * 2
    .const pointer = 0xc800 + 8 * 4
    .const end = 0xc800 + 8 * 5

    .global __cc_ret

    ; uint16_t tcpip_chksum(uint16_t sum, const uint8_t *data, uint16_t length)
    ; sum - host byte order
    ; data - net byte order, must be aligned 2
    ; result - host byte order
    .section text.tcpip_chksum
tcpip_chksum:
    mov a, pl
    mov b, a
    mov a, ph
    ldi pl, lo(ret_addr)
    ldi ph, hi(ret_addr)
    st  b
    inc pl
    st  a

    ldi pl, lo(length)
    ld  a
    shr a
    ldi ph, hi(tcpip_chksum_length_even)
    ldi pl, lo(tcpip_chksum_length_even)
    jnc

    ; length is odd: add last byte to the sum

    ; length -= 1
    ldi ph, hi(length)
    ldi pl, lo(length)
    ld  b
    dec b
    st  b

    ; *(data + length)
    ldi pl, lo(data)
    ld  a
    add b, a
    ldi pl, lo(pointer)
    st  b
    ldi pl, lo(data + 1)
    ld  b
    ldi pl, lo(length + 1)
    ld  a
    adc a, b
    ldi pl, lo(pointer)
    ld  pl
    mov ph, a

    ld  b ; b = data[length]
    ldi ph, hi(sum)
    ldi pl, lo(sum + 1)
    ld  a
    add a, b
    st  a
    ldi pl, lo(sum)
    ld  a
    adc a, 0
    st  a

tcpip_chksum_length_even:
    ; 1. sum lo bytes
    ; pointer = data + 1
    ldi ph, hi(data)
    ldi pl, lo(data)
    ld  b
    inc pl
    ld  a
    inc b
    adc a, 0
    ldi pl, lo(pointer)
    st  b
    inc pl
    st  a

    ; end = pointer + length
    ldi pl, lo(length)
    ld  a
    add b, a
    ldi pl, lo(end)
    st  b
    ldi pl, lo(pointer + 1)
    ld  a
    ldi pl, lo(length + 1)
    ld  b
    adc b, a
    ldi pl, lo(end + 1)
    st  b

tcpip_chksum_sum_lo_loop:
    ; if pointer == end goto loop end
    ldi ph, hi(pointer)
    ldi pl, lo(pointer)
    ld  a
    ldi pl, lo(end)
    ld  b
    sub  b, a
    inc pl
    ld  a
    ldi pl, lo(pointer + 1)
    ld  pl
    sub a, pl
    or  a, b
    ldi ph, hi(tcpip_chksum_sum_lo_loop_end)
    ldi pl, lo(tcpip_chksum_sum_lo_loop_end)
    jz

    ldi ph, hi(pointer)
    ldi pl, lo(pointer)
    ld  a
    inc pl
    ld  ph
    mov pl, a
    ld  b
    ldi ph, hi(sum)
    ldi pl, lo(sum)
    ld  a
    add a, b
    st  a
    ldi pl, lo(sum + 1)
    ld  a
    adc a, 0
    st  a
    ldi pl, lo(sum)
    ld  a
    adc a, 0
    st  a

    ; pointer += 2
    ldi ph, hi(pointer)
    ldi pl, lo(pointer)
    ld  b
    inc pl
    ld  a
    inc b    ; pointer is odd - only this can overflow
    adc a, 0
    inc b
    st  a
    dec pl
    st  b

    ldi ph, hi(tcpip_chksum_sum_lo_loop)
    ldi pl, lo(tcpip_chksum_sum_lo_loop)
    jmp
tcpip_chksum_sum_lo_loop_end:

    ; 2. sum hi bytes
    ; pointer = data
    ldi ph, hi(data)
    ldi pl, lo(data)
    ld  b
    inc pl
    ld  a
    ldi pl, lo(pointer)
    st  b
    inc pl
    st  a

    ; end -= 1
    ldi pl, lo(end)
    ld  b
    inc pl
    ld  a
    dec b
    sbb a, 0
    st  a
    dec pl
    st  b

tcpip_chksum_sum_hi_loop:
    ; if pointer == end goto loop end
    ldi ph, hi(pointer)
    ldi pl, lo(pointer)
    ld  a
    ldi pl, lo(end)
    ld  b
    sub  b, a
    inc pl
    ld  a
    ldi pl, lo(pointer + 1)
    ld  pl
    sub a, pl
    or  a, b
    ldi ph, hi(tcpip_chksum_sum_hi_loop_end)
    ldi pl, lo(tcpip_chksum_sum_hi_loop_end)
    jz

    ldi ph, hi(pointer)
    ldi pl, lo(pointer)
    ld  a
    inc pl
    ld  ph
    mov pl, a
    ld  b
    ldi ph, hi(sum)
    ldi pl, lo(sum + 1)
    ld  a
    add a, b
    st  a
    ldi pl, lo(sum)
    ld  a
    adc a, 0
    st  a

    ; pointer += 2
    ldi ph, hi(pointer)
    ldi pl, lo(pointer)
    ld  b
    inc pl
    ld  a
    inc b
    inc b    ; pointer is even - only this can overflow
    adc a, 0
    st  a
    dec pl
    st  b

    ldi ph, hi(tcpip_chksum_sum_hi_loop)
    ldi pl, lo(tcpip_chksum_sum_hi_loop)
    jmp
tcpip_chksum_sum_hi_loop_end:

    ; return sum
    ldi ph, hi(sum)
    ldi pl, lo(sum)
    ld  b
    inc pl
    ld  a
    ldi ph, hi(__cc_ret)
    ldi pl, lo(__cc_ret)
    st  b
    inc pl
    st  a

    ldi pl, lo(ret_addr)
    ldi ph, hi(ret_addr)
    ld  a
    inc pl
    ld  ph
    mov pl, a
    jmp
