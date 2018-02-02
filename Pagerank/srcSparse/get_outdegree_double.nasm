
%include "sseutils.nasm"

section .data
	n	equ	8
	cnz	equ	12
	a	equ	16
	d	equ	20
	no	equ	24

section	.text
	global	get_outdegree_double

get_outdegree_double:
	
	push	ebp
	mov	ebp, esp
	pushad
	mov	ecx, [ebp+a]		;indirizzo di A
	mov	ebx, [ebp+cnz]		;valore di n
	mov	edx, [ebp+d]
	mov	esi, 0			;i = 0
cicloi:
	cmp	esi, ebx
	jge	fine
	mov	eax, [ebp+no]		;eax <-- n+o
	imul	eax, esi		;(n+o)*i
	imul	eax, 8			;(n+o)*i*8
	add	eax, ecx		;a+(n+o)*i*8
	xorpd	xmm1, xmm1		;out = 0
	mov	ebx, [ebp+n]
	mov	edi, 0			;j = 0
ciclou:
	cmp	edi, ebx
	jge	fineciclou
	movapd	xmm0, [eax + edi*8]
	haddpd	xmm0, xmm0
	addsd	xmm1, xmm0
	movapd	xmm0, [eax + edi*8+16]
	haddpd	xmm0, xmm0
	addsd	xmm1, xmm0
	movapd	xmm0, [eax + edi*8+32]
	haddpd	xmm0, xmm0
	addsd	xmm1, xmm0
	movapd	xmm0, [eax + edi*8+48]
	haddpd	xmm0, xmm0
	addsd	xmm1, xmm0
	add	edi, 8
	jmp	ciclou
fineciclou:
	movsd	[edx+esi*8], xmm1	
	mov	ebx, [ebp+cnz]
	inc	esi
	jmp	cicloi
fine:
	popad
	mov	esp, ebp
	pop	ebp	
	ret
