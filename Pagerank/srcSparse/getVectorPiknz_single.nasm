
%include "sseutils.nasm"

section .data
	p	equ	8
	pi0nz	equ	12
	pi0z	equ	16
	piknz	equ	20
	e	equ	24
	no	equ	28
	cnz	equ	32
	cz	equ	36
	nz	equ	40
	z	equ	44

section	.text
	global getVectorPiknz_single
getVectorPiknz_single:
	push	ebp
	mov	ebp, esp
	pushad
	mov	edx, [ebp+cnz]
	mov	eax, [ebp+pi0nz]
	mov	ebx, [ebp+piknz]
	movss	xmm2, [ebp+no]
	xorps	xmm0, xmm0
	xor	esi, esi
cicloi:
	cmp	esi, edx
	jge	fine
	mov	ecx, [ebp+nz]
	movss	xmm5, [ecx+esi*4]		;nz[i]
	xor	edi, edi
cicloj:
	cmp	edi, edx
	jge	parte2
	extractps	edx, xmm2, 0		;n+o --> edx
	imul	edx, edi			;(n+o)*j
	imul	edx, 4				;(n+o)*j*4
	mov	ecx, [ebp+p]	
	add	edx, ecx			;p + j*no*4
	extractps	ecx, xmm5, 0
	mov	edx, [edx+ecx*4]		;p[j*no*4 + nz[i]*4]
	printi	edx
	mov	eax, [eax+edi*4]		;pi0nz[j]
	imul	edx, eax
	mov	eax, [ebx+esi*4]
	;printi	eax
	add	eax, edx
	mov	[ebx+esi*4], eax		;salva pik_nz[i]
	mov	edx, [ebp+cnz]
	mov	eax, [ebp+pi0nz]	
	inc	edi
	jmp	cicloj
parte2:
	xor	edi, edi
	mov	edx, [ebp+cz]
	mov	eax, [ebp+pi0z]
	movss	xmm0, [ebp+e]
	shufps	xmm0, xmm0, 0			;duplica 1/n
	xorps	xmm3, xmm3
cicloj2:
	cmp	edi, edx
	jge	finecicloi
	movaps	xmm1, [eax+edi*4]		;Pi0_z[j...j+p-1]
	mulps	xmm1, xmm0
	haddps	xmm1, xmm1
	haddps	xmm1, xmm1
	addss	xmm3, xmm1	
	add	edi, 4
	jmp	cicloj2
finecicloi:
	movss	xmm4, [ebx+esi*4]
	addss	xmm4, xmm3
	movss	[ebx+esi*4], xmm4
	inc	esi
	jmp	cicloi
fine:
	popad
	mov	esp, ebp
	pop	ebp
	ret
