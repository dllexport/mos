%idefine rip rel $

    _R15	equ	0x00
    _R14	equ	0x08
    _R13	equ	0x10
    _R12	equ	0x18
    _R11	equ	0x20
    _R10	equ	0x28
    _R9	equ	0x30
    _R8	equ	0x38
    _RBX	equ	0x40
    _RCX	equ	0x48
    _RDX	equ	0x50
    _RSI	equ	0x58
    _RDI	equ	0x60
    _RBP	equ	0x68
    _DS	equ	0x70
    _ES	equ	0x78
    _HANDLER	equ	0x80
    _RAX	equ	0x88
    _ERROR_CODE	equ	0x90
    _RIP	equ	0x98
    _CS	equ	0xa0
    _RFLAGS	equ	0xa8
    _OLDRSP	equ	0xb0
    _OLDSS	equ	0xb8

[BITS 64]      
SECTION .text
    
restore_all:

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8

    pop rbx
    pop rcx
    pop rdx
    pop rsi
    pop rdi
    pop rbp

    pop rax
    mov ds, ax
    pop rax
    mov es, ax

    pop rax ; handler
    pop rax ; original

    ;skip _ERROR_CODE
    add rsp, 0x8

    iret
    
int_ret:
	jmp	restore_all	

int_with_ec:
    cld
    ; save es
    mov rax, es
    push rax
    ; save ds
    mov rax, ds
    push rax
    ;set kernel code gdt seg
    mov rax, 0x10
    mov es, rax
    mov ds, rax

    push rbp
    push rdi
    push rsi
    push rdx
    push rcx
    push rbx

    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    mov rsi, [rsp + _ERROR_CODE]
    mov rdi, rsp
    mov rdx, [rsp + _HANDLER]
    call rdx
    jmp int_ret
loop:
    jmp loop
extern handle_devide_error
global divide_error
divide_error:
	push	0x0 ; ec
    push    rax ; original rax
	lea     rax, [rel handle_devide_error]
    push    rax ; handler
    jmp int_with_ec

extern handle_debug_error
global debug_error
debug_error:
	push	0x0 ; ec
    push    rax ; original rax
	lea     rax, [rel handle_debug_error]
    push    rax ; handler
    jmp int_with_ec

extern handle_nmi_error
global nmi_error
nmi_error:
	push	0x0 ; ec
    push    rax ; original rax
	lea     rax, [rel handle_nmi_error]
    push    rax ; handler
    jmp int_with_ec

extern handle_int3_error
global int3_error
int3_error:
	push	0x0 ; ec
    push    rax ; original rax
	lea     rax, [rel handle_int3_error]
    push    rax ; handler
    jmp int_with_ec

extern handle_tss_error
global tss_error
tss_error:
    push    rax ; original rax
	lea     rax, [rel handle_tss_error]
    push    rax ; handler
    jmp int_with_ec

extern handle_page_fault_error
global page_fault_error
page_fault_error:
    push    rax ; original rax
	lea     rax, [rel handle_page_fault_error]
    push    rax ; handler
    jmp int_with_ec


