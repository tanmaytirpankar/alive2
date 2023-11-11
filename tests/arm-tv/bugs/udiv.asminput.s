        .text
        .file   "udiv.asminput.ll"
        .globl  f3
        .p2align        2
        .type   f3,@function
f3:
        .cfi_startproc
        udiv    w8, w1, w0
        cmp     w0, #0
        cset    w9, eq
        tst     w2, #0x1
        lsl     w9, w9, #1
        csel    w0, w8, w9, ne
        ret
.Lfunc_end0:
        .size   f3, .Lfunc_end0-f3
        .cfi_endproc

        .section        ".note.GNU-stack","",@progbits
