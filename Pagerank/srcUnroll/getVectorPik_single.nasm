;for(int i = 0; i < (n+o); i+= u*p){	u*p = 4*4 = 16
;	Pik[i...i+p-1] = 0;
;	...
;	Pik[i...i+p-1] = 0;
;	for(int j = 0; j < n; j++){
;		Pik[i...i+p-1] += P[j*(n+o)+i...i+p-1]*Pi0[j];
;		...
;		Pik[i+u*p...i+(u+1)*p-1] += P[j*(n+o)+i+u*p...i+(u+1)*p-1]*Pi0[j];
;	}

%include "sseutils.nasm"

section	.data
	p	equ	8
	pi0	equ	12
	pik	equ	16
	n	equ	20
	no	equ	24

section	.text
	global	getVectorPik_single

getVectorPik_single:
	push	ebp
	mov	ebp, esp
	pushad
	mov	eax, [ebp+p]
	mov	ebx, [ebp+pi0]
	mov	ecx, [ebp+pik]
	movss	xmm2, [ebp+no]		;n+o
	mov	edx, [ebp+n]
	xor	esi, esi		;i = 0
ciclou:
	cmp	esi, edx		;compara con n, anche se in realtà esi arriva a n+o con l'ultima iterazione
	jge	fine
	xorps	xmm0, xmm0		;0->xmm0
	xorps	xmm1, xmm1
	xorps	xmm4, xmm4
	xorps	xmm5, xmm5
	xorps	xmm6, xmm6	
	xor	edi, edi
cicloj:
	cmp	edi, edx
	jge	finecicloj
	movss	xmm3, [ebx+edi*4]	;Pi0[j] nei primi 32 bit di xmm3
	shufps	xmm3, xmm3, 0		;duplica Pi0[j] in xmm3
	extractps	ecx, xmm2, 0		;(n+o)
	imul	ecx, edi		;j*(n+o)
	imul	ecx, 4			;j*(n+o)*4
	add	ecx, eax		;p[j*(n+o)*4]
	movaps	xmm0, xmm3
	mulps	xmm0, [ecx+esi*4]	;p[j*(n+o)*4+(i...i+p-1)*4]*Pi0[j] --> xmm0
	addps	xmm1, xmm0		;Pik[i...i+p-1]+= prodotto
	movaps	xmm0, xmm3
	mulps	xmm0, [ecx+esi*4+16]
	addps	xmm4, xmm0
	movaps	xmm0, xmm3
	mulps	xmm0, [ecx+esi*4+32]		
	addps	xmm5, xmm0
	movaps	xmm0, xmm3
	mulps	xmm0, [ecx+esi*4+48]		
	addps	xmm6, xmm0
	inc	edi
	jmp	cicloj
finecicloj:
	mov	ecx, [ebp+pik]
	movaps	[ecx+esi*4], xmm1	;salva Pik[i...i+p-1]
	movaps	[ecx+esi*4+16], xmm4
	movaps	[ecx+esi*4+32], xmm5
	movaps	[ecx+esi*4+48], xmm6
	add	esi, 16
	jmp	ciclou
fine:
	popad
	mov	esp, ebp
	pop	ebp
	ret
