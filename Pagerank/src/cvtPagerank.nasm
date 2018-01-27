
%include "sseutils.nasm"

section .data
	align	16
	n	equ	8
	align	16
	pik	equ	12
	align	16
	pic	equ	16
section	.bss
	alignb	16
	b	resq	1
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
	movss	xmm1, [ebx+esi*4]		;Pik[i]
	cvtss2sd	xmm1, xmm1
	movss	xmm2, [ebx+esi*4+4]		;Pik[i+1]
	cvtss2sd	xmm2, xmm2
	printi	esi
	movsd	[ecx+esi*4], xmm1		;Piconv[i] = (double)Pik[i]
	movsd	[ecx+esi*4+8], xmm2		;Piconv[i+1] = (double)Pik[i+1]
	add	esi, 2
	jmp	cicloi
finecicloi:
	popad
	mov	esp, ebp
	pop	ebp
	
	
