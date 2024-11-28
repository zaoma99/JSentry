####################

.extern java_sot_append0 java_sot_append1

.section .text
.global _asm_java_sot_append0

_asm_java_sot_append0:
	# pushq %rbp
	# movq %rsp,%rbp

	test %r9d,%r9d
	jle _LAB_WITHOUT_ARGS
	lea 0x8(%rsp),%rax
	pushq %rax

	_LAB_READY_CALL:
	mov java_sot_append1,%rax
	# mov java_sot_append0,%rax
	callq *(%rax)

	# _LAB_AFTER_CALL:
	add $0x08,%rsp
	# leaveq
	retq

	_LAB_WITHOUT_ARGS:
	pushq $0
	jmp _LAB_READY_CALL
	#nop
