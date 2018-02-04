
%include "sseutils64.nasm"

section	.text
	global	getVectorPiIn_double
getVectorPiIn_double:
	push	rbp
	mov	rbp, rsp
	pushaq
	vpushay
	;mov	rdi, [ebp+n]
	;mov	rsi, [ebp+pi]
	;movsd	xmm0, [ebp+e]
	vmovsd	xmm1, xmm0
	vpermilpd	xmm1, xmm1, 0
	vperm2f128	ymm1, ymm1, ymm1, 0
	mov	rbx, rdi
	sub	rbx, 32
	xor	r10, r10
cicloi:
	cmp	r10, rbx
	jge	cicloR
	vmovapd	[rsi + r10*8], ymm1
	vmovapd	[rsi + r10*8 + 32], ymm1
	vmovapd	[rsi + r10*8 + 64], ymm1
	vmovapd	[rsi + r10*8 + 96], ymm1
	add 	r10, 32
	jmp	cicloi
cicloR:
	cmp	r10, rdi
	jge	fine
	vmovsd	[rsi+r10*8], xmm1	;Pi[n*8+i*8] = 0	
	inc	r10
	jmp	cicloR
fine:
	vpopay
	popaq
	mov	rsp, rbp
	pop	rbp
	ret
