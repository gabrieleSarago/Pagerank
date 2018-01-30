
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
	;mov	rsi, [ebp+o]
	;movss	xmm0, [ebp+e]
	vxorps	xmm1, xmm1
	vmovss	xmm1, xmm0
	vpermilps	xmm1, xmm1, 0			;1/n duplicato 4 volte in xmm1
	vperm2f128	ymm1, ymm1, ymm1, 0	;1/n duplicato 8 volte in ymm1
	xor	r10, r10
cicloi:
	cmp	r10, rdi
	jge	finecicloi
	vmovaps	[rdx + r10*4], ymm1
	add 	r10, 8
	jmp	cicloi
finecicloi:
	xor	r10, r10
	imul	rdi, 4		;n*4
	add	rdx, rdi	;Pi+n*4
	vxorps	xmm1, xmm1
cicloo:
	cmp	r10, rsi
	jge	finecicloo
	vmovss	[rdx+r10*4], xmm1
	inc	r10
	jmp	cicloo
finecicloo:
	vpopay
	popaq
	mov	rsp, rbp
	pop	rbp
	ret
