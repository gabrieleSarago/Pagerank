
%include "sseutils.nasm"

section .data
	n	equ	8
	e	equ	12
	no	equ	20
	pi	equ	24
section	.text
	global	getVectorPiIn_double
getVectorPiIn_double:
	push	ebp
	mov	ebp, esp
	pushad
	mov	edi, [ebp+n]
	mov	ecx, [ebp+pi]
	mov	eax, [ebp+no]
	movsd	xmm1, [ebp+e]
	shufpd	xmm1, xmm1, 0
	mov	ebx, edi
	sub	ebx, 8			;penultima iterazione
	xor	esi, esi
cicloi:
	cmp	esi, ebx
	jge	cicloR
	movapd	[ecx + esi*8], xmm1
	movapd	[ecx + esi*8 + 16], xmm1
	movapd	[ecx + esi*8 + 32], xmm1
	movapd	[ecx + esi*8 + 48], xmm1
	add 	esi, 8
	jmp	cicloi
cicloR:
	cmp	esi, edi
	jge	fine
	movsd	[ecx+esi*4], xmm0
	inc	esi
	jmp	cicloR
fine:
	popad
	mov	esp, ebp
	pop	ebp
	ret
