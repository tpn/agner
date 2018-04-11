;**************************  strlen32.asm  **********************************
; Author:           Agner Fog
; Date created:     2008-07-06
; Last modified:    2008-07-06
; Syntax:           MASM/ML 6.x, 32 bit
; Operating system: Windows, Linux, BSD or Mac, 32-bit x86
; Instruction set:  80386
; Description:
; Standard strlen function:
; size_t strlen(const char *str);
; Finds the length of a zero-terminated string of bytes, optimized for speed.
; Uses 32-bit registers to check four bytes at a time, all memory reads aligned.
;
; Alternatives:
; In 64-bit systems or when SSE2 is available, use strlenSSE2.asm
; More versions are available in www.agner.org/optimize/asmlib.zip
;
; The latest version of this file is available at:
; www.agner.org/optimize/asmexamples.zip
; Copyright (c) 2008 GNU General Public License www.gnu.org/copyleft/gpl.html
;******************************************************************************
.386
.model flat

.code

_strlen PROC    NEAR
; extern "C" int strlen (const char * s);
; Works in all 32-bit systems 
; In Linux, remove the underscore from the function name.

        push    ebx
        mov     ecx, [esp+8]           ; get pointer to string
        mov     eax, ecx               ; copy pointer
        and     ecx, 3                 ; lower 2 bits of address, check alignment
        jz      L2                     ; string is aligned by 4. Go to loop
        and     eax, -4                ; align pointer by 4
        mov     ebx, [eax]             ; read from nearest preceding boundary
        shl     ecx, 3                 ; mul by 8 = displacement in bits
        mov     edx, -1
        shl     edx, cl                ; make byte mask
        not     edx                    ; mask = 0FFH for false bytes
        or      ebx, edx               ; mask out false bytes

        ; check first four bytes for zero
        lea     ecx, [ebx-01010101H]   ; subtract 1 from each byte
        not     ebx                    ; invert all bytes
        and     ecx, ebx               ; and these two
        and     ecx, 80808080H         ; test all sign bits
        jnz     L3                     ; zero-byte found
        
        ; Main loop, read 4 bytes aligned
L1:     add     eax, 4                 ; increment pointer by 4
L2:     mov     ebx, [eax]             ; read 4 bytes of string
        lea     ecx, [ebx-01010101H]   ; subtract 1 from each byte
        not     ebx                    ; invert all bytes
        and     ecx, ebx               ; and these two
        and     ecx, 80808080H         ; test all sign bits
        jz      L1                     ; no zero bytes, continue loop
        
L3:     bsf     ecx, ecx               ; find right-most 1-bit
        shr     ecx, 3                 ; divide by 8 = byte index
        sub     eax, [esp+8]           ; subtract start address
        add     eax, ecx               ; add index to byte
        pop     ebx
        ret
_strlen ENDP

END
