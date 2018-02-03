
%include "sseutils.nasm"

section	.data
	n	equ	8
	a	equ	12
	d	equ	16
	no	equ	20
	c	equ	24
	e	equ	32
	b	equ	40

section	.text
	global get_matrix_P_double
get_matrix_P_double:
	push	ebp
	mov	ebp, esp
	pushad
	mov	edx, [ebp+n]
	mov	eax, [ebp+d]
	mov	ebx, [ebp+a]
	movsd	xmm3, [ebp+c]
	shufpd	xmm3, xmm3, 0		;duplica c
	movsd	xmm4, [ebp+e]		;salva e
	shufpd	xmm4, xmm4, 0		;duplica e
	movsd	xmm5, [ebp+b]		;1/n
	shufpd	xmm5, xmm5, 0		;duplica 1/n
	xorpd	xmm1, xmm1
	xorpd	xmm2, xmm2
	xor	esi, esi
cicloi:
	cmp	esi, edx
	jge	fine
	movsd	xmm0, [eax+esi*8]	;d[i]
	shufpd	xmm0, xmm0, 0		;duplica d[i]
	mov	ecx, [ebp+no]		;(n+o)
	imul	ecx, esi		;i*(n+o)
	imul	ecx, 8			;i*(n+o)*8
	add	ecx, ebx		;A[i*(n+o)*8]
	xor	edi, edi
cicloj:
	cmp	edi, edx
	jge	finecicloj
	comisd	xmm0, xmm1
	jne	parte2
	movapd	[ecx+edi*8], xmm5	;salva 1/n
	movapd	[ecx+edi*8 + 16], xmm5
	movapd	[ecx+edi*8 + 32], xmm5
	movapd	[ecx+edi*8 + 48], xmm5
	add	edi, 8
	jmp	cicloj
parte2:	
	movapd	xmm2, [ecx+edi*8]
	divpd	xmm2, xmm0		;A[i*(n+o)+j...j+p-1]/d[i]
	mulpd	xmm2, xmm3		;A[i*(n+o)+j...j+p-1]*c
	addpd	xmm2, xmm4		;somma e
	movapd	[ecx+edi*8], xmm2
	movapd	xmm2, [ecx+edi*8 +16]
	divpd	xmm2, xmm0		;A[i*(n+o)+j...j+p-1]/d[i]
	mulpd	xmm2, xmm3		;A[i*(n+o)+j...j+p-1]*c
	addpd	xmm2, xmm4		;somma e
	movapd	[ecx+edi*8 + 16], xmm2
	movapd	xmm2, [ecx+edi*8 + 32]
	divpd	xmm2, xmm0		;A[i*(n+o)+j...j+p-1]/d[i]
	mulpd	xmm2, xmm3		;A[i*(n+o)+j...j+p-1]*c
	addpd	xmm2, xmm4		;somma e
	movapd	[ecx+edi*8 + 32], xmm2
	movapd	xmm2, [ecx+edi*8 + 48]
	divpd	xmm2, xmm0		;A[i*(n+o)+j...j+p-1]/d[i]
	mulpd	xmm2, xmm3		;A[i*(n+o)+j...j+p-1]*c
	addpd	xmm2, xmm4		;somma e
	movapd	[ecx+edi*8 + 48], xmm2
	add	edi, 8
	jmp	cicloj
finecicloj:
	inc	esi
	jmp	cicloi
fine:
	popad
	mov	esp, ebp
	pop	ebp
	ret
