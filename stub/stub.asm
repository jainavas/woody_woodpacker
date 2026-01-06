; ************************************************************************** ;
;                                                                            ;
;                                                        :::      ::::::::   ;
;   stub.asm                                           :+:      :+:    :+:   ;
;                                                    +:+ +:+         +:+     ;
;   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        ;
;                                                +#+#+#+#+#+   +#+           ;
;   Created: 2025/01/06 23:50:00 by jainavas          #+#    #+#             ;
;   Updated: 2025/01/06 23:50:01 by jainavas         ###   ########.fr       ;
;                                                                            ;
; ************************************************************************** ;

BITS 64

; Syscalls
%define SYS_WRITE 1
%define SYS_MPROTECT 10
%define SYS_EXIT 60

; Permisos
%define PROT_RWX 7
%define PROT_RX 5

section .text
    global _start

_start:
    ; Los datos están en una posición fija: 440 bytes desde el inicio
    ; Como el stub está en 0x405000, los datos están en 0x405000 + 440
    push rdx
    ; Obtener la dirección base usando RIP-relative
    lea rsi, [rel _start]       ; RSI = dirección de _start
    add rsi, 440                ; RSI = _start + 440 = stub_data
    
    ; Cargar datos
    mov r12, [rsi]              ; R12 = text_addr
    mov r13, [rsi + 8]          ; R13 = text_size
    mov r14, [rsi + 16]         ; R14 = old_entry
    lea r15, [rsi + 24]         ; R15 = dirección de la clave (16 bytes)
    
    ; === 1. mprotect: hacer .text escribible ===
    ; Alinear text_addr a página (4096 = 0x1000)
    mov rdi, r12
    and rdi, ~0xFFF             ; page_start
    
    ; Calcular page_size
    mov rax, r12
    add rax, r13
    add rax, 0xFFF
    and rax, ~0xFFF             ; page_end
    sub rax, rdi                ; page_size = page_end - page_start
    mov rsi, rax
    
    mov rdx, PROT_RWX           ; permisos RWX
    mov rax, SYS_MPROTECT
    syscall
    
    ; === 2. Descifrar con RC4 ===
    ; Inicializar S en el stack
    sub rsp, 256                ; S[256] en el stack
    mov rdi, rsp                ; RDI = S
    mov rsi, r15                ; RSI = key
    call rc4_init
    
    mov rdi, rsp                ; RDI = S
    mov rsi, r12                ; RSI = text_addr
    mov rdx, r13                ; RDX = text_size
    call rc4_crypt
    
    add rsp, 256                ; Limpiar stack
    
    ; === 3. mprotect: restaurar .text a RX ===
    mov rdi, r12
    and rdi, ~0xFFF
    mov rax, r12
    add rax, r13
    add rax, 0xFFF
    and rax, ~0xFFF
    sub rax, rdi
    mov rsi, rax
    mov rdx, PROT_RX
    mov rax, SYS_MPROTECT
    syscall
    
    ; === 4. Imprimir "....WOODY...." ===
    mov rax, SYS_WRITE
    mov rdi, 1                  ; stdout
    lea rsi, [rel woody_msg]
    mov rdx, 14
    syscall
    
    ; === 5. Saltar al entry point original ===
    jmp r14

; ===========================
; RC4 Init: rc4_init(S, key, key_len)
; RDI = S (256 bytes)
; RSI = key
; ===========================
rc4_init:
    push rbx
    push rcx
    push r8
    push r9
    
    ; Inicializar S[i] = i
    xor rcx, rcx                ; i = 0
.init_loop:
    mov byte [rdi + rcx], cl
    inc rcx
    cmp rcx, 256
    jl .init_loop
    
    ; Barajar S con la clave
    xor rcx, rcx                ; i = 0
    xor r8, r8                  ; j = 0
    mov r9, 16                  ; key_len = 16
    
.shuffle_loop:
    movzx rax, byte [rdi + rcx]     ; S[i]
    mov rbx, rcx
    xor rdx, rdx
    mov rax, rbx
    div r9                          ; i % 16
    movzx rax, byte [rsi + rdx]     ; key[i % 16]
    movzx rbx, byte [rdi + rcx]     ; S[i]
    add r8, rbx
    add r8, rax
    and r8, 0xFF                    ; j = (j + S[i] + key[i%16]) % 256
    
    ; Swap S[i] y S[j]
    mov al, [rdi + rcx]
    mov bl, [rdi + r8]
    mov [rdi + rcx], bl
    mov [rdi + r8], al
    
    inc rcx
    cmp rcx, 256
    jl .shuffle_loop
    
    pop r9
    pop r8
    pop rcx
    pop rbx
    ret

; ===========================
; RC4 Crypt: rc4_crypt(S, data, len)
; RDI = S
; RSI = data
; RDX = len
; ===========================
rc4_crypt:
    push rbx
    push rcx
    push r8
    push r9
    push r10
    push r11
    
    xor r8, r8                  ; i = 0
    xor r9, r9                  ; j = 0
    xor rcx, rcx                ; k = 0
    
.crypt_loop:
    cmp rcx, rdx
    jge .crypt_done
    
    ; i = (i + 1) % 256
    inc r8
    and r8, 0xFF
    
    ; j = (j + S[i]) % 256
    xor rax, rax
    mov al, byte [rdi + r8]
    add r9, rax
    and r9, 0xFF
    
    ; Swap S[i] y S[j]
    mov al, byte [rdi + r8]
    mov bl, byte [rdi + r9]
    mov byte [rdi + r8], bl
    mov byte [rdi + r9], al
    
    ; Calcular índice: (S[i] + S[j]) % 256
    xor rax, rax
    xor rbx, rbx
    mov al, byte [rdi + r8]
    mov bl, byte [rdi + r9]
    add rax, rbx
    and rax, 0xFF
    
    ; rnd = S[índice]
    xor r10, r10
    mov r10b, byte [rdi + rax]
    
    ; data[k] ^= rnd
    mov r11b, byte [rsi + rcx]
    xor r11b, r10b
    mov byte [rsi + rcx], r11b
    
    inc rcx
    jmp .crypt_loop
    
.crypt_done:
    pop r11
    pop r10
    pop r9
    pop r8
    pop rcx
    pop rbx
    ret

woody_msg:
    db "....WOODY....", 10

; Alineamiento
align 8

; ===========================
; DATOS DEL STUB
; El packer escribirá aquí:
; - text_addr (8 bytes)
; - text_size (8 bytes)  
; - old_entry (8 bytes)
; - key[16] (16 bytes)
; TOTAL: 40 bytes
; ===========================
stub_data:
    dq 0    ; text_addr
    dq 0    ; text_size
    dq 0    ; old_entry
    times 16 db 0  ; key[16]