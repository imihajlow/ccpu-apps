    .global fat_open_path
    .global fat_read
    .global fat_write
    .global fat_close
    .global fat_truncate
    .global fat_seek_end
    .global fat_get_size
    .global fat_get_next_dir_entry
    .global fat_mount
    .global fat_exec
    .global fat_exec_fd
    .global fat_get_last_error

    .global __cc_ret

    .const syscall_number = 0xc800 + 0 * 8
    .const frameb_ret_val = 0xc800 + 0 * 8

    .section ramtext
    .align 0x1000
os_entry:
    mov a, pl
    mov b, a
    mov a, ph
    ldi ph, hi(os_entry_ret)
    ldi pl, lo(os_entry_ret)
    st  b
    inc pl
    st  a

    ldi pl, lo(0xff02)
    ldi ph, hi(0xff02)
    ldi a, 0xde ; enable segments A-B, D-E both boards, disable lo RAM
    st a

    ; call requested function
    ldi ph, hi(syscall_number)
    ldi pl, lo(syscall_number)
    ld  a
    shl a ; less than 128 syscalls
    ldi ph, hi(syscall_table)
    ldi pl, lo(syscall_table)
    add pl, a
    mov a, 0
    adc ph, a
    ld  a
    inc pl
    ld  ph
    mov pl, a
    jmp

    ; save return value on frame B
    ldi ph, hi(__cc_ret + 0)
    ldi pl, lo(__cc_ret + 0)
    ld  a
    inc pl
    ld  b
    ldi  ph, hi(frameb_ret_val + 0)
    ldi  pl, lo(frameb_ret_val + 0)
    st  a
    inc pl
    st  b
    ldi ph, hi(__cc_ret + 2)
    ldi pl, lo(__cc_ret + 2)
    ld  a
    inc pl
    ld  b
    ldi ph, hi(frameb_ret_val + 2)
    ldi pl, lo(frameb_ret_val + 2)
    st  a
    inc pl
    st  b
    ldi ph, hi(__cc_ret + 4)
    ldi pl, lo(__cc_ret + 4)
    ld  a
    inc pl
    ld  b
    ldi ph, hi(frameb_ret_val + 4)
    ldi pl, lo(frameb_ret_val + 4)
    st  a
    inc pl
    st  b
    ldi ph, hi(__cc_ret + 6)
    ldi pl, lo(__cc_ret + 6)
    ld  a
    inc pl
    ld  b
    ldi ph, hi(frameb_ret_val + 6)
    ldi pl, lo(frameb_ret_val + 6)
    st  a
    inc pl
    st  b

    ldi pl, lo(0xff02)
    ldi ph, hi(0xff02)
    ldi a, 0xdf ; enable segments A-B, D-E, both boards, lo RAM
    st a

    ldi ph, hi(os_entry_ret)
    ldi pl, lo(os_entry_ret)
    ld  a
    inc pl
    ld  ph
    mov pl, a
    jmp


    .section bss
    .align 2
os_entry_ret: res 2


    .section rodata
    .align 2
syscall_table:
    dw fat_open_path
    dw fat_read
    dw fat_write
    dw fat_close
    dw fat_truncate
    dw fat_seek_end
    dw fat_get_size
    dw fat_get_next_dir_entry
    dw fat_mount
    dw fat_exec
    dw fat_exec_fd
    dw fat_get_last_error
    dw 0x0000
