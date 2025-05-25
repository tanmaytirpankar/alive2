	.attribute	4, 16
	.attribute	5, "rv64i2p1"
	.file	"f13b.ll"
	.text
	.globl	f13                             # -- Begin function f13
	.p2align	2
	.type	f13,@function
f13:                                    # @f13
	.cfi_startproc
# %bb.0:                                # %entry
	sub	a0, a0, a1
	ret
.Lfunc_end0:
	.size	f13, .Lfunc_end0-f13
	.cfi_endproc
                                        # -- End function
	.section	".note.GNU-stack","",@progbits
