	.file	"coroutine.cc"
	.text
	.section	.rodata
.LC0:
	.string	"hello from world, %ld\t"
	.text
	.globl	_Z5worldm
	.type	_Z5worldm, @function
_Z5worldm:
.LFB15:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$16, %rsp
	movq	%rdi, -8(%rbp)
	movq	-8(%rbp), %rax
	movq	%rax, %rsi
	leaq	.LC0(%rip), %rdi
	movl	$0, %eax
	call	printf@PLT
	movl	$42, %eax
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE15:
	.size	_Z5worldm, .-_Z5worldm
	.globl	_Z5hellom
	.type	_Z5hellom, @function
_Z5hellom:
.LFB16:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$16, %rsp
	movq	%rdi, -8(%rbp)
	movq	-8(%rbp), %rax
	movq	%rax, %rdi
	call	_Z5worldm
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE16:
	.size	_Z5hellom, .-_Z5hellom
	.section	.rodata
.LC1:
	.string	"num = %ld, res = %ld\n"
	.text
	.globl	main
	.type	main, @function
main:
.LFB17:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	pushq	%rbx
	subq	$40, %rsp
	.cfi_offset 3, -24
	movq	$5, -48(%rbp)
	movq	$0, -40(%rbp)
	movl	$65536, %edi
	call	malloc@PLT
	movq	%rax, -32(%rbp)
	movq	-32(%rbp), %rax
	addq	$65536, %rax
	movq	%rax, -24(%rbp)
	movq	-24(%rbp), %rax
	leaq	-16(%rax), %rcx
	leaq	_Z5hellom(%rip), %rdx
	movq	-48(%rbp), %rax
	movq	%rcx, %rbx
#APP
# 34 "coroutine.cc" 1
	movq    %rcx, 0(%rbx)
	movq    %rbx, %rsp
	movq    %rax, %rdi
	call    *%rdx
	
# 0 "" 2
#NO_APP
	movq	%rax, -40(%rbp)
#APP
# 36 "coroutine.cc" 1
	movq    0(%rsp), %rcx
# 0 "" 2
#NO_APP
	movq	-40(%rbp), %rdx
	movq	-48(%rbp), %rax
	movq	%rax, %rsi
	leaq	.LC1(%rip), %rdi
	movl	$0, %eax
	call	printf@PLT
	movl	$0, %eax
	addq	$40, %rsp
	popq	%rbx
	popq	%rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE17:
	.size	main, .-main
	.ident	"GCC: (Ubuntu 7.5.0-6ubuntu2) 7.5.0"
	.section	.note.GNU-stack,"",@progbits
