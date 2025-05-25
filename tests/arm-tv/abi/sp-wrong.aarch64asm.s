        .text
        .file   "foo.ll"
        .globl  f1
        .p2align        2
        .type   f1,@function
f1:
        .cfi_startproc
        sub     sp, sp, #128
        .cfi_def_cfa_offset 128
        ldr     w0, [sp], #4
        ret
.Lfunc_end0:
        .size   f1, .Lfunc_end0-f1
        .cfi_endproc

        .section        ".note.GNU-stack","",@progbits
