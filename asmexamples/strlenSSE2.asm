;*************************  strlenSSE2.asm  **********************************
; Author:           Agner Fog
; Date created:     2008-07-06
; Last modified:    2008-07-06
; Syntax:           MASM/ML 6.x, 32 bit
; Operating system: Windows, Linux, BSD or Mac, 32-bit x86
; Instruction set:  SSE2
; Description:
; Standard strlen function:
; size_t strlen(const char * str);
; Finds the length of a zero-terminated string of bytes, optimized for speed.
; Uses XMM registers to check 16 bytes at a time, all memory reads aligned.
;
; Alternatives:
; 64-bit versions given below
; If SSE2 not available, use strlen32.asm
; More versions are available in www.agner.org/optimize/asmlib.zip
;
; The latest version of this file is available at:
; www.agner.org/optimize/asmexamples.zip
; Copyright (c) 2008 GNU General Public License www.gnu.org/copyleft/gpl.html
;******************************************************************************
.386
.xmm
.model flat

.code

; extern "C" size_t strlen (const char * str);
; Works in all 32-bit systems 
; In Linux, remove the underscore from the function name.
_strlen PROC     NEAR
        mov      eax,  [esp+4]         ; get pointer to string
        mov      ecx,  eax             ; copy pointer
        pxor     xmm0, xmm0            ; set to zero
        and      ecx,  15              ; lower 4 bits indicate misalignment
        and      eax,  -16             ; align pointer by 16
        movdqa   xmm1, [eax]           ; read from nearest preceding boundary
        pcmpeqb  xmm1, xmm0            ; compare 16 bytes with zero
        pmovmskb edx,  xmm1            ; get one bit for each byte result
        shr      edx,  cl              ; shift out false bits
        shl      edx,  cl              ; shift back again
        bsf      edx,  edx             ; find first 1-bit
        jnz      L2                    ; found
        
        ; Main loop, search 16 bytes at a time
L1:     add      eax,  16              ; increment pointer by 16
        movdqa   xmm1, [eax]           ; read 16 bytes aligned
        pcmpeqb  xmm1, xmm0            ; compare 16 bytes with zero
        pmovmskb edx,  xmm1            ; get one bit for each byte result
        bsf      edx,  edx             ; find first 1-bit
        jz       L1                    ; loop if not found
        
L2:     ; Zero-byte found. Compute string length        
        sub      eax,  [esp+4]         ; subtract start address
        add      eax,  edx             ; add byte index
        ret
        
_strlen endp        

END

comment #

; 64-bit Windows version:
strlen  PROC
        mov      rax,  rcx             ; get pointer to string from rcx
        mov      r8,   rcx             ; copy pointer
        pxor     xmm0, xmm0            ; set to zero
        and      ecx,  15              ; lower 4 bits indicate misalignment
        and      rax,  -16             ; align pointer by 16
        movdqa   xmm1, [rax]           ; read from nearest preceding boundary
        pcmpeqb  xmm1, xmm0            ; compare 16 bytes with zero
        pmovmskb edx,  xmm1            ; get one bit for each byte result
        shr      edx,  cl              ; shift out false bits
        shl      edx,  cl              ; shift back again
        bsf      edx,  edx             ; find first 1-bit
        jnz      L2                    ; found
        
        ; Main loop, search 16 bytes at a time
L1:     add      rax,  16              ; increment pointer by 16
        movdqa   xmm1, [rax]           ; read 16 bytes aligned
        pcmpeqb  xmm1, xmm0            ; compare 16 bytes with zero
        pmovmskb edx,  xmm1            ; get one bit for each byte result
        bsf      edx,  edx             ; find first 1-bit
        jz       L1                    ; loop if not found
        
L2:     ; Zero-byte found. Compute string length        
        sub      rax,  r8              ; subtract start address
        add      rax,  rdx             ; add byte index
        ret
        
strlen  endp        

; 64-bit Linux version:
strlen  PROC
        mov      rax,  rdi             ; get pointer to string from rdi
        mov      ecx,  edi             ; copy pointer (lower 32 bits)
        pxor     xmm0, xmm0            ; set to zero
        and      ecx,  15              ; lower 4 bits indicate misalignment
        and      rax,  -16             ; align pointer by 16
        movdqa   xmm1, [rax]           ; read from nearest preceding boundary
        pcmpeqb  xmm1, xmm0            ; compare 16 bytes with zero
        pmovmskb edx,  xmm1            ; get one bit for each byte result
        shr      edx,  cl              ; shift out false bits
        shl      edx,  cl              ; shift back again
        bsf      edx,  edx             ; find first 1-bit
        jnz      L2                    ; found
        
        ; Main loop, search 16 bytes at a time
L1:     add      rax,  16              ; increment pointer by 16
        movdqa   xmm1, [rax]           ; read 16 bytes aligned
        pcmpeqb  xmm1, xmm0            ; compare 16 bytes with zero
        pmovmskb edx,  xmm1            ; get one bit for each byte result
        bsf      edx,  edx             ; find first 1-bit
        jz       L1                    ; loop if not found
        
L2:     ; Zero-byte found. Compute string length        
        sub      rax,  rdi             ; subtract start address
        add      rax,  rdx             ; add byte index
        ret
        
strlen  endp        

#