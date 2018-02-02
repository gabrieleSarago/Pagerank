
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
	mov	ebx, edi
	imul	ebx, 8		;n*8
	add	ecx, ebx	;Pi+n*8
	xorps	xmm0, xmm0
cicloo:
	cmp	edi, eax
	jge	finecicloo
	movss	[ecx+esi*8], xmm0	;Pi[n*8+i*8] = 0
	inc	edi	
	inc	esi
	jmp	cicloo
finecicloo:
	popad
	mov	esp, ebp
	pop	ebp
	ret
