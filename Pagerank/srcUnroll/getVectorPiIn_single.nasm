;for(int i = 0; i < (n+o); i++){
;	Pi[i...i+p-1] = {e, ..., e};
;	...
;	Pi[i+u*p...i+(u+1)*p-1] = {e, ..., e};
;}
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
	mov	ecx, [ebp+pi]		;indirizzo Pi
	mov	eax, [ebp+no]		;n+o
	movss	xmm1, [ebp+e]		;1/n
	shufps	xmm1, xmm1, 0		;duplica 1/n
	xor	esi, esi
cicloi:
	cmp	esi, edi
	jge	finecicloi
	movaps	[ecx + esi*4], xmm1	;salva 16 1/n per volta
	movaps	[ecx+esi*4+16], xmm1
	movaps	[ecx+esi*4+32], xmm1
	movaps	[ecx+esi*4+48], xmm1
	add 	esi, 16
	jmp	cicloi
finecicloi:
;siccome gli elementi di padding potrebbero essere sostituiti con 1/n
;si fa in modo che questi ritornino a 0, altrimenti potrebbero invalidare i calcoli successivi.
	xor	esi, esi
	mov	ebx, edi
	imul	ebx, 4		;n*4
	add	ecx, ebx	;Pi+n*4
	xorps	xmm0, xmm0
cicloo:
	cmp	edi, eax
	jge	finecicloo
	movss	[ecx+esi*4], xmm0
	inc	edi			;edi va da n fino a n+o
	inc	esi			;esi va da 0 a o
	jmp	cicloo
finecicloo:
	popad
	mov	esp, ebp
	pop	ebp
	ret
