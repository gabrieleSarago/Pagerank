
%include "sseutils64.nasm"

section .data
	align	32
	m:	dd	0x7fffffff, 0x7fffffff, 0x7fffffff, 0x7fffffff, 0x7fffffff, 0x7fffffff, 0x7fffffff, 0x7fffffff

section .text
	global	getPagrnk_single
getPagrnk_single:
	push	rbp
	mov	rbp, rsp
	pushaq
	vpushay	
	;mov	rdi, [ebp+n]
	;mov	rsi, [ebp+pik]
	vxorps	xmm0, xmm0		;somma = 0
	vmovaps	ymm2, [m]
	xor	r10, r10		;i = 0
ciclo1:
	cmp	r10, rdi
	jge	fineciclo1
	vmovaps	ymm1, [rsi+r10*4]	;salva in xmm1 Pik[i...i+p-1]
	vandps	ymm1, ymm2
	vhaddps	ymm1, ymm1
	vhaddps	ymm1, ymm1
	vperm2f128	ymm3, ymm1, ymm1, 1
	vaddss	xmm1, xmm3
	vaddss	xmm0, xmm1		;somma += |Pik[i...i+p-1]|
	add	r10, 8
	jmp	ciclo1
fineciclo1:
	vpermilps	xmm0, xmm0, 0
	xor	r10, r10
ciclo2:
	cmp	r10, rdi
	jge	fine
	vmovaps	ymm1, [rsi+r10*4]	;salva in xmm0	Pik[i...i+p-1]
	vdivps	ymm1, ymm0
	vmovaps	[rsi+r10*4], ymm1
	add	r10, 8
	jmp	ciclo2
fine:
	vpopay
	popaq
	mov	rsp, rbp
	pop	rbp
	ret
