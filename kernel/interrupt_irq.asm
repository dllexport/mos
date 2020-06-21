
    _ISR_CODE	equ	0x90
    _ERROR_CODE	equ	0x98
    _HANDLER	equ	0x80

[BITS 64]
%macro IRQ 2
[GLOBAL irq%1]
irq%1:
    cli
    push 0               ; push error_code (invalid)
    push %2              ; push isr
    push    rax ; original rax
	lea     rax, [rel irq_handler]
    push    rax ; handler
    jmp int_with_ec
%endmacro
     
SECTION .text
IRQ   0,    32  ; 电脑系统计时器
IRQ   1,    33  ; 键盘
IRQ   2,    34  ; 与 IRQ9 相接，MPU-401 MD 使用
IRQ   3,    35  ; 串口设备
IRQ   4,    36  ; 串口设备
IRQ   5,    37  ; 建议声卡使用
IRQ   6,    38  ; 软驱传输控制使用
IRQ   7,    39  ; 打印机传输控制使用
IRQ   8,    40  ; 即时时钟
IRQ   9,    41  ; 与 IRQ2 相接，可设定给其他硬件
IRQ  10,    42  ; 建议网卡使用
IRQ  11,    43  ; 建议 AGP 显卡使用
IRQ  12,    44  ; 接 PS/2 鼠标，也可设定给其他硬件
IRQ  13,    45  ; 协处理器使用
IRQ  14,    46  ; IDE0 传输控制使用
IRQ  15,    47  ; IDE1 传输控制使用
int_ret:

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

    ;skip _ISR_CODE and _ERROR_CODE
    add rsp, 0x10

    o64 iret

extern irq_handler

int_with_ec:
    ; save es
    mov rax, es
    push rax
    ; save ds
    mov rax, ds
    push rax
    ;set kernel code gdt seg
    mov ax, 0x10
    mov es, ax
    mov ds, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

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

    ;mov rsi, [rsp + _ERROR_CODE]
    ;mov rdi, rsp
    mov rdi, [rsp + _ISR_CODE]
    mov rsi, [rsp + _ERROR_CODE]
    mov rdx, [rsp + _HANDLER]
    call rdx
    jmp int_ret


