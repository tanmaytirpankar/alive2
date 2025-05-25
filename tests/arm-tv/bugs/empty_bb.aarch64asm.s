	.text
	.file	"test-978123849.ll"
	.globl	truncate_source_phi_switch
	.p2align	2
	.type	truncate_source_phi_switch,@function
truncate_source_phi_switch:
	.cfi_startproc
	ldrb	w9, [x0]
	mov	w8, wzr
	b	.LBB0_3
.LBB0_1:
.LBB0_2:
	add	w9, w8, #1
	strb	w9, [x1]
	mov	w8, w9
.LBB0_3:
	and	w10, w9, #0xff
	cmp	w10, #45
	b.eq	.LBB0_1
	cmp	w10, #43
	b.ne	.LBB0_6
	eor	w2, w2, #0x1
	b	.LBB0_2
.LBB0_6:
	sub	w9, w9, #1
	mov	w2, w8
	and	w9, w9, #0xff
	cmp	w9, #5
	b.hs	.LBB0_2
	ret
.Lfunc_end0:
	.size	truncate_source_phi_switch, .Lfunc_end0-truncate_source_phi_switch
	.cfi_endproc

	.section	".note.GNU-stack","",@progbits
