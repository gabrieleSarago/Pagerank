
%include "sseutils.nasm"

section	.data
	align	16
	pi0	equ	8
	align	16
	pik	equ	12
	align	16
	n	equ	16
	align	16
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
	xor	esi, esi
	xorpd	xmm0, xmm0
cicloi:
	cmp	esi, edi
	jge	finecicloi
	movapd	xmm1, [ebx+esi*8]
	movapd	xmm2, [ecx+esi*8]
	subpd	xmm1, xmm2
	andpd	xmm1, [m]
	haddpd	xmm1, xmm1
	addsd	xmm0, xmm1
	movapd	[ebx+esi*8], xmm2		;Pi0[i...i+p-1] = Pik[i...i+p-1]
	add	esi, 2
	jmp	cicloi
finecicloi:
	movsd	[eax], xmm0
	popad
	mov	esp, ebp
	pop	ebp