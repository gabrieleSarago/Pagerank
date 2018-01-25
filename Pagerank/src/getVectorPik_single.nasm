
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

section .bss
	alignb	16	
	s	resd	1

section	.text
	global	getVectorPik_single

getVectorPik_single:
	push	ebp
	mov	ebp, esp
	pushad
	mov	eax, [ebp+p]
	mov	ebx, [ebp+pi0]
	mov	ecx, [ebp+pik]
	mov	edx, [ebp+n]		;n
	add	edx, [ebp+o]		;n+o
	mov	[s], edx		;n+o --> s
	mov	edx, [ebp+n]
	xor	esi, esi		;i = 0
cicloi:
	cmp	esi, edx		;compara con n tanto arriva lo stesso a n+o
	jge	fine
	xorps	xmm0, xmm0		;0->xmm0
	xorps	xmm1, xmm1
	movups	[ecx+esi*4], xmm0	;azzera Pik[i...i+p-1]
	xor	edi, edi
cicloj:
	cmp	edi, edx
	jge	finecicloi
	movss	xmm0, [ebx+edi*4]	;Pi0[j] nei primi 32 bit di xmm0
	shufps	xmm0, xmm0, 0		;duplica Pi0[j] in xmm0
	mov	ecx, [s]		;n+o
	imul	ecx, edi		;j*(n+o)
	imul	ecx, 4			;j*(n+o)*4
	add	ecx, eax		;p+j*(n+o)*4
	mulps	xmm0, [ecx+esi*4]	;p+j*(n+o)*4+(i...i+p-1)*4 --> xmm0	
	addps	xmm1, xmm0
	inc	edi
	jmp	cicloj
finecicloi:
	mov	ecx, [ebp+pik]
	movups	[ecx+esi*4], xmm1	;salva Pik[i...i+p-1]
	add	esi, 4
	jmp	cicloi
fine:
	popad
	mov	esp, ebp
	pop	ebp
	ret
