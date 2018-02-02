
%include "sseutils.nasm"

section	.data
	p	equ	8
	pi0	equ	12
	pik	equ	16
	n	equ	20
	no	equ	24

section	.text
	global	getVectorPik_dense

getVectorPik_dense:
	push	ebp
	mov	ebp, esp
	pushad
	mov	eax, [ebp+p]
	mov	ebx, [ebp+pi0]
	mov	ecx, [ebp+pik]
	mov	edx, [ebp+n]
	xor	esi, esi		;i = 0
cicloi:
	cmp	esi, edx		;compara con n tanto arriva lo stesso a n+o
	jge	fine
	xorpd	xmm0, xmm0		;0->xmm0
	xorpd	xmm1, xmm1
	xorpd	xmm4, xmm4
	xorpd	xmm5, xmm5
	xorpd	xmm6, xmm6
	movapd	[ecx+esi*8], xmm0	;azzera Pik[i...i+p-1]
	movapd	[ecx+esi*8+16], xmm0
	movapd	[ecx+esi*8+32], xmm0
	movapd	[ecx+esi*8+48], xmm0
	xor	edi, edi
cicloj:
	cmp	edi, edx
	jge	finecicloj
	movsd	xmm3, [ebx+edi*8]	;Pi0[j] nei primi 32 bit di xmm3
	shufpd	xmm3, xmm3, 0		;duplica Pi0[j] in xmm3
	mov	ecx, [ebp+no]		;n+o
	imul	ecx, edi		;j*(n+o)
	imul	ecx, 8			;j*(n+o)*8
	add	ecx, eax		;p+j*(n+o)*8
	movapd	xmm0, xmm3
	mulpd	xmm0, [ecx+esi*8]	;p+j*(n+o)*8+(i...i+p-1)*8 * Pi0[j]
	addpd	xmm1, xmm0
	movapd	xmm0, xmm3
	mulpd	xmm0, [ecx+esi*8+16]	
	addpd	xmm4, xmm0
	movapd	xmm0, xmm3
	mulpd	xmm0, [ecx+esi*8+32]	
	addpd	xmm5, xmm0
	movapd	xmm0, xmm3
	mulpd	xmm0, [ecx+esi*8+48]	
	addpd	xmm6, xmm0
	inc	edi
	jmp	cicloj
finecicloj:
	mov	ecx, [ebp+pik]
	movapd	[ecx+esi*8], xmm1	;salva Pik[i...i+p-1]
	movapd	[ecx+esi*8+16], xmm4
	movapd	[ecx+esi*8+32], xmm5
	movapd	[ecx+esi*8+48], xmm6
	add	esi, 8
	jmp	cicloi
fine:
	popad
	mov	esp, ebp
	pop	ebp
	ret
