
%include "sseutils64.nasm"

section	.data
	align	32
	abs m	dd	0x7fffffff, 0x7fffffff, 0x7fffffff, 0x7fffffff, 0x7fffffff, 0x7fffffff, 0x7fffffff, 0x7fffffff
section	.text
	global	getDelta_single
getDelta_single:
	push	rbp
	mov	rbp, rsp
	pushaq
	vpushay
	;mov	rdi, [ebp+pi0]
	;mov	rsi, [ebp+pik]
	;mov	rdx, [ebp+n]
	;mov	rcx, [ebp+d]
	xor	r10, r10
	vxorps	xmm0, xmm0
cicloi:
	cmp	r10, rdx
	jge	finecicloi
	vmovaps	ymm1, [rdi+r10*4]
	vmovaps	ymm2, [rsi+r10*4]
	vsubps	ymm1, ymm2
	vandps	ymm1, [m]
	vhaddps	ymm1, ymm1
	vhaddps	ymm1, ymm1
	vperm2f128	ymm3, ymm1, ymm1, 1
	vaddss	xmm1, xmm3
	vaddss	xmm0, xmm1
	vmovaps	[rdi+r10*4], ymm2		;Pi0[i...i+p-1] = Pik[i...i+p-1]
	add	r10, 8
	jmp	cicloi
finecicloi:
	movss	[rcx], xmm0
	vpopay	
	popaq
	mov	rsp, rbp
	pop	rbp
	ret
