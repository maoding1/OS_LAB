org 0100h

LABEL_START:
	mov	ax, cs
	mov	ds, ax
	mov	es, ax
	Call	DispStr		; 调用显示字符串例程
	jmp	$			; 无限循环

DispStr:
	mov	ax, 0B800h     	; 将显存地址0B800h读入AX
	mov	es, ax        		; 将AX赋给段地址寄存器ES
	mov	bp, ((7*80)+16)*2      ; (16 * 80 + 2)的偏移量
	mov	cx, 14         	; CX = 串长度
	mov	si, LoaderMessage 	; SI = 串地址

next_char:
	lodsb             		; 加载一个字符到AL中，并将SI自动递增1
	mov	ah, 04Fh       	; 设置字符属性，背景色为4即红色，前景色为F即白色
	mov	[es:bp], ax   		; 将字符和属性写入显存
	add	bp, 2          	; 每个字符占2字节
	loop	next_char    		; 循环继续加载和显示字符

	ret

LoaderMessage:		db	"Hello, Loader!"  ; 输出的字符串。

