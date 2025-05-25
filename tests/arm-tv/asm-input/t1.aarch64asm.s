	.text
	.file	"add.ll"
	.globl	add                             // -- Begin function add
	.p2align	2
	.type	add,@function
add:                                    // @add
	.cfi_startproc
// %bb.0:
	add	w0, w0, w1
	ret
.Lfunc_end0:
	.size	add, .Lfunc_end0-add
	.cfi_endproc
                                        // -- End function
	.section	".note.GNU-stack","",@progbits
