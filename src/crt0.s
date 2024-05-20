global _start
extern main

section .text
_start:
	call main
    mov ebx, eax
	mov eax, 10   ; Assuming syscall exit is 10
	int 0x30