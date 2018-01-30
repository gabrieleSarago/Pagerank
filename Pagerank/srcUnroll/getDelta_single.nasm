
%include "sseutils.nasm"

section	.data
	align	16
	pi0	equ	8
	align	16
	pik	equ	12
	align	16
	n	equ	16
	align	16
	d	equ	20
	align	16
	m	dd	0x7fffffff, 0x7fffffff, 0x7fffffff, 0x7fffffff

section	.text
	global	getDelta_single
getDelta_single:
	push	ebp
	mov	ebp, esp
	pushad
	mov	ebx, [ebp+pi0]
	mov	ecx, [ebp+pik]
	mov	edi, [ebp+n]
	mov	eax, [ebp+d]
	xor	esi, esi
	xorps	xmm0, xmm0			;delta = 0
cicloi:
	cmp	esi, edi
	jge	finecicloi
	movaps	xmm1, [ebx+esi*4]
	movaps	xmm2, [ecx+esi*4]
	subps	xmm1, xmm2
	andps	xmm1, [m]
	haddps	xmm1, xmm1
	haddps	xmm1, xmm1
	addss	xmm0, xmm1
	movaps	[ebx+esi*4], xmm2
	movaps	xmm1, [ebx+esi*4 + 16]
	movaps	xmm2, [ecx+esi*4 + 16]
	subps	xmm1, xmm2
	andps	xmm1, [m]
	haddps	xmm1, xmm1
	haddps	xmm1, xmm1
	addss	xmm0, xmm1
	movaps	[ebx+esi*4 + 16], xmm2
	movaps	xmm1, [ebx+esi*4 + 32]
	movaps	xmm2, [ecx+esi*4 + 32]
	subps	xmm1, xmm2
	andps	xmm1, [m]
	haddps	xmm1, xmm1
	haddps	xmm1, xmm1
	addss	xmm0, xmm1
	movaps	[ebx+esi*4 + 32], xmm2
	movaps	xmm1, [ebx+esi*4 + 48]
	movaps	xmm2, [ecx+esi*4 + 48]
	subps	xmm1, xmm2
	andps	xmm1, [m]
	haddps	xmm1, xmm1
	haddps	xmm1, xmm1
	addss	xmm0, xmm1
	movaps	[ebx+esi*4 + 48], xmm2		;Pi0[i...i+p-1] = Pik[i...i+p-1]
	add	esi, 16
	jmp	cicloi
finecicloi:
	movss	[eax], xmm0
	popad
	mov	esp, ebp
	pop	ebp
	ret
