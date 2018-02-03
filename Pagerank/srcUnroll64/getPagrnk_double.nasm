
%include "sseutils64.nasm"

section .data
	align	32
	m	dq	0x7fffffffffffffff, 0x7fffffffffffffff, 0x7fffffffffffffff, 0x7fffffffffffffff

section .text
	global	getPagrnk_double
getPagrnk_double:
	push	rbp
	mov	rbp, rsp
	pushaq
	vpushay
	;mov	rdi, [ebp+n]
	;mov	rsi, [ebp+pik]
	vxorpd	xmm0, xmm0		;somma = 0
	vmovapd	ymm2, [m]
	xor	r10, r10		;i = 0
ciclo1:
	cmp	r10, rdi
	jge	fineciclo1
	vmovapd	ymm1, [rsi+r10*8]	;salva in ebx Pik[i...i+p-1]
	vandpd	ymm1, ymm2
	vhaddpd	ymm1, ymm1
	vperm2f128	ymm3, ymm1, ymm1, 1
	vaddsd	xmm1, xmm3
	vaddsd	xmm0, xmm1		;somma += |Pik[i...i+p-1]|
	vmovapd	ymm1, [rsi+r10*8 + 32]	;salva in ebx Pik[i...i+p-1]
	vandpd	ymm1, ymm2
	vhaddpd	ymm1, ymm1
	vperm2f128	ymm3, ymm1, ymm1, 1
	vaddsd	xmm1, xmm3
	vaddsd	xmm0, xmm1		;somma += |Pik[i...i+p-1]|
	vmovapd	ymm1, [rsi+r10*8 + 64]	;salva in ebx Pik[i...i+p-1]
	vandpd	ymm1, ymm2
	vhaddpd	ymm1, ymm1
	vperm2f128	ymm3, ymm1, ymm1, 1
	vaddsd	xmm1, xmm3
	vaddsd	xmm0, xmm1		;somma += |Pik[i...i+p-1]|
	vmovapd	ymm1, [rsi+r10*8 + 96]	;salva in ebx Pik[i...i+p-1]
	vandpd	ymm1, ymm2
	vhaddpd	ymm1, ymm1
	vperm2f128	ymm3, ymm1, ymm1, 1
	vaddsd	xmm1, xmm3
	vaddsd	xmm0, xmm1		;somma += |Pik[i...i+p-1]|
	add	r10, 16
	jmp	ciclo1
fineciclo1:
	vpermilpd	xmm0, xmm0, 0
	vperm2f128	ymm0, ymm0, ymm0, 0
	xor	r10, r10
ciclo2:
	cmp	r10, rdi
	jge	fine
	vmovapd	ymm1, [rsi+r10*8]	;salva in ymm0	Pik[i...i+p-1]
	vdivpd	ymm1, ymm0
	vmovapd	[rsi+r10*8], ymm1
	vmovapd	ymm1, [rsi+r10*8 + 32]	;salva in ymm0	Pik[i...i+p-1]
	vdivpd	ymm1, ymm0
	vmovapd	[rsi+r10*8 + 32], ymm1
	vmovapd	ymm1, [rsi+r10*8 + 64]	;salva in ymm0	Pik[i...i+p-1]
	vdivpd	ymm1, ymm0
	vmovapd	[rsi+r10*8 + 64], ymm1
	vmovapd	ymm1, [rsi+r10*8 + 96]	;salva in ymm0	Pik[i...i+p-1]
	vdivpd	ymm1, ymm0
	vmovapd	[rsi+r10*8 + 96], ymm1
	add	r10, 16
	jmp	ciclo2
fine:
	vpopay
	popaq
	mov	rsp, rbp
	pop	rbp
	ret
