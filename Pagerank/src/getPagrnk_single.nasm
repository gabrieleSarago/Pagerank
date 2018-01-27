
%include "sseutils.nasm"

section .data
	align	16
	n	equ	8
	align	16
	pik	equ	12
	align	16
	m	dd	0x7fffffff, 0x7fffffff, 0x7fffffff, 0x7fffffff

section .text
	global	getPagrnk_single
getPagrnk_single:
	push	ebp
	mov	ebp, esp
	pushad
	mov	edi, [ebp+n]
	mov	eax, [ebp+pik]
	xorps	xmm0, xmm0		;somma = 0
	movaps	xmm2, [m]
	xor	esi, esi		;i = 0
ciclo1:
	cmp	esi, edi
	jge	fineciclo1
	movaps	xmm1, [eax+esi*4]	;salva in ebx Pik[i...i+p-1]
	andps	xmm1, xmm2
	haddps	xmm1, xmm1
	haddps	xmm1, xmm1
	addss	xmm0, xmm1		;somma += |Pik[i...i+p-1]|
	add	esi, 4
	jmp	ciclo1
fineciclo1:
	shufps	xmm0, xmm0, 0
	xor	esi, esi
ciclo2:
	cmp	esi, edi
	jge	fine
	movaps	xmm1, [eax+esi*4]	;salva in xmm0	Pik[i...i+p-1]
	divps	xmm1, xmm0
	movaps	[eax+esi*4], xmm1
	add	esi, 4
	jmp	ciclo2
fine:
	popad
	mov	esp, ebp
	pop	ebp
	ret
