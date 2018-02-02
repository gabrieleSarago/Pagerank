
%include "sseutils.nasm"

section .data
	n	equ	8
	e	equ	12
	no	equ	16
	pi	equ	20
section	.text
	global	getVectorPiIn_single
getVectorPiIn_single:
	push	ebp
	mov	ebp, esp
	pushad
	mov	edi, [ebp+n]
	mov	ecx, [ebp+pi]
	mov	eax, [ebp+no]
	movss	xmm1, [ebp+e]
	shufps	xmm1, xmm1, 0
	xor	esi, esi
cicloi:
	cmp	esi, edi
	jge	finecicloi
	movaps	[ecx + esi*4], xmm1
	movaps	[ecx+esi*4+16], xmm1
	movaps	[ecx+esi*4+32], xmm1
	movaps	[ecx+esi*4+48], xmm1
	add 	esi, 16
	jmp	cicloi
finecicloi:
	xor	esi, esi
	mov	ebx, edi
	imul	edi, 4		;n*4
	add	ecx, edi	;Pi+n*4
	xorps	xmm0, xmm0
cicloo:
	cmp	ebx, eax
	jge	finecicloo
	movss	[ecx+esi*4], xmm0
	inc	ebx
	inc	esi
	jmp	cicloo
finecicloo:
	popad
	mov	esp, ebp
	pop	ebp
	ret
