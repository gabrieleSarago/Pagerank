
%include "sseutils64.nasm"

section	.text
	global	getVectorPiIn_single
getVectorPiIn_single:
	push	rbp
	mov	rbp, rsp
	pushaq
	vpushay
	;mov	rdi, [ebp+n]
	;mov	rdx, [ebp+pi]
	;mov	rsi, [ebp+no]
	;movss	xmm0, [ebp+e]		in xmm0 è presente 1/n
	vxorps	xmm1, xmm1
	vmovss	xmm1, xmm0
	vpermilps	xmm1, xmm1, 0			;1/n duplicato 4 volte in xmm1
	vperm2f128	ymm1, ymm1, ymm1, 0	;1/n duplicato 8 volte in ymm1
	xor	r10, r10
cicloi:
	cmp	r10, rdi
	jge	finecicloi
	vmovaps	[rdx + r10*4], ymm1
	vmovaps	[rdx + r10*4 + 32], ymm1
	vmovaps	[rdx + r10*4 + 64], ymm1
	vmovaps	[rdx + r10*4 + 96], ymm1
	add 	r10, 32
	jmp	cicloi
finecicloi:
	xor	r10, r10
	mov	rbx, rdi
	imul	rbx, 4		;n*4
	add	rdx, rbx	;Pi+n*4
	vxorps	xmm1, xmm1
cicloo:
	cmp	rdi, rsi
	jge	finecicloo
	vmovss	[rdx+r10*4], xmm1
	inc	r10
	inc	rdi
	jmp	cicloo
finecicloo:
	vpopay
	popaq
	mov	rsp, rbp
	pop	rbp
	ret
