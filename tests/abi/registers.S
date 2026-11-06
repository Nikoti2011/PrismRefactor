// Test-only x86-64 Darwin assembly. SysV harness, Windows callee under test.
.text
.globl _alignment
_alignment:
movq %rsp, %rax
andq $15, %rax
movq $0x1234, 8(%rsp)
movq $0x1234, 16(%rsp)
movq $0x1234, 24(%rsp)
movq $0x1234, 32(%rsp)
ret
.globl _native_alignment
_native_alignment:
movq %rsp,%rax
andq $15,%rax
ret
.globl _native_clobber
_native_clobber:
xorq %rdi,%rdi
xorq %rsi,%rsi
pxor %xmm6,%xmm6
pxor %xmm7,%xmm7
pxor %xmm8,%xmm8
pxor %xmm9,%xmm9
pxor %xmm10,%xmm10
pxor %xmm11,%xmm11
pxor %xmm12,%xmm12
pxor %xmm13,%xmm13
pxor %xmm14,%xmm14
pxor %xmm15,%xmm15
ret
.globl _bad_guest
_bad_guest:
pxor %xmm15,%xmm15
ret
.globl _preserved
_preserved:
pushq %rbx
pushq %rbp
pushq %r12
pushq %r13
pushq %r14
pushq %r15
subq $56,%rsp
movq %rdi,48(%rsp)
movq $4352,%rbx
movq $4353,%rbp
movq $4354,%rdi
movq $4355,%rsi
movq $4356,%r12
movq $4357,%r13
movq $4358,%r14
movq $4359,%r15
movl $8710,%eax
movd %eax,%xmm6
pshufd $0,%xmm6,%xmm6
movl $8711,%eax
movd %eax,%xmm7
pshufd $0,%xmm7,%xmm7
movl $8712,%eax
movd %eax,%xmm8
pshufd $0,%xmm8,%xmm8
movl $8713,%eax
movd %eax,%xmm9
pshufd $0,%xmm9,%xmm9
movl $8714,%eax
movd %eax,%xmm10
pshufd $0,%xmm10,%xmm10
movl $8715,%eax
movd %eax,%xmm11
pshufd $0,%xmm11,%xmm11
movl $8716,%eax
movd %eax,%xmm12
pshufd $0,%xmm12,%xmm12
movl $8717,%eax
movd %eax,%xmm13
pshufd $0,%xmm13,%xmm13
movl $8718,%eax
movd %eax,%xmm14
pshufd $0,%xmm14,%xmm14
movl $8719,%eax
movd %eax,%xmm15
pshufd $0,%xmm15,%xmm15
callq *48(%rsp)
cmpq $4352,%rbx
jne Lbad
cmpq $4353,%rbp
jne Lbad
cmpq $4354,%rdi
jne Lbad
cmpq $4355,%rsi
jne Lbad
cmpq $4356,%r12
jne Lbad
cmpq $4357,%r13
jne Lbad
cmpq $4358,%r14
jne Lbad
cmpq $4359,%r15
jne Lbad
movl $8710,%eax
movd %eax,%xmm0
pshufd $0,%xmm0,%xmm0
pcmpeqb %xmm6,%xmm0
pmovmskb %xmm0,%eax
cmpl $65535,%eax
jne Lbad
movl $8711,%eax
movd %eax,%xmm0
pshufd $0,%xmm0,%xmm0
pcmpeqb %xmm7,%xmm0
pmovmskb %xmm0,%eax
cmpl $65535,%eax
jne Lbad
movl $8712,%eax
movd %eax,%xmm0
pshufd $0,%xmm0,%xmm0
pcmpeqb %xmm8,%xmm0
pmovmskb %xmm0,%eax
cmpl $65535,%eax
jne Lbad
movl $8713,%eax
movd %eax,%xmm0
pshufd $0,%xmm0,%xmm0
pcmpeqb %xmm9,%xmm0
pmovmskb %xmm0,%eax
cmpl $65535,%eax
jne Lbad
movl $8714,%eax
movd %eax,%xmm0
pshufd $0,%xmm0,%xmm0
pcmpeqb %xmm10,%xmm0
pmovmskb %xmm0,%eax
cmpl $65535,%eax
jne Lbad
movl $8715,%eax
movd %eax,%xmm0
pshufd $0,%xmm0,%xmm0
pcmpeqb %xmm11,%xmm0
pmovmskb %xmm0,%eax
cmpl $65535,%eax
jne Lbad
movl $8716,%eax
movd %eax,%xmm0
pshufd $0,%xmm0,%xmm0
pcmpeqb %xmm12,%xmm0
pmovmskb %xmm0,%eax
cmpl $65535,%eax
jne Lbad
movl $8717,%eax
movd %eax,%xmm0
pshufd $0,%xmm0,%xmm0
pcmpeqb %xmm13,%xmm0
pmovmskb %xmm0,%eax
cmpl $65535,%eax
jne Lbad
movl $8718,%eax
movd %eax,%xmm0
pshufd $0,%xmm0,%xmm0
pcmpeqb %xmm14,%xmm0
pmovmskb %xmm0,%eax
cmpl $65535,%eax
jne Lbad
movl $8719,%eax
movd %eax,%xmm0
pshufd $0,%xmm0,%xmm0
pcmpeqb %xmm15,%xmm0
pmovmskb %xmm0,%eax
cmpl $65535,%eax
jne Lbad
xorl %eax,%eax
jmp Ldone
Lbad:
movl $1,%eax
Ldone:
addq $56,%rsp
popq %r15
popq %r14
popq %r13
popq %r12
popq %rbp
popq %rbx
ret
