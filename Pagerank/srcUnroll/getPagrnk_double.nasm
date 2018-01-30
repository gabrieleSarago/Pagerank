
%include "sseutils.nasm"

section .data
	align	16
	n	equ	8
	align	16
	pik	equ	12
	align	16
	m	dq	0x7fffffffffffffff, 0x7fffffffffffffff

section .text
	global	getPagrnk_double
getPagrnk_double:
	push	ebp
	mov	ebp, esp
	pushad
	mov	edi, [ebp+n]
	mov	eax, [ebp+pik]
	xorpd	xmm0, xmm0		;somma = 0
	movapd	xmm2, [m]
	xor	esi, esi		;i = 0
ciclo1:
	cmp	esi, edi
	jge	fineciclo1
	movapd	xmm1, [eax+esi*8]	;salva in ebx Pik[i...i+p-1]
	andpd	xmm1, xmm2
	haddpd	xmm1, xmm1
	addsd	xmm0, xmm1		;somma += |Pik[i...i+p-1]|
	add	esi, 2
	jmp	ciclo1
fineciclo1:
	shufpd	xmm0, xmm0, 0
	xor	esi, esi
ciclo2:
	cmp	esi, edi
	jge	fine
	movapd	xmm1, [eax+esi*8]	;salva in xmm0	Pik[i...i+p-1]
	divpd	xmm1, xmm0
	movapd	[eax+esi*8], xmm1
	add	esi, 2
	jmp	ciclo2
fine:
	popad
	mov	esp, ebp
	pop	ebp
	ret
