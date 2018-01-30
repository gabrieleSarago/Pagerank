
%include "sseutils.nasm"

section .data
	align	16
	n	equ	8
	align	16
	e	equ	12
	align	16
	o	equ	20
	align	16
	pi	equ	24
section	.text
	global	getVectorPiIn_double
getVectorPiIn_double:
	push	ebp
	mov	ebp, esp
	pushad
	mov	edi, [ebp+n]
	mov	ecx, [ebp+pi]
	mov	eax, [ebp+o]
	movsd	xmm1, [ebp+e]
	shufpd	xmm1, xmm1, 0
	xor	esi, esi
cicloi:
	cmp	esi, edi
	jge	finecicloi
	movapd	[ecx + esi*8], xmm1
	movapd	[ecx + esi*8 + 16], xmm1
	movapd	[ecx + esi*8 + 32], xmm1
	movapd	[ecx + esi*8 + 48], xmm1
	add 	esi, 8
	jmp	cicloi
finecicloi:
	xor	esi, esi
	imul	edi, 8		;n*8
	add	ecx, edi	;Pi+n*8
	xorps	xmm0, xmm0
cicloo:
	cmp	esi, eax
	jge	finecicloo
	movss	[ecx+esi*8], xmm0	;Pi[n*8+i*8] = 0
	inc	esi
	jmp	cicloo
finecicloo:
	popad
	mov	esp, ebp
	pop	ebp
	ret
