
%include "sseutils.nasm"

section .data
	align	16
	n	equ	8
	align	16
	pik	equ	12
	align	16
	pic	equ	16

section	.text
	global	cvtPagerank
cvtPagerank:
	push	ebp
	mov	ebp, esp
	pushad
	mov	ebx, [ebp+pik]
	mov	ecx, [ebp+pic]
	mov	edi, [ebp+n]
	xor	esi, esi
cicloi:
	cmp	esi, edi
	jge	finecicloi
	xorps	xmm1, xmm1
	movss	xmm1, [ebx+esi*4]		;Pik[i]
	cvtss2sd	xmm1, xmm1
	movsd	[ecx+esi*8], xmm1		;Piconv[i] = (double)Pik[i]
	inc	esi
	jmp	cicloi
finecicloi:
	popad
	mov	esp, ebp
	pop	ebp
	ret
	
