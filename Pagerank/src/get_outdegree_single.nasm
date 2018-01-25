
%include "sseutils.nasm"

section .data
	align	16		;align serve?
	n	equ	8
	align	16
	a	equ	12
	align	16
	d	equ	16
	align	16
	o	equ	20

section	.text
	global	get_outdegree_single

get_outdegree_single:
	
	push	ebp
	mov	ebp, esp
	pushad
	mov	ecx, [ebp+a]		;indirizzo di A
	mov	ebx, [ebp+n]		;valore di n
	mov	edx, [ebp+d]
	mov	esi, 0		;i = 0
cicloi:
	cmp	esi, ebx
	jge	fine
	mov	eax, ebx		;eax <-- n
	add	eax, [ebp+o]		;(n+o)
	imul	eax, esi		;(n+o)*i
	imul	eax, 4			;(n+o)*i*4
	;imul	eax, esi, 4		;(n+o)*i*4
	add	eax, ecx		;a+(n+o)*i*4
	xorps	xmm1, xmm1		;out = 0
	mov	edi, 0			;j = 0
cicloj:
	cmp	edi, ebx
	jge	finecicloi
	movaps	xmm0, [eax + edi*4]
	haddps	xmm0, xmm0
	haddps	xmm0, xmm0
	addss	xmm1, xmm0
	add	edi, 4
	jmp	cicloj
finecicloi:
	movss	[edx+esi*4], xmm1	
	inc	esi
	jmp	cicloi
fine:
	popad
	mov	esp, ebp
	pop	ebp	
	ret
