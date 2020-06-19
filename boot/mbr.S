%include "boot.inc"

[BITS 16]
SECTION MBR vstart=0x7c00   

      mov sp,0x7c00
            
      mov     ax, 0600h
      mov     bx, 0700h
      mov     cx, 0                   
      mov     dx, 184fh		   			   
      int     10h                     

      mov ax,LOADER_START_SECTOR	 ; 起始扇区lba地址
      mov bx,LOADER_BASE_ADDR       ; 写入的地址
      mov cl,0x4			 ; 待读入的扇区数
      call read_disk_lba28		 ; 以下读取程序的起始部分（一个扇区）
      
      jmp LOADER_BASE_ADDR
       
;功能:读取硬盘n个扇区 LBA28 
;相当于 void read_disk_lba28(lba_number, to, count);
read_disk_lba28:	   
      mov si,ax	      ;备份ax
      mov di,cx		;备份cx
;读写硬盘:
;第1步：设置要读取的扇区数
      mov dx,0x1f2 ;设置要读取的扇区数量。这个数值要写入 0x1f2 端口。这是个 8 位端口，因此每次只能读写 255 个扇区
      mov al,cl
      out dx,al            ;读取的扇区数

      mov ax,si	   ;恢复ax

      ;设置起始 LBA 扇区号。
      ;扇区的读写是连续的，因此只需要给出第一个扇区的编号就可以了。
      ;28 位的扇区号太长，需要将其分成 4 段，
      ;分别写入端口 0x1f3、0x1f4、0x1f5 和 0x1f6 号端口。
      ;其中，
      ;0x1f3 号端口存放的是 0～7 位；
      ;0x1f4 号端口存放的是 8～15 位；
      ;0x1f5 号端口存放的是 16～23 位;
      ;最后 4 位在 0x1f6 号端口

;第2步：将LBA地址存入0x1f3 ~ 0x1f6

      ;LBA地址7~0位写入端口0x1f3
      mov dx,0x1f3                       
      out dx,al  ;输出 0～7 位                        

      ;LBA地址15~8位写入端口0x1f4
      mov cl,8
      shr ax,cl ;右移cl位 右移8位 8～15 位
      mov dx,0x1f4
      out dx,al

      ;LBA地址23~16位写入端口0x1f5
      shr ax,cl ;继续右移8位
      mov dx,0x1f5
      out dx,al

      shr ax,cl
      and al,0x0f	   ;lba第24~27位 最后4位

      or al,0xe0	   ; 设置7～4位为1110,表示lba模式
      mov dx,0x1f6

      out dx,al

;第3步：向0x1f7端口写入读命令，0x20 
      mov dx,0x1f7
      mov al,0x20                        
      out dx,al

;第4步：检测硬盘状态
  .not_ready:
      ;同一端口，写时表示写入命令字，读时表示读入硬盘状态
      nop
      in al,dx    ;端口0x1f7 既是命令端口，又是状态端口
      and al,0x88	   ;第4位为1表示硬盘控制器已准备好数据传输，第7位为1表示硬盘忙, 如果已完成 第七位为0 ax变成0000 1000
      cmp al,0x08    ;如果ax为0000 1000那么就准备好了
      jnz .not_ready	   ;若未准备好，继续等。

;第5步：从0x1f0端口读数据
      mov ax, di     ;读4个扇区
      mov dx, 256
      mul dx
      mov cx, ax	   ; di为要读取的扇区数，一个扇区有512字节，每次读入两个字节，
			   ; 共需di*512/2次，所以di*256
                     ; cx为读取次数 每次读取2 bytes
      mov dx, 0x1f0
  .go_on_read:
      in ax,dx       ; 读进ax
      mov [bx],ax    ; bx为写入地址 把读进来的内容写进[bx]
      add bx,2       ; bx往前移2字节  
      loop .go_on_read ;读取cx次数
      ret

   times 510-($-$$) db 0
   db 0x55,0xaa

