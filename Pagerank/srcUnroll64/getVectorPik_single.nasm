
%include "sseutils64.nasm"

section	.text
	global	getVectorPik_single

getVectorPik_single:
	push	rbp
	mov	rbp, rsp
	pushaq
	vpushay
	;mov	rdi, [ebp+p]
	;mov	rsi, [ebp+pi0]
	;mov	rdx, [ebp+pik]
	;mov	rcx, [ebp+n]
	mov	r12, r8			;n+o
	imul	r12, 4			;(n+o)*4
	xor	r10, r10		;i = 0
cicloi:
	cmp	r10, rcx		;compara con n tanto arriva lo stesso a n+o
	jge	fine
	vxorps	xmm0, xmm0		;0->xmm0
	vxorps	xmm1, xmm1
	vxorps	xmm3, xmm3
	vxorps	xmm4, xmm4
	vxorps	xmm5, xmm5
	xor	r11, r11
cicloj:
	cmp	r11, rcx
	jge	finecicloi
	vmovss	xmm0, [rsi+r11*4]		;Pi0[j] nei primi 32 bit di xmm0
	vpermilps	xmm0, xmm0, 0		;duplica Pi0[j] in xmm0
	vperm2f128	ymm0, ymm0, ymm0, 0
	mov	rax, r12			;(n+o)*4
	imul	rax, r11			;j*(n+o)*4
	add	rax, rdi			;p+j*(n+o)*4
	vmovaps	ymm2, ymm0			;copia Pi0[j]
	vmulps	ymm2, [rax+r10*4]		;p+j*(n+o)*4 + (i...i+p-1)*4 mul Pi0[j] --> ymm2
	vaddps	ymm1, ymm2
	vmovaps	ymm2, ymm0			;copia Pi0[j]
	vmulps	ymm2, [rax+r10*4 + 32]		;p+j*(n+o)*4 + (i...i+p-1)*4 mul Pi0[j] --> ymm2
	vaddps	ymm3, ymm2
	vmovaps	ymm2, ymm0			;copia Pi0[j]
	vmulps	ymm2, [rax+r10*4 + 64]		;p+j*(n+o)*4 + (i...i+p-1)*4 mul Pi0[j] --> ymm2
	vaddps	ymm4, ymm2
	vmovaps	ymm2, ymm0			;copia Pi0[j]
	vmulps	ymm2, [rax+r10*4 + 96]		;p+j*(n+o)*4 + (i...i+p-1)*4 mul Pi0[j] --> ymm2
	vaddps	ymm5, ymm2
	inc	r11
	jmp	cicloj
finecicloi:
	vmovaps	[rdx+r10*4], ymm1		;salva Pik[i...i+p-1]
	vmovaps	[rdx+r10*4 + 32], ymm3
	vmovaps	[rdx+r10*4 + 64], ymm4
	vmovaps	[rdx+r10*4 + 96], ymm5
	add	r10, 32
	jmp	cicloi
fine:
	vpopay
	popaq
	mov	rsp, rbp
	pop	rbp
	ret
