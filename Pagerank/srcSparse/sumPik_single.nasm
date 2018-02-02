
%include "sseutils.nasm"

section	.data
	pik	equ	8
	pi0z	equ	12
	e	equ	16
	cz	equ	20

section	.text
	global sumPik_single
sumPik_single:
	push	ebp
	mov	ebp, esp
	pushad
	mov	edi, [ebp+cz]
	mov	eax, [ebp+pik]
	mov	ecx, [ebp+pi0z]
	movss	xmm2, [ebp+e]
	shufps	xmm2, xmm2, 0
	xorps	xmm0, xmm0
	xorps	xmm1, xmm1
	xor	esi, esi
cicloj:
	cmp	esi, edi
	jge	finecicloj
	movaps	xmm0, [ecx+esi*4]
	mulps	xmm0, xmm2
	haddps	xmm0, xmm0
	haddps	xmm0, xmm0
	addss	xmm1, xmm0
	movaps	xmm0, [ecx+esi*4+16]
	mulps	xmm0, xmm2
	haddps	xmm0, xmm0
	haddps	xmm0, xmm0
	addss	xmm1, xmm0
	movaps	xmm0, [ecx+esi*4+32]
	mulps	xmm0, xmm2
	haddps	xmm0, xmm0
	haddps	xmm0, xmm0
	addss	xmm1, xmm0
	movaps	xmm0, [ecx+esi*4+48]
	mulps	xmm0, xmm2
	haddps	xmm0, xmm0
	haddps	xmm0, xmm0
	addss	xmm1, xmm0
	add	esi, 16
	jmp	cicloj
finecicloj:
	movss	xmm3, [eax]
	addss	xmm3, xmm1
	movss	[eax], xmm3
	popad
	mov	esp, ebp
	pop	ebp
	ret

