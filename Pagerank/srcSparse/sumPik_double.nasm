
%include "sseutils.nasm"

section	.data
	pik	equ	8
	pi0z	equ	12
	e	equ	16
	cz	equ	24

section	.text
	global sumPik_double
sumPik_double:
	push	ebp
	mov	ebp, esp
	pushad
	mov	edi, [ebp+cz]
	mov	eax, [ebp+pik]
	mov	ecx, [ebp+pi0z]
	movsd	xmm2, [ebp+e]
	shufpd	xmm2, xmm2, 0
	xorpd	xmm0, xmm0
	xorpd	xmm1, xmm1
	xor	esi, esi
cicloj:
	cmp	esi, edi
	jge	finecicloj
	movapd	xmm0, [ecx+esi*8]		;Pi0_z[j, j+1]
	mulpd	xmm0, xmm2
	haddpd	xmm0, xmm0
	addsd	xmm1, xmm0
	movapd	xmm0, [ecx+esi*8+16]		;Pi0_z[j, j+1]
	mulpd	xmm0, xmm2
	haddpd	xmm0, xmm0
	addsd	xmm1, xmm0
	movapd	xmm0, [ecx+esi*8+32]		;Pi0_z[j, j+1]
	mulpd	xmm0, xmm2
	haddpd	xmm0, xmm0
	addsd	xmm1, xmm0
	movapd	xmm0, [ecx+esi*8+48]		;Pi0_z[j, j+1]
	mulpd	xmm0, xmm2
	haddpd	xmm0, xmm0
	addsd	xmm1, xmm0
	add	esi, 8
	jmp	cicloj
finecicloj:
	movsd	xmm3, [eax]
	addsd	xmm3, xmm1
	movsd	[eax], xmm3
	popad
	mov	esp, ebp
	pop	ebp
	ret
