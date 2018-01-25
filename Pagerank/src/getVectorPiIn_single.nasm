
%include "sseutils.nasm"

section .data
	align	16
	n	equ	8
	align	16
	e	equ	12
	align	16
	pi	equ	16

section	.text
	global	getVectorPiIn_single
getVectorPiIn_single:
	push	ebp
	mov	ebp, esp
	pushad
	mov	edi, [ebp+n]
	mov	ecx, [ebp+pi]
	movss	xmm1, [ebp+e]
	imul	edi, 4
	xor	esi, esi
cicloi:
	cmp	esi, edi
	jge	finecicloi
	movss	xmm0, xmm1
	shufps	xmm0, xmm0, 0
	movaps	[ecx + esi], xmm0
	add 	esi, 16
	jmp	cicloi
finecicloi:
	;printps ecx, 1
	popad
	mov	esp, ebp
	pop	ebp
	ret
