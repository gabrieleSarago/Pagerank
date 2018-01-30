
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
	mov	r12, rcx		;n
	add	r12, r8			;n+o
	imul	r12, 8			;(n+o)*8
	xor	r10, r10		;i = 0
cicloi:
	cmp	r10, rcx		;compara con n tanto arriva lo stesso a n+o
	jge	fine
	vxorpd	xmm0, xmm0		;0->xmm0
	vxorpd	xmm1, xmm1
	vmovapd	[rdx+r10*8], ymm0	;azzera Pik[i...i+p-1]
	xor	r11, r11
cicloj:
	cmp	r11, rcx
	jge	finecicloi
	vmovsd	xmm0, [rsi+r11*8]		;Pi0[j] nei primi 32 bit di xmm0
	vpermilpd	xmm0, xmm0, 0		;duplica Pi0[j] in xmm0
	vperm2f128	ymm0, ymm0, ymm0, 0	
	mov	rax, r12		;n+o
	imul	rax, r11		;j*(n+o)*8
	add	rax, rdi		;p+j*(n+o)*8
	vmulpd	ymm0, [rax+r10*8]	;p+j*(n+o)*8+(i...i+p-1)*8 * Pi0[j]
	vaddpd	ymm1, ymm0
	inc	r11
	jmp	cicloj
finecicloi:
	vmovapd	[rdx+r10*8], ymm1	;salva Pik[i...i+p-1]
	add	r10, 4
	jmp	cicloi
fine:
	vpopay
	popaq
	mov	rsp, rbp
	pop	rbp
	ret
