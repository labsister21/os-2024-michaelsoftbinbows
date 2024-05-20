global loader                        ; the entry symbol for ELF
global load_gdt                      ; load GDT table
global set_tss_register              ; set tss register to GDT entry
extern kernel_setup                  ; kernel C entrypoint
extern _paging_kernel_page_directory ; kernel page directory

KERNEL_VIRTUAL_BASE equ 0xC0000000    ; kernel virtual memory
KERNEL_STACK_SIZE   equ 2097152       ; size of stack in bytes
MAGIC_NUMBER        equ 0x1BADB002    ; define the magic number constant
FLAGS               equ 0x0           ; multiboot flags
CHECKSUM            equ -MAGIC_NUMBER ; calculate the checksum (magic number + checksum + flags == 0)


section .bss
align 4                    ; align at 4 bytes
kernel_stack:              ; label points to beginning of memory
    resb KERNEL_STACK_SIZE ; reserve stack for the kernel


section .multiboot  ; GRUB multiboot header
align 4             ; the code must be 4 byte aligned
    dd MAGIC_NUMBER ; write the magic number to the machine code,
    dd FLAGS        ; the flags,
    dd CHECKSUM     ; and the checksum


; start of the text (code) section
section .setup.text 
loader equ (loader_entrypoint - KERNEL_VIRTUAL_BASE)
loader_entrypoint:         ; the loader label (defined as entry point in linker script)
    ; Set CR3 (CPU page register)
    mov eax, _paging_kernel_page_directory - KERNEL_VIRTUAL_BASE
    mov cr3, eax

    ; Use 4 MB paging
    mov eax, cr4
    or  eax, 0x00000010    ; PSE (4 MB paging)
    mov cr4, eax

    ; Enable paging
    mov eax, cr0
    or  eax, 0x80000000    ; PG flag
    mov cr0, eax

    ; Jump into higher half first, cannot use C because call stack is still not working
    lea eax, [loader_virtual]
    jmp eax

loader_virtual:
    mov dword [_paging_kernel_page_directory], 0
    invlpg [0]                                ; Delete identity mapping and invalidate TLB cache for first page
    mov esp, kernel_stack + KERNEL_STACK_SIZE ; Setup stack register to proper location
    call kernel_setup
.loop:
    jmp .loop                                 ; loop forever


section .text
; More details: https://en.wikibooks.org/wiki/X86_Assembly/Protected_Mode
global kernel_execute_user_program ; execute initial user program from kernel
kernel_execute_user_program:
    mov  eax, 0x20 | 0x3
    mov  ds, ax
    mov  es, ax
    mov  fs, ax
    mov  gs, ax
    
    ; Using iret (return instruction for interrupt) technique for privilege change
    ; Stack values will be loaded into these register:
    ; [esp] -> eip, [esp+4] -> cs, [esp+8] -> eflags, [] -> user esp, [] -> user ss
    mov  ecx, [esp+4] ; Save first (before pushing anything to stack) for last push
    push eax ; Stack segment selector (GDT_USER_DATA_SELECTOR), user privilege
    mov  eax, ecx
    add  eax, 0x400000 - 4
    push eax ; User space stack pointer (esp), move it into last 4 MiB
    pushf    ; eflags register state, when jump inside user program
    mov  eax, 0x18 | 0x3
    push eax ; Code segment selector (GDT_USER_CODE_SELECTOR), user privilege
    mov  eax, ecx
    push eax ; eip register to jump back

    iret
load_gdt:
    cli
    mov  eax, [esp+4]
    lgdt [eax] ; Load GDT from GDTDescriptor, eax at this line will point GDTR location
    
    ; Set bit-0 (Protection Enable bit-flag) in Control Register 0 (CR0)
    ; This is optional, as usually GRUB already start with protected mode flag enabled
    mov  eax, cr0
    or   eax, 1
    mov  cr0, eax

    ; Far jump to update cs register
    ; Warning: Invalid GDT will raise exception in any instruction below
    jmp 0x8:flush_cs
flush_cs:
    ; Update all segment register
    mov ax, 10h
    mov ss, ax
    mov ds, ax
    mov es, ax
    ret


set_tss_register:
    mov ax, 0x28 | 0 ; GDT TSS Selector, ring 0
    ltr ax
    ret

global process_context_switch
; Load struct Context (CPU GP-register) then jump
; Function Signature: void process_context_switch(struct Context ctx);
process_context_switch:
    mov gs, word [esp+36]
    mov fs, word [esp+40]
    mov es, word [esp+44]
    mov ds, word [esp+48]

    ; Using iret (return instruction for interrupt) technique for privilege change
    ; Stack values will be loaded into these register:
    ; [esp] -> eip, [esp+4] -> cs?, [esp+8] -> eflags, [] -> process esp, [] -> process ss?
    lea  ecx, [esp+0x04] ; Save the base address for struct Context ctx
    mov  eax, 0x20 | 0x3 ; some requirement for stack manipulation (why 23?)
    push eax ; Stack segment selector (GDT_USER_DATA_SELECTOR), user privilege
    mov  eax, [ecx+12] ; esp
    ; sub  eax, 0x04
    push eax ; User space stack pointer (esp), move it into last 4 MiB
    mov  eax, [ecx+52]
    push eax ; eflags register state, when jump inside user program
    mov  eax, 0x18 | 0x3 ; not sure what to put here
    push eax ; Code segment selector (GDT_USER_CODE_SELECTOR), user privilege
    mov  eax, [ecx+48]
    push eax ; eip register to jump back

    ; [ecx+12] ; esp
    ; [ecx+52] ; eflags
    ; [ecx+48] ; eip

    ; Load register from ctx
    mov edi, [ecx+0] ; edi
    mov esi, [ecx+4] ; esi
    mov ebp, [ecx+8] ; ebp
    mov ebx, [ecx+16] ; ebx
    mov edx, [ecx+20] ; edx
    mov ecx, [ecx+24] ; ecx
    mov eax, [ecx+28] ; eax

    iret