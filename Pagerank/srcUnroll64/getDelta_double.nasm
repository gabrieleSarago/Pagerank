
%include "sseutils64.nasm"

section	.data
	align	32
	m	dq	0x7fffffffffffffff, 0x7fffffffffffffff, 0x7fffffffffffffff, 0x7fffffffffffffff

section	.text
	global	getDelta_double
getDelta_double:
	push	rbp
	mov	rbp, rsp
	pushaq
	vpushay
	;mov	rdi, [ebp+pi0]
	;mov	rsi, [ebp+pik]
	;mov	rdx, [ebp+n]
	;mov	rcx, [ebp+d]
	vmovapd	ymm4, [m]
	xor	r10, r10
	vxorpd	xmm0, xmm0
cicloi:
	cmp	r10, rdx
	jge	finecicloi
	vmovapd	ymm1, [rdi+r10*8]
	vmovapd	ymm2, [rsi+r10*8]
	vsubpd	ymm1, ymm2
	vandpd	ymm1, ymm4
	vhaddpd	ymm1, ymm1
	vperm2f128	ymm3, ymm1, ymm1, 1
	vaddsd	xmm1, xmm3
	vaddsd	xmm0, xmm1
	vmovapd	[rdi+r10*8], ymm2		;Pi0[i...i+p-1] = Pik[i...i+p-1]
	vmovapd	ymm1, [rdi+r10*8 + 32]
	vmovapd	ymm2, [rsi+r10*8 + 32]
	vsubpd	ymm1, ymm2
	vandpd	ymm1, ymm4
	vhaddpd	ymm1, ymm1
	vperm2f128	ymm3, ymm1, ymm1, 1
	vaddsd	xmm1, xmm3
	vaddsd	xmm0, xmm1
	vmovapd	[rdi+r10*8 + 32], ymm2		;Pi0[i...i+p-1] = Pik[i...i+p-1]
	vmovapd	ymm1, [rdi+r10*8 + 64]
	vmovapd	ymm2, [rsi+r10*8 + 64]
	vsubpd	ymm1, ymm2
	vandpd	ymm1, ymm4
	vhaddpd	ymm1, ymm1
	vperm2f128	ymm3, ymm1, ymm1, 1
	vaddsd	xmm1, xmm3
	vaddsd	xmm0, xmm1
	vmovapd	[rdi+r10*8 + 64], ymm2		;Pi0[i...i+p-1] = Pik[i...i+p-1]
	vmovapd	ymm1, [rdi+r10*8 + 96]
	vmovapd	ymm2, [rsi+r10*8 + 96]
	vsubpd	ymm1, ymm2
	vandpd	ymm1, ymm4
	vhaddpd	ymm1, ymm1
	vperm2f128	ymm3, ymm1, ymm1, 1
	vaddsd	xmm1, xmm3
	vaddsd	xmm0, xmm1
	vmovapd	[rdi+r10*8 + 96], ymm2		;Pi0[i...i+p-1] = Pik[i...i+p-1]
	add	r10, 16
	jmp	cicloi
finecicloi:
	vmovsd	[rcx], xmm0
	vpopay	
	popaq
	mov	rsp, rbp
	pop	rbp
	ret
