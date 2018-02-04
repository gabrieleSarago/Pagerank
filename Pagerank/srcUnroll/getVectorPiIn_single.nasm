;for(int i = 0; i < (n+o); i++){
;	Pi[i...i+p-1] = {e, ..., e};
;	...
;	Pi[i+u*p...i+(u+1)*p-1] = {e, ..., e};
;}
%include "sseutils.nasm"

section .data
	n	equ	8
	e	equ	12
	pi	equ	16
section	.text
	global	getVectorPiIn_single
getVectorPiIn_single:
	push	ebp
	mov	ebp, esp
	pushad
	mov	edi, [ebp+n]
	mov	ecx, [ebp+pi]		;indirizzo Pi
	movss	xmm1, [ebp+e]		;1/n
	shufps	xmm1, xmm1, 0		;duplica 1/n
	mov	ebx, edi
	sub	ebx, 16			;penultima iterazione
	xor	esi, esi
cicloi:
	cmp	esi, ebx
	jge	cicloR
	movaps	[ecx + esi*4], xmm1	;salva 16 1/n per volta
	movaps	[ecx+esi*4+16], xmm1
	movaps	[ecx+esi*4+32], xmm1
	movaps	[ecx+esi*4+48], xmm1
	add 	esi, 16
	jmp	cicloi
;siccome gli elementi di padding potrebbero essere sostituiti con 1/n
;si sceglie di ricorrere a un ciclo resto
cicloR:
	cmp	esi, edi
	jge	fine
	movss	[ecx+esi*4], xmm1
	inc	esi
	jmp	cicloR
fine:
	popad
	mov	esp, ebp
	pop	ebp
	ret
