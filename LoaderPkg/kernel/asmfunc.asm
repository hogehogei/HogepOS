; asmfunc.asm

; System V AMD64 Calling Convention

bits 64
section .text

global IoOut32  ; void IoOut32( uint16_t addr, uint32_t data );
IoOut32:
    mov dx, di      ; dx = addr
    mov eax, esi    ; eax = data
    out dx, eax
    ret

global IoIn32   ; void IoIn32( uint16_t addr );
IoIn32:
    mov dx, di      ; dx = addr
    in  eax, dx
    ret
