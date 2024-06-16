	.section	__TEXT,__text,regular,pure_instructions
	.build_version macos, 14, 0
	.globl	_f                              ; -- Begin function f
	.p2align	2
_f:                                     ; @f
	.cfi_startproc
; %bb.0:
	and	w8, w0, #0x7
	strb	w8, [x1]
	ret
	.cfi_endproc
                                        ; -- End function
.subsections_via_symbols
