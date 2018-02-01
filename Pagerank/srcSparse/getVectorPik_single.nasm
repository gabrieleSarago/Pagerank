
%include "sseutils.nasm"

section	.data
	align	16
	p	equ	8
	align	16
	pi0	equ	12
	align	16
	pik	equ	16
	align	16
	n	equ	20
	align	16
	o	equ	24

section	.text
	global	getVectorPik_single

getVectorPik_single:
	push	ebp
	mov	ebp, esp
	pushad
	mov	eax, [ebp+p]
	mov	ebx, [ebp+pi0]
	mov	ecx, [ebp+pik]
	movss	xmm2, [ebp+n]		;n
	addss	xmm2, [ebp+o]		;n+o --> xmm2
	mov	edx, [ebp+n]
	xor	esi, esi		;i = 0
ciclou:
	cmp	esi, edx		;compara con n tanto arriva lo stesso a n+o
	jge	fine
	xorps	xmm0, xmm0		;0->xmm0
	xorps	xmm1, xmm1
	xorps	xmm4, xmm4
	xorps	xmm5, xmm5
	xorps	xmm6, xmm6
	movaps	[ecx+esi*4], xmm0	;azzera Pik[i...i+p-1]
	movaps	[ecx+esi*4+16], xmm0	
	movaps	[ecx+esi*4+32], xmm0	
	movaps	[ecx+esi*4+48], xmm0	
	xor	edi, edi
cicloj:
	cmp	edi, edx
	jge	finecicloj
	movss	xmm3, [ebx+edi*4]	;Pi0[j] nei primi 32 bit di xmm3
	shufps	xmm3, xmm3, 0		;duplica Pi0[j] in xmm3
	extractps	ecx, xmm2, 0		;n+o
	imul	ecx, edi		;j*(n+o)
	imul	ecx, 4			;j*(n+o)*4
	add	ecx, eax		;p+j*(n+o)*4
	movaps	xmm0, xmm3
	mulps	xmm0, [ecx+esi*4]	;p+j*(n+o)*4+(i...i+p-1)*4 --> xmm0	
	addps	xmm1, xmm0
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
