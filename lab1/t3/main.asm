section .data

a times 101 db 0    ; 存放被除数
b times 101 db 0    ; 存放除数 
c times 101 dd 0
d times 101 dd 0
e times 101 dd 0
alen        dd 0
blen        dd 0
clen        dd 0
i           dd 0
j           dd 0
tmp         db 0

section .text
global _start

_start:

input_a:
call getchar        ; 调用 getchar，将返回值保存到 tmp
xor ebx, ebx
movzx ebx, byte[tmp]

cmp ebx, 0x20       ; 检查当前字符是否是空格
je input_b

mov ecx, [alen]
mov [a + ecx], ebx    ; 将tmp值移到a[len]中
inc dword [alen]
jmp input_a

input_b:
call getchar
xor ebx, ebx
movzx ebx, byte[tmp]

cmp ebx, 0x0a           ; 检查当前字符是否是'\n'
je do_div

mov ecx, [blen]
mov[b+ ecx], ebx;
inc dword [blen]
jmp input_b

do_div:
;检查除数是否为0
movzx ebx, byte[b]      ;ebx = b[0]
cmp ebx, 0x30           ; b[0]=0?
je divisor_is_zero
;检查被除数是否为0
movzx ebx, byte[a]
cmp ebx, 0x30
jne begin

mov byte[tmp], 0x30     ;print '0'
mov eax, 4
mov ebx, 1
mov ecx, tmp
mov edx, 1
int 0x80

mov byte[tmp], 0x20     ;print ' '
mov eax, 4
mov ebx, 1
mov ecx, tmp
mov edx, 1
int 0x80

mov byte[tmp], 0x30     ;print '0'
mov eax, 4
mov ebx, 1
mov ecx, tmp
mov edx, 1
int 0x80

mov byte[tmp], 0x0a     ;print '\n'
mov eax, 4
mov ebx, 1
mov ecx, tmp
mov edx, 1
int 0x80


jmp end

begin:
mov dword [i], 0        ; 将 i 的值设置为 0

loop1:
mov ebx, dword [alen]   ; 将alen的值加载到ebx寄存器
mov ecx , dword [i]     ; 将i的值加载到ecx寄存器
cmp ecx , ebx
jge loop1_end

movzx edx, byte[a+ecx]        ; 将a[i]的值存到edx寄存器中
sub edx, '0'            ; edx = a[i] - '0'
sub ebx, 1              ; ebx = alen-1
sub ebx, ecx            ; ebx = alen-i-1
mov dword [d+ebx*4], edx      ; d[alen - i - 1] = a[i] - '0';

inc dword[i]            ; i++
jmp loop1               ;

loop1_end:
mov dword[i], 0         ; 将 i 的值设置为 0

loop2:
mov ebx, dword [blen]   ; 将blen的值加载到ebx寄存器
mov ecx , dword [i]     ; 将i的值加载到ecx寄存器
cmp ecx , ebx
jge loop2_end

movzx edx, byte[b+ecx]    ; 将b[i]的值存到edx寄存器中
sub edx, '0'            ; edx = b[i] - '0'
sub ebx, 1              ; ebx = blen-1
sub ebx, ecx            ; ebx = blen-i-1
mov dword [e+ebx*4], edx      ; e[blen - i - 1] = b[i] - '0';
inc dword[i]            ; i++
jmp loop2               ;

loop2_end:

mov eax, dword [alen]   ; eax = alen
mov ebx, dword [blen]   ; ebx = blen
add eax, 1              ; eax = alen + 1
sub eax, ebx            ; eax = alen-blen+1
mov dword [clen], eax   ; clen = alen-blen+1
sub eax, 1
mov dword [i], eax      ; i= clen - 1

loop3:
mov ebx, dword[i]       ;
jl  loop3_end           ;如果 i 小于 0，则跳到 loop3_end 

loop4:
;如果equal返回false(eax =0),跳转
call Equal
cmp eax, 0
je  loop4_end

mov dword[j], 0         ;初始化j=0

loop5:
mov eax, dword[j]
mov ebx, dword[blen]
cmp eax, ebx
jge loop5_end
;d[i+j] -= e[j]
mov eax, dword[i]       ; eax = i
mov ebx, dword[j]       ; ebx = j
add eax, ebx            ; eax = i+j
mov ecx, dword [d+eax*4]; ecx = d[i+j]
mov edx, dword [e+ebx*4]; edx = e[j]
sub ecx, edx            ; ecx = d[i+j]-e[j]
mov dword[d+eax*4],ecx  ; d[i+j] = d[i+j]-e[j]
if:
cmp dword[d+eax*4],0    ; 比较d[i+j]与 0, >=0跳转
jge if_end
;d[i+j] += 10
add ecx, 10             ;
mov dword[d+eax*4],ecx  ; d[i+j] += 10
add eax, 1
dec dword[d+eax*4]      ; d[i+j+1]--

if_end:
inc dword[j]            ;j++
jmp loop5

loop5_end:
;c[i]++
mov eax, dword[i]       ;eax = i
inc dword[c+eax*4]      ;c[i]++
jmp loop4

loop4_end:
dec dword[i]            ;i--
jmp loop3


loop3_end:
; i = blen-1
mov eax, dword[blen]    ; eax = blen
;去除前导0
delete_pre_0:
sub eax, 1              ; eax--
cmp eax, 0
jl  print_0
mov ecx, dword[d+eax*4] ; ecx = b[i]
cmp ecx, 0
je delete_pre_0
jmp print_0_end

print_0:
mov byte[tmp], 0x30     ;print '0'
mov eax, 4
mov ebx, 1
mov ecx, tmp
mov edx, 1
int 0x80
jmp print1_end

print_0_end:
mov dword[i], eax       ; i = eax

print1:
cmp dword[i], 0
jl print1_end                  ; i<0时跳转

mov eax, dword[i]       ; eax = i
mov ebx, dword[d+eax*4] ; ebx = d[i]
add ebx, '0'            ; ebx = d[i]+'0'
mov byte[tmp], bl       ; tmp = d[i]+'0'
;putchar(tmp)
mov eax, 4
mov ebx, 1
mov ecx, tmp
mov edx, 1
int 0x80

mov eax, dword[i]
sub eax, 1              ; eax = i-1
mov dword[i], eax       ; i--
jmp print1

print1_end:
mov ebx, 0x20           ; ebx = ' '
mov byte[tmp], bl       ; tmp = ' '

mov eax, 4
mov ebx, 1
mov ecx, tmp
mov edx, 1              
int 0x80

; i = clen-1
mov eax, dword[clen]    ; eax = clen
sub eax, 1              ; eax = clen - 1
cmp eax, 0
jl  ans_is_zero

mov dword[i], eax       ; i = clen-1

print2:
cmp dword[i], 0
jl print2_end                  ; i<0时跳转

mov eax, dword[i]       ; eax = i
mov ebx, dword[c+eax*4] ; ebx = c[i]
add ebx, '0'            ; ebx = c[i]+'0'
mov byte[tmp], bl       ; tmp = c[i]+'0'
;putchar(tmp)
mov eax, 4
mov ebx, 1
mov ecx, tmp
mov edx, 1
int 0x80

mov eax, dword[i]
sub eax, 1              ; eax = i-1
mov dword[i], eax       ; i--
jmp print2

print2_end:
mov ebx, 0x0a           ; ebx = '\n'
mov byte[tmp], bl       ; tmp = '\n'

mov eax, 4
mov ebx, 1
mov ecx, tmp
mov edx, 1              
int 0x80



end:
mov eax, 1
xor ebx, ebx
int 0x80





getchar:
    push eax 
    push ebx
    push ecx
    push edx

    ; 读取一个字符
    mov eax, 3      ; 系统调用号为3的read函数
    mov ebx, 0
    mov ecx, tmp
    mov edx, 1
    int 0x80

    pop edx
    pop ecx
    pop ebx
    pop eax
    ret
    
Equal:
mov eax, dword [i]          ;eax = i
add eax, dword [blen]       ;eax = i + blen
mov ebx, dword [d+eax*4]    ;ebx = d[i+blen]
cmp ebx, 0
jne is_true                 ;如果不相等，则返回true
;初始化j=blen-1
mov ecx, dword [blen]       ;ecx = blen
sub ecx, 1                  ;ecx = blen - 1
mov dword[j], ecx           ;j = blen - 1

loop6:
;如果j < 0 ,return true
cmp dword[j], 0
jl  is_true

mov eax, dword[i]           ; eax = i
mov ebx, dword[j]           ; ebx = j
add eax, ebx                ; eax = i+j
mov ecx, dword [d+eax*4]    ; ecx = d[i+j]
mov edx, dword [e+ebx*4]    ; edx = e[j]
cmp ecx, edx
jg  is_true
jl  is_false

;j--
sub ebx, 1                  
mov dword[j], ebx           
jmp loop6

is_false:
mov eax, 0
ret                         ;将eax设为0 返回

is_true:
mov eax, 1                  ;将eax设为1 返回
ret

divisor_is_zero:
mov eax,4
mov ebx,1
mov ecx, wrongMessage
mov edx, 16
int 80h
;print '\n'
mov ebx, 0x0a           ; ebx = '\n'
mov byte[tmp], bl       ; tmp = '\n'

mov eax, 4
mov ebx, 1
mov ecx, tmp
mov edx, 1              
int 0x80
jmp end

wrongMessage:   db  "divisor is zero!"

ans_is_zero:
mov byte[tmp], 0x30     ;print '0'
mov eax, 4
mov ebx, 1
mov ecx, tmp
mov edx, 1
int 0x80

mov byte[tmp], 0x0a     ;print '\n'
mov eax, 4
mov ebx, 1
mov ecx, tmp
mov edx, 1
int 0x80
jmp end