
%include "sseutils64.nasm"

section	.text
	global	cvtPagerank
cvtPagerank:
	push	rbp
	mov	rbp, rsp
	pushaq
	vpushay
	;mov	rsi, [ebp+pik]
	;mov	rdx, [ebp+pic]
	;mov	edi rdi, [ebp+n]
	xor	r10, r10
cicloi:
	cmp	r10, rdi
	jge	finecicloi
	xorps	xmm1, xmm1
	vmovss	xmm1, [rsi+r10*4]		;Pik[i]
	vcvtss2sd	xmm1, xmm1
	vmovsd	[rdx+r10*8], xmm1		;Piconv[i] = (double)Pik[i]
	inc	r10
	jmp	cicloi
finecicloi:
	vpopay
	popaq
	mov	rsp, rbp
	pop	rbp
	ret
	
