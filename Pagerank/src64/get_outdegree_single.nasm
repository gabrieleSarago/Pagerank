
%include "sseutils64.nasm"

section	.text
	global	get_outdegree_single

get_outdegree_single:
	
	push	rbp
	mov	rbp, rsp
	pushaq
	vpushay
	;mov	rsi, [ebp+a]		;indirizzo di A
	;mov	rdi, [ebp+n]		;valore di n
	;mov	rdx, [ebp+d]
	mov	r11, 0		;i = 0
cicloi:
	cmp	r11, rdi
	jge	fine
	mov	rax, rdi		;eax <-- n
	add	rax, rcx		;(n+o)
	imul	rax, r11		;(n+o)*i
	imul	rax, 4			;(n+o)*i*4
	add	rax, rsi		;a+(n+o)*i*4
	vxorps	xmm1, xmm1		;out = 0
	mov	r10, 0			;j = 0
cicloj:
	cmp	r10, rdi
	jge	finecicloj
	vmovaps	ymm0, [rax + r10*4]
	vhaddps	ymm0, ymm0
	vhaddps	ymm0, ymm0
	vperm2f128	ymm2, ymm0, ymm0, 1
	vaddss	xmm0, xmm2
	vaddss	xmm1, xmm0
	add	r10, 8
	jmp	cicloj
finecicloj:
	vmovss	[rdx+r11*4], xmm1	
	inc	r11
	jmp	cicloi
fine:
	vpopay
	popaq
	mov	rsp, rbp
	pop	rbp	
	ret
