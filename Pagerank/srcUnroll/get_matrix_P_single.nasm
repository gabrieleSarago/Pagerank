
%include "sseutils.nasm"

section	.data
	n	equ	8
	a	equ	12
	d	equ	16
	no	equ	20
	c	equ	24
	e	equ	32
	b	equ	36

section	.text
	global get_matrix_P_single
get_matrix_P_single:
	push	ebp
	mov	ebp, esp
	pushad
	mov	edx, [ebp+n]
	mov	eax, [ebp+d]
	mov	ebx, [ebp+a]
	movsd	xmm3, [ebp+c]
	cvtsd2ss	xmm3, xmm3	;converte in float
	shufps	xmm3, xmm3, 0		;duplica c
	movss	xmm4, [ebp+e]		;salva e
	shufps	xmm4, xmm4, 0		;duplica e
	movss	xmm5, [ebp+b]		;1/n
	shufps	xmm5, xmm5, 0		;duplica 1/n
	xorps	xmm1, xmm1
	xorps	xmm2, xmm2
	xor	esi, esi
cicloi:
	cmp	esi, edx
	jge	fine
	movss	xmm0, [eax+esi*4]	;d[i]
	shufps	xmm0, xmm0, 0		;duplica d[i]
	mov	ecx, [ebp+no]		;(n+o)
	imul	ecx, esi		;i*(n+o)
	imul	ecx, 4			;i*(n+o)*4
	add	ecx, ebx		;A[i*(n+o)*4]
	xor	edi, edi
cicloj:
	cmp	edi, edx
	jge	finecicloj
	comiss	xmm0, xmm1
	jne	parte2
	movaps	[ecx+edi*4], xmm5	;salva 1/n
	movaps	[ecx+edi*4 + 16], xmm5
	movaps	[ecx+edi*4 + 32], xmm5
	movaps	[ecx+edi*4 + 48], xmm5	
	add	edi, 16
	jmp	cicloj
parte2:	
	movaps	xmm2, [ecx+edi*4]
	divps	xmm2, xmm0		;A[i*(n+o)+j...j+p-1]/d[i]
	mulps	xmm2, xmm3		;A[i*(n+o)+j...j+p-1]*c
	addps	xmm2, xmm4		;somma e
	movaps	[ecx+edi*4], xmm2
	movaps	xmm2, [ecx+edi*4 + 16]
	divps	xmm2, xmm0		;A[i*(n+o)+j...j+p-1]/d[i]
	mulps	xmm2, xmm3		;A[i*(n+o)+j...j+p-1]*c
	addps	xmm2, xmm4		;somma e
	movaps	[ecx+edi*4 + 16], xmm2
	movaps	xmm2, [ecx+edi*4 + 32]
	divps	xmm2, xmm0		;A[i*(n+o)+j...j+p-1]/d[i]
	mulps	xmm2, xmm3		;A[i*(n+o)+j...j+p-1]*c
	addps	xmm2, xmm4		;somma e
	movaps	[ecx+edi*4 + 32], xmm2
	movaps	xmm2, [ecx+edi*4 + 48]
	divps	xmm2, xmm0		;A[i*(n+o)+j...j+p-1]/d[i]
	mulps	xmm2, xmm3		;A[i*(n+o)+j...j+p-1]*c
	addps	xmm2, xmm4		;somma e
	movaps	[ecx+edi*4 + 48], xmm2
	add	edi, 16
	jmp	cicloj
finecicloj:
	inc	esi
	jmp	cicloi
fine:
	popad
	mov	esp, ebp
	pop	ebp
	ret
