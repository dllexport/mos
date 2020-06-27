%include "boot.inc"

%define FREE_SPACE 0x9000

%define PAGE_PRESENT    (1 << 0)
%define PAGE_WRITE      (1 << 1)
%define PAGE_USER      (1 << 2)
 
%define CODE_SEG     0x0008
%define DATA_SEG     0x0010
 
SECTION loader vstart=LOADER_BASE_ADDR

   jmp loader_start

   ; total_mem_bytes用于保存内存容量,以字节为单位,此位置比较好记。
   ; 当前偏移loader.bin文件头0x3字节,loader.bin的加载地址是0x9000,
   ; 故total_mem_bytes内存中的地址是0x9003.将来在内核中咱们会引用此地址
   total_mem_bytes dq 0					 
   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

   ;人工对齐:total_mem_bytes4字节+gdt_ptr6字节+ards_buf244字节+ards_nr2,共256字节
   ards_buf times 246 db 0
   ards_nr dw 0		      ;用于记录ards结构体数量

loader_start:
   
;-------  int 15h eax = 0000E820h ,edx = 534D4150h ('SMAP') 获取内存布局  -------

   xor ebx, ebx		      ;第一次调用时，ebx值要为0
   mov edx, 0x534d4150	      ;edx只赋值一次，循环体中不会改变
   mov di, ards_buf	      ;ards结构缓冲区
.e820_mem_get_loop:	      ;循环获取每个ARDS内存范围描述结构
   mov eax, 0x0000e820	      ;执行int 0x15后,eax值变为0x534d4150,所以每次执行int前都要更新为子功能号。
   mov ecx, 20		      ;ARDS地址范围描述符结构大小是20字节
   int 0x15
   jc .e820_not_support   ;若cf位为1则有错误发生，不尝试0xe801子功能
   add di, cx		      ;使di增加20字节指向缓冲区中新的ARDS结构位置
   inc word [ards_nr]	      ;记录ARDS数量
   cmp ebx, 0		      ;若ebx为0且cf不为1,这说明ards全部返回，当前已是最后一个
   jnz .e820_mem_get_loop

;在所有ards结构中，找出(base_add_low + length_low)的最大值，即内存的容量。
   mov cx, [ards_nr]	      ;遍历每一个ARDS结构体,循环次数是ARDS的数量
   mov ebx, ards_buf 
   xor edx, edx		      ;edx为最大的内存容量,在此先清0
.find_max_mem_area:	      ;无须判断type是否为1,最大的内存块一定是可被使用
   mov eax, [ebx]	      ;base_add_low
   add eax, [ebx+8]	      ;length_low
   add ebx, 20		      ;指向缓冲区中下一个ARDS结构
   cmp edx, eax		      ;冒泡排序，找出最大,edx寄存器始终是最大的内存容量
   jge .next_ards
   mov edx, eax		      ;edx为总内存大小
.next_ards:
   loop .find_max_mem_area
   jmp .mem_get_ok

.e820_not_support:
   jmp .error_hlt

.mem_get_ok:
   mov [total_mem_bytes], edx	 ;将内存换为byte单位后存入total_mem_bytes处。


   ;-----------------  打开A20  ----------------
   in al,0x92
   or al,0000_0010B
   out 0x92,al


   mov edi, FREE_SPACE ; which is 0x9000
   ; Zero out the 16KiB buffer.
   ; Since we are doing a rep stosd, count should be bytes/4.   
   push di                           ; REP STOSD alters DI.
   mov ecx, 0x1000
   xor eax, eax
   cld
   rep stosd
   pop di                            ; Get DI back.

   ; Build the Page Map Level 4.
   ; es:di points to the Page Map Level 4 table.
   lea eax, [es:di + 0x1000]         ; Put the address of the Page Directory Pointer Table in to EAX.
   or eax, PAGE_PRESENT | PAGE_WRITE | PAGE_USER ; Or EAX with the flags - present flag, writable flag.
   mov [es:di], eax                  ; Store the value of EAX as the first PML4E.
 
   ; map 0xffff8000 to 0, 11111111 11111111 10000000 00000000 --> 0
   ; so the pml4 and pde is 111111111 111111111 (nine 1)
   ; the last one, which is 111111111
   mov [es:di + 0xff8], eax                  ; Store the value of EAX as the first PML4E.
 
   ; Build the Page Directory Pointer Table.
   lea eax, [es:di + 0x2000]         ; Put the address of the Page Directory in to EAX.
   or eax, PAGE_PRESENT | PAGE_WRITE | PAGE_USER ; Or EAX with the flags - present flag, writable flag.
   mov [es:di + 0x1000], eax         ; Store the value of EAX as the first PDPTE.

 
   ; Build the Page Directory for 0-2m
   lea eax, [es:di + 0x3000]         ; Put the address of the Page Table in to EAX.
   or eax, PAGE_PRESENT | PAGE_WRITE | PAGE_USER ; Or EAX with the flags - present flag, writeable flag.
   mov [es:di + 0x2000], eax         ; Store to value of EAX as the first PDE.

   ; for 2-4m
   lea eax, [es:di + 0x4000]         ; Put the address of the Page Table in to EAX.
   or eax, PAGE_PRESENT | PAGE_WRITE | PAGE_USER ; Or EAX with the flags - present flag, writeable flag.
   mov [es:di + 0x2008], eax         ; Store to value of EAX as the first PDE.

   ; for 4-6m
   lea eax, [es:di + 0x5000]         ; Put the address of the Page Table in to EAX.
   or eax, PAGE_PRESENT | PAGE_WRITE | PAGE_USER ; Or EAX with the flags - present flag, writeable flag.
   mov [es:di + 0x2010], eax         ; Store to value of EAX as the first PDE.

   push di                           ; Save DI for the time being.
   lea di, [di + 0x3000]             ; Point DI to the page table.
   mov eax, PAGE_PRESENT | PAGE_WRITE | PAGE_USER    ; Move the flags into EAX - and point it to 0x0000.

   ; Build the Page Table.
.build2MTable:
   mov [es:di], eax
   add eax, 0x1000
   add di, 8
   cmp eax, 0x200000                 ; If we did all 2MiB, end.
   jb .build2MTable

   mov di, FREE_SPACE + 0x4000           ; Point DI to the page table.
   mov eax, PAGE_PRESENT | PAGE_WRITE | PAGE_USER    ; Move the flags into EAX - and point it to 0x0000.
   xor eax, 0x200000
.build4MTable:
   mov [es:di], eax
   add eax, 0x1000
   add di, 8
   cmp eax, 0x400000                 ; If we did all 2MiB, end.
   jb .build4MTable

   mov di, FREE_SPACE + 0x5000           ; Point DI to the page table.
   mov eax, PAGE_PRESENT | PAGE_WRITE | PAGE_USER    ; Move the flags into EAX - and point it to 0x0000.
   xor eax, 0x400000
.build6MTable:
   mov [es:di], eax
   add eax, 0x1000
   add di, 8
   cmp eax, 0x600000                 ; If we did all 2MiB, end.
   jb .build6MTable

   pop di                            ; Restore DI.
 
   ; Disable IRQs
   mov al, 0xFF                      ; Out 0xFF to 0xA1 and 0x21 to disable all IRQs.
   out 0xA1, al
   out 0x21, al
 
   nop
   nop
 
   lidt [IDT]                        ; Load a zero length IDT so that any NMI causes a triple fault.
 
   ; Enter long mode.
   mov eax, 10100000b                ; Set the PAE and PGE bit.
   mov cr4, eax
 
   mov edx, edi                      ; Point CR3 at the PML4.
   mov cr3, edx
 
   mov ecx, 0xC0000080               ; Read from the EFER MSR. 
   rdmsr    
   
   or eax, 0x00000101                ; Set the LME bit.
   wrmsr
 
   mov ebx, cr0                      ; Activate long mode -
   or ebx,0x80000001                 ; - by enabling paging and protection simultaneously.
   mov cr0, ebx                    
 
   ;-----------------  打开SSE  ----------------
   mov eax, cr0
   and ax, 0xFFFB		;clear coprocessor emulation CR0.EM
   or ax, 0x2			;set coprocessor monitoring  CR0.MP
   mov cr0, eax
   mov eax, cr4
   or ax, 3 << 9		;set CR4.OSFXSR and CR4.OSXMMEXCPT at the same time
   mov cr4, eax

   lgdt [GDT.Pointer]                ; Load GDT.Pointer defined below.
   jmp CODE_SEG:LongMode             ; Load CS with 64 bit segment and flush the instruction cache

.error_hlt:		      ;出错则挂起
   hlt

[BITS 64]      
LongMode:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

; -------------------------   加载kernel  ----------------------
   mov rax, KERNEL_START_SECTOR        ; kernel.bin所在的扇区号
   mov rbx, KERNEL_BIN_BASE_ADDR       ; 从磁盘读出后，写入到ebx指定的地址
   mov rcx, 200			       ; 读入的扇区数

   call rd_disk_m_32

   call kernel_init
   mov rsp, KERNEL_BIN_BASE_ADDR
   mov rbp, rsp

   ; for clang KERNEL_ENTRY_POINT is cxx_global_var_init
   ; so after cxx_global_var_init is call, we call start_kernel
   mov rax, KERNEL_ENTRY_POINT
   jmp rax                 
   
kernel_init:
   xor rax, rax
   xor rbx, rbx		;ebx记录程序头表地址
   xor rcx, rcx		;cx记录程序头表中的program header数量
   xor rdx, rdx		;dx 记录program header尺寸,即e_phentsize

   mov dx, [KERNEL_BIN_BASE_ADDR + 54]	  ; 偏移文件54字节处的属性是e_phentsize,表示program header大小
   mov rbx, [KERNEL_BIN_BASE_ADDR + 32]   ; 偏移文件开始部分32字节的地方是e_phoff,表示第1 个program header在文件中的偏移量
   add rbx, KERNEL_BIN_BASE_ADDR
   mov cx, [KERNEL_BIN_BASE_ADDR + 56]    ; 偏移文件开始部分56字节的地方是e_phnum,表示有几个program header
.each_segment:
   cmp byte [ebx + 0], PT_NULL		  ; 若p_type等于 PT_NULL,说明此program header未使用。
   je .PTNULL

   ;为函数memcpy压入参数,参数是从右往左依然压入.函数原型类似于 memcpy(dst,src,size)
   push qword [rbx + 32]		  ; program header中偏移16字节的地方是p_filesz,压入函数memcpy的第三个参数:size
   mov rax, [rbx + 8]			  ; 距程序头偏移量为4字节的位置是p_offset
   add rax, KERNEL_BIN_BASE_ADDR	  ; 加上kernel.bin被加载到的物理地址,rax为该段的物理地址
   push rax				  ; 压入函数memcpy的第二个参数:源地址
   mov rax, [rbx + 16]
   push qword [rbx + 16]			  ; 压入函数memcpy的第一个参数:目的地址,偏移程序头8字节的位置是p_vaddr，这就是目的地址
   call mem_cpy				  ; 调用mem_cpy完成段复制
   add rsp, 24				  ; 清理栈中压入的三个参数
.PTNULL:
   add rbx, rdx				  ; edx为program header大小,即e_phentsize,在此ebx指向下一个program header 
   loop .each_segment
   ret

;----------  逐字节拷贝 mem_cpy(dst,src,size) ------------
;输入:栈中三个参数(dst,src,size)
;输出:无
;---------------------------------------------------------
mem_cpy:		      
   cld
   push rbp
   mov rbp, rsp
   push rcx		   ; rep指令用到了ecx，但ecx对于外层段的循环还有用，故先入栈备份
   mov rdi, [rbp + 16]	   ; dst
   mov rsi, [rbp + 24]	   ; src
   mov rcx, [rbp + 32]	   ; size
   rep movsb		   ; 逐字节拷贝

   ;恢复环境
   pop rcx		
   pop rbp
   ret

;-------------------------------------------------------------------------------
			   ;功能:读取硬盘n个扇区 LBA28
rd_disk_m_32:	   ; 基本上跟MBR的一样
;-------------------------------------------------------------------------------
							 ; eax=LBA扇区号
							 ; ebx=将数据写入的内存地址
							 ; ecx=读入的扇区数
   mov rsi,rax	   ; 备份eax
   mov rdi,rcx		   ; 备份扇区数到di
;读写硬盘:
;第1步：设置要读取的扇区数
   mov dx,0x1f2
   mov al,cl
   out dx,al            ;读取的扇区数

   mov rax,rsi	   ;恢复ax

;第2步：将LBA地址存入0x1f3 ~ 0x1f6

   ;LBA地址7~0位写入端口0x1f3
   mov dx,0x1f3                       
   out dx,al                          

   ;LBA地址15~8位写入端口0x1f4
   mov cl,8
   shr rax,cl
   mov dx,0x1f4
   out dx,al

   ;LBA地址23~16位写入端口0x1f5
   shr rax,cl
   mov dx,0x1f5
   out dx,al

   shr rax,cl
   and al,0x0f	   ;lba第24~27位
   or al,0xe0	   ; 设置7～4位为1110,表示lba模式
   mov dx,0x1f6
   out dx,al

;第3步：向0x1f7端口写入读命令，0x20 
   mov dx,0x1f7
   mov al,0x20                        
   out dx,al

;;;;;;; 至此,硬盘控制器便从指定的lba地址(eax)处,读出连续的cx个扇区,下面检查硬盘状态,不忙就能把这cx个扇区的数据读出来

;第4步：检测硬盘状态
.not_ready:		   ;测试0x1f7端口(status寄存器)的的BSY位
      ;同一端口,写时表示写入命令字,读时表示读入硬盘状态
   nop
   in al,dx
   and al,0x88	   ;第4位为1表示硬盘控制器已准备好数据传输,第7位为1表示硬盘忙
   cmp al,0x08
   jnz .not_ready	   ;若未准备好,继续等。

;第5步：从0x1f0端口读数据
   mov ax, di	   ;以下从硬盘端口读数据用insw指令更快捷,不过尽可能多的演示命令使用,
			   ;在此先用这种方法,在后面内容会用到insw和outsw等

   mov dx, 256	   ;di为要读取的扇区数,一个扇区有512字节,每次读入一个字,共需di*512/2次,所以di*256
   mul dx
   mov cx, ax	   
   mov dx, 0x1f0
  .go_on_read:
   in ax,dx		
   mov [rbx], ax
   add rbx, 2
; 由于在实模式下偏移地址为16位,所以用bx只会访问到0~FFFFh的偏移。
			  ; loader的栈指针为0x900,bx为指向的数据输出缓冲区,且为16位，
			  ; 超过0xffff后,bx部分会从0开始,所以当要读取的扇区数过大,待写入的地址超过bx的范围时，
			  ; 从硬盘上读出的数据会把0x0000~0xffff的覆盖，
			  ; 造成栈被破坏,所以ret返回时,返回地址被破坏了,已经不是之前正确的地址,
			  ; 故程序出会错,不知道会跑到哪里去。
			  ; 所以改为ebx代替bx指向缓冲区,这样生成的机器码前面会有0x66和0x67来反转。
			  ; 0X66用于反转默认的操作数大小! 0X67用于反转默认的寻址方式.
			  ; cpu处于16位模式时,会理所当然的认为操作数和寻址都是16位,处于32位模式时,
			  ; 也会认为要执行的指令是32位.
			  ; 当我们在其中任意模式下用了另外模式的寻址方式或操作数大小(姑且认为16位模式用16位字节操作数，
			  ; 32位模式下用32字节的操作数)时,编译器会在指令前帮我们加上0x66或0x67，
			  ; 临时改变当前cpu模式到另外的模式下.
			  ; 假设当前运行在16位模式,遇到0X66时,操作数大小变为32位.
			  ; 假设当前运行在32位模式,遇到0X66时,操作数大小变为16位.
			  ; 假设当前运行在16位模式,遇到0X67时,寻址方式变为32位寻址
			  ; 假设当前运行在32位模式,遇到0X67时,寻址方式变为16位寻址.
   loop .go_on_read
   ret


ALIGN 4
IDT:
    .Length       dw 0
    .Base         dd 0
GDT:
.Null:
    dq 0x0000000000000000             ; Null Descriptor - should be present.
 
.Code:
    dq 0x00209A0000000000             ; 64-bit code descriptor (exec/read).
    dq 0x0000920000000000             ; 64-bit data descriptor (read/write).
 
ALIGN 4
    dw 0                              ; Padding to make the "address of the GDT" field aligned on a 4-byte boundary
 
.Pointer:
    dw $ - GDT - 1                    ; 16-bit Size (Limit) of GDT.
    dd GDT                            ; 32-bit Base Address of GDT. (CPU will zero extend to 64-bit)


