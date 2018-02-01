
%include "sseutils.nasm"

section .data
	align	16
	pi0	equ	8
	align	16
	pik	equ	12
	align	16
	p	equ	16
	align	16
	eps	equ	20	;eps è un double, occupa 64 bit, quindi differisce di 8 byte dall'elemento successivo
	align	16
	n	equ	28
	align	16
	o	equ	32
	align	16
	pic	equ	36
	align	16
	m	dd	0x7fffffff, 0x7fffffff, 0x7fffffff, 0x7fffffff
section	.bss
	alignb	16
	b	resq	1
section .text
	global getPagerank_single
	extern getVectorPik_single
	extern getPagrnk_single

getPagerank_single:
	push	ebp
	mov	ebp, esp
	pushad
	movsd	xmm1, [ebp+eps]			;salva esp in xmm1
	mov	edi, [ebp+n]
	mov	eax, [ebp+pic]
	mov	ebx, [ebp+pi0]
	mov	ecx, [ebp+pik]
	mov	edx, [ebp+p]
	movss	xmm4, [m]
ciclow:	
	push	dword [ebp+o]
	push	edi
	push	ecx
	push	ebx
	push	edx
	call	getVectorPik_single
	add	esp, 20	
	xorps	xmm3, xmm3		;delta = 0
	xor	esi, esi
ciclo1:
	cmp	esi, edi
	jge	fineciclo1
	movss	xmm0, [ebx+esi*4]
	subss	xmm0, [ecx+esi*4]
	andps	xmm0, xmm4
	addss	xmm3, xmm0
	inc	esi
	jmp	ciclo1
;ciclo1:
;	cmp	esi, edi	
;	jge	fineciclo1
;	movaps	xmm0, [ebx+esi*4]
;	subps	xmm0, [ecx+esi*4]
;	andps	xmm0, xmm4		;|Pi0[i]-Pik[i]|...|Pi0[i+p-1]-Pik[i+p-1]|	da sommare tra di loro e salvare	
;	haddps	xmm0, xmm0
;	haddps	xmm0, xmm0
;	addss	xmm3, xmm0
;	movaps	[b], xmm3
;	printps	b, 1
;	add	esi, 4
;	jmp	ciclo1
fineciclo1:
	xor	esi, esi
ciclo2:
	cmp	esi, edi
	jge	compara
	movaps	xmm0, [ecx+esi*4]		;Pik[i...i+p-1] --> xmm0
	movaps	[ebx+esi*4], xmm0		;Pi0[i...i+p-1] = Pik[i...i+p-1]
	add	esi, 4
	jmp	ciclo2
compara:
	cvtss2sd	xmm3, xmm3			;converte delta da 32 a 64 bit floating-point
	movsd	xmm1, [ebp+eps]
	movsd	[b], xmm1
	printsd	b
	comisd	xmm3, xmm1				
	jl	fineciclow				;se delta < eps finisce
	jmp	ciclow
fineciclow:
	movsd	[b], xmm1
	printsd	b
	push	ecx
	push	edi
	call	getPagrnk_single
	add	esp, 8
	xor	esi, esi	
ciclo3:
	cmp	esi, edi
	jge	fine
	cvtss2sd	xmm0, [ecx+esi*4]		;prende Pik[i] e lo converte a 64 bit in xmm0
	cvtss2sd	xmm2, [ecx+esi*4 + 4]		;prende Pik[i+1] e lo converte a 64 bit in xmm2
	movsd	[eax+esi*4], xmm0			;salva Pik[i] a 64 in Piconv[i]
	movsd	[eax+esi*4+8], xmm2			;salva Pik[i+1] a 64 in Piconv[i+1]
	add	esi, 2
	jmp	ciclo3
fine:
	popad
	mov	esp,ebp
	pop	ebp
	ret
