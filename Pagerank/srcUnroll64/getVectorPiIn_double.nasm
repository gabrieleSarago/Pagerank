
%include "sseutils64.nasm"

section	.text
	global	getVectorPiIn_double
getVectorPiIn_double:
	push	rbp
	mov	rbp, rsp
	pushaq
	vpushay
	;mov	rdi, [ebp+n]
	;mov	rdx, [ebp+pi]
	;mov	rsi, [ebp+o]
	;movsd	xmm0, [ebp+e]
	vmovsd	xmm1, xmm0
	vpermilpd	xmm1, xmm1, 0
	vperm2f128	ymm1, ymm1, ymm1, 0
	xor	r10, r10
cicloi:
	cmp	r10, rdi
	jge	finecicloi
	vmovapd	[rdx + r10*8], ymm1
	vmovapd	[rdx + r10*8 + 32], ymm1
	vmovapd	[rdx + r10*8 + 64], ymm1
	vmovapd	[rdx + r10*8 + 96], ymm1
	add 	r10, 32
	jmp	cicloi
finecicloi:
	xor	r10, r10
	mov	rbx, rdi
	imul	rbx, 8		;n*8
	add	rdx, rdi	;Pi+n*8
	vxorpd	xmm1, xmm1
cicloo:
	cmp	rdi, rsi
	jge	finecicloo
	vmovsd	[rdx+r10*8], xmm1	;Pi[n*8+i*8] = 0
	inc	rdi	
	inc	r10
	jmp	cicloo
finecicloo:
	vpopay
	popaq
	mov	rsp, rbp
	pop	rbp
	ret
