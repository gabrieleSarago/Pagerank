
%include "sseutils64.nasm"

section	.text
	global	getVectorPik_double

getVectorPik_double:
	push	rbp
	mov	rbp, rsp
	pushaq
	vpushay
	;mov	rdi, [ebp+p]
	;mov	rsi, [ebp+pi0]
	;mov	rdx, [ebp+pik]
	;mov	rcx, [ebp+n]		;n
	mov	r12, r8			;n+o
	imul	r12, 8			;(n+o)*8
	xor	r10, r10		;i = 0
cicloi:
	cmp	r10, rcx		;compara con n tanto arriva lo stesso a n+o
	jge	fine
	vxorpd	xmm0, xmm0		;0->xmm0
	vxorpd	xmm1, xmm1
	vxorpd	xmm3, xmm3
	vxorpd	xmm4, xmm4
	vxorpd	xmm5, xmm5
	xor	r11, r11
cicloj:
	cmp	r11, rcx
	jge	finecicloi
	vmovsd	xmm0, [rsi+r11*8]		;Pi0[j] nei primi 32 bit di xmm0
	vpermilpd	xmm0, xmm0, 0		;duplica Pi0[j] in xmm0
	vperm2f128	ymm0, ymm0, ymm0, 0	
	mov	rax, r12		;(n+o)*8
	imul	rax, r11		;j*(n+o)*8
	add	rax, rdi		;p+j*(n+o)*8
	vmovapd	ymm2, ymm0
	vmulpd	ymm2, [rax+r10*8]	;p+j*(n+o)*8+(i...i+p-1)*8 * Pi0[j]
	vaddpd	ymm1, ymm2
	vmovapd	ymm2, ymm0
	vmulpd	ymm2, [rax+r10*8 + 32]	;p+j*(n+o)*8+(i...i+p-1)*8 * Pi0[j]
	vaddpd	ymm3, ymm2
	vmovapd	ymm2, ymm0
	vmulpd	ymm2, [rax+r10*8 + 64]	;p+j*(n+o)*8+(i...i+p-1)*8 * Pi0[j]
	vaddpd	ymm4, ymm2
	vmovapd	ymm2, ymm0
	vmulpd	ymm2, [rax+r10*8 + 96]	;p+j*(n+o)*8+(i...i+p-1)*8 * Pi0[j]
	vaddpd	ymm5, ymm2
	inc	r11
	jmp	cicloj
finecicloi:
	vmovapd	[rdx+r10*8], ymm1	;salva Pik[i...i+p-1]
	vmovapd	[rdx+r10*8 + 32], ymm3
	vmovapd	[rdx+r10*8 + 64], ymm4
	vmovapd	[rdx+r10*8 + 96], ymm5	
	add	r10, 16
	jmp	cicloi
fine:
	vpopay
	popaq
	mov	rsp, rbp
	pop	rbp
	ret
