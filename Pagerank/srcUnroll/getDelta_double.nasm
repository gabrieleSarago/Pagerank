
%include "sseutils.nasm"

section	.data
	pi0	equ	8
	pik	equ	12
	n	equ	16
	d	equ	20
	align	16
	m	dq	0x7fffffffffffffff, 0x7fffffffffffffff

section	.text
	global	getDelta_double
getDelta_double:
	push	ebp
	mov	ebp, esp
	pushad
	mov	ebx, [ebp+pi0]
	mov	ecx, [ebp+pik]
	mov	edi, [ebp+n]
	mov	eax, [ebp+d]
	movapd	xmm3, [m]
	xor	esi, esi
	xorpd	xmm0, xmm0
cicloi:
	cmp	esi, edi
	jge	finecicloi
	movapd	xmm1, [ebx+esi*8]
	movapd	xmm2, [ecx+esi*8]
	subpd	xmm1, xmm2
	andpd	xmm1, xmm3
	haddpd	xmm1, xmm1
	addsd	xmm0, xmm1
	movapd	[ebx+esi*8], xmm2		;Pi0[i...i+p-1] = Pik[i...i+p-1]
	movapd	xmm1, [ebx+esi*8 + 16]
	movapd	xmm2, [ecx+esi*8 + 16]
	subpd	xmm1, xmm2
	andpd	xmm1, xmm3
	haddpd	xmm1, xmm1
	addsd	xmm0, xmm1
	movapd	[ebx+esi*8 + 16], xmm2
	movapd	xmm1, [ebx+esi*8 + 32]
	movapd	xmm2, [ecx+esi*8 + 32]
	subpd	xmm1, xmm2
	andpd	xmm1, xmm3
	haddpd	xmm1, xmm1
	addsd	xmm0, xmm1
	movapd	[ebx+esi*8 + 32], xmm2
	movapd	xmm1, [ebx+esi*8 + 48]
	movapd	xmm2, [ecx+esi*8 + 48]
	subpd	xmm1, xmm2
	andpd	xmm1, xmm3
	haddpd	xmm1, xmm1
	addsd	xmm0, xmm1
	movapd	[ebx+esi*8 + 48], xmm2
	add	esi, 8
	jmp	cicloi
finecicloi:
	movsd	[eax], xmm0
	popad
	mov	esp, ebp
	pop	ebp
	ret
