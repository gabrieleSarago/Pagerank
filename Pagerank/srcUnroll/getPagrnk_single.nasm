;float somma = 0
;for(int i = 0; i < (n+o); i+= u*p){
;	somma += fabsf(Pik[i...i+p-1]);
;	...
;	somma += fabsf(Pik[i+u*p...i+(u+1)*p-1]);
;}
;for(int i = 0; i < (n+o); i+=u*p){
;	Pi0[i...i+p-1] = Pik[i...i+p-1]/(float) somma
;	...
;	Pi0[i+u*p...i+(u+1)*p-1] = Pik[i+u*p...i+(u+1)*p-1]/(float) somma;
;}
%include "sseutils.nasm"

section .data
	n	equ	8
	pik	equ	12
	align	16
	m	dd	0x7fffffff, 0x7fffffff, 0x7fffffff, 0x7fffffff

section .text
	global	getPagrnk_single
getPagrnk_single:
	push	ebp
	mov	ebp, esp
	pushad
	mov	edi, [ebp+n]
	mov	eax, [ebp+pik]
	xorps	xmm0, xmm0		;somma = 0
	movaps	xmm2, [m]
	xor	esi, esi		;i = 0
ciclo1:
	cmp	esi, edi
	jge	fineciclo1
	movaps	xmm1, [eax+esi*4]	;salva in ebx Pik[i...i+p-1]
	andps	xmm1, xmm2
	haddps	xmm1, xmm1
	haddps	xmm1, xmm1
	addss	xmm0, xmm1		;somma += |Pik[i...i+p-1]|
	movaps	xmm1, [eax+esi*4 + 16]	;salva in ebx Pik[i...i+p-1]
	andps	xmm1, xmm2
	haddps	xmm1, xmm1
	haddps	xmm1, xmm1
	addss	xmm0, xmm1		;somma += |Pik[i...i+p-1]|
	movaps	xmm1, [eax+esi*4 + 32]	;salva in ebx Pik[i...i+p-1]
	andps	xmm1, xmm2
	haddps	xmm1, xmm1
	haddps	xmm1, xmm1
	addss	xmm0, xmm1		;somma += |Pik[i...i+p-1]|
	movaps	xmm1, [eax+esi*4 + 48]	;salva in ebx Pik[i...i+p-1]
	andps	xmm1, xmm2
	haddps	xmm1, xmm1
	haddps	xmm1, xmm1
	addss	xmm0, xmm1		;somma += |Pik[i...i+p-1]|
	add	esi, 16
	jmp	ciclo1
fineciclo1:
	shufps	xmm0, xmm0, 0		;duplica la somma
	xor	esi, esi
ciclo2:
	cmp	esi, edi
	jge	fine
	movaps	xmm1, [eax+esi*4]	;salva in xmm0	Pik[i...i+p-1]
	divps	xmm1, xmm0
	movaps	[eax+esi*4], xmm1
	movaps	xmm1, [eax+esi*4 + 16]	;salva in xmm0	Pik[i...i+p-1]
	divps	xmm1, xmm0
	movaps	[eax+esi*4 + 16], xmm1
	movaps	xmm1, [eax+esi*4 + 32]	;salva in xmm0	Pik[i...i+p-1]
	divps	xmm1, xmm0
	movaps	[eax+esi*4 + 32], xmm1
	movaps	xmm1, [eax+esi*4 + 48]	;salva in xmm0	Pik[i...i+p-1]
	divps	xmm1, xmm0
	movaps	[eax+esi*4 + 48], xmm1
	add	esi, 16
	jmp	ciclo2
fine:
	popad
	mov	esp, ebp
	pop	ebp
	ret
