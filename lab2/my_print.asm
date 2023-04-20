SECTION .data
color_red:      db  1Bh, '[31;1m', 0
.len            equ $ - color_red
color_default:  db  1Bh, '[37;0m', 0
.len            equ $ - color_default

SECTION .text
global my_print

my_print:
    push    ebp
    mov     ebp, esp
    mov     eax, [ebp+16]
    push    ebx
    cmp     eax, 0
    je      begin_print
    
    mov     eax, 4
    mov     ebx, 1
    mov     ecx, color_red
    mov     edx, color_red.len
    int     80h
begin_print:
    mov     ecx, [ebp+8]
    mov     edx, [ebp+12]
    mov     eax, 4
    mov     ebx, 1
    int     80h

    mov     eax, 4
    mov     ebx, 1
    mov     ecx, color_default
    mov     edx, color_default.len
    int     80h
    xor ebx,ebx
    pop ebx
    pop    ebp
    ret
