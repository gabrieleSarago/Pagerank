
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
	global	getVectorPik_double

getVectorPik_double:
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
	xorpd	xmm0, xmm0		;0->xmm0
	xorpd	xmm1, xmm1
	movapd	[ecx+esi*8], xmm0	;azzera Pik[i...i+p-1]
	xor	edi, edi
cicloj:
	cmp	edi, edx
	jge	finecicloi
	movsd	xmm0, [ebx+edi*8]	;Pi0[j] nei primi 32 bit di xmm0
	shufpd	xmm0, xmm0, 0		;duplica Pi0[j] in xmm0
	mov	ecx, [s]		;n+o
	imul	ecx, edi		;j*(n+o)
	imul	ecx, 8			;j*(n+o)*8
	add	ecx, eax		;p+j*(n+o)*8
	mulpd	xmm0, [ecx+esi*8]	;p+j*(n+o)*8+(i...i+p-1)*8 * Pi0[j]
	addpd	xmm1, xmm0
	inc	edi
	jmp	cicloj
finecicloi:
	mov	ecx, [ebp+pik]
	movapd	[ecx+esi*8], xmm1	;salva Pik[i...i+p-1]
	add	esi, 2
	jmp	cicloi
fine:
	popad
	mov	esp, ebp
	pop	ebp
	ret
