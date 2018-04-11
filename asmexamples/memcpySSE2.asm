;*************************  memcpySSE2.asm  **********************************
; Author:           Agner Fog
; Date created:     2008-07-09
; Last modified:    2008-08-24
; Syntax:           MASM/ML 6.x, 32 bit
; Operating system: Windows, Linux, BSD or Mac, 32-bit x86
; Instruction set:  SSE2 required, Suppl.SSE3 used if available
; Description:
; Standard memcpy function:
; void *memcpy(void *dest, const void *src, size_t count);
; Copies 'count' bytes from 'src' to 'dest'
;
; Optimization:
; Uses XMM registers to copy 16 bytes at a time, aligned.
; If source and destination are misaligned relative to each other
; then the code will combine parts of every two consecutive 16-bytes 
; blocks from the source into one 16-bytes register which is written 
; to the destination, aligned.
; This method is 2 - 6 times faster than the implementations in the
; standard C libraries (MS, Gnu) when src or dest are misaligned.
; When src and dest are aligned by 16 (relative to each other) then this
; function is only slightly faster than the best standard libraries.
;
; Alternatives:
; 64-bit versions etc. are available in www.agner.org/optimize/asmlib.zip
;
; The latest version of this file is available at:
; www.agner.org/optimize/asmexamples.zip
; Copyright (c) 2008 GNU General Public License www.gnu.org/copyleft/gpl.html
;******************************************************************************
.386
.xmm
.model flat

public _memcpy                         ; Function memcpy
public _CacheBypassLimit               ; Bypass cache if count > _CacheBypassLimit
public $memcpyEntry2                   ; Entry from memmove


.code

; extern "C" void *memcpy(void *dest, const void *src, size_t count);
; Function entry:
_memcpy PROC    NEAR
        push    esi
        push    edi
        mov     edi, [esp+12]          ; dest
        mov     esi, [esp+16]          ; src
        mov     ecx, [esp+20]          ; count
        
$memcpyEntry2 label near  ; entry from memmove
public $memcpyEntry2

        cmp     ecx, 40H
        jae     B100                    ; Use simpler code if count < 64
        
        ; count < 64. Move 32-16-8-4-2-1 bytes
        add     esi, ecx               ; end of src
        add     edi, ecx               ; end of dest
        neg     ecx                    ; negative index from the end
        cmp     ecx, -20H
        jg      A100        
        ; move 32 bytes
        ; movq is faster than movdqu on current processors (2008),
        ; movdqu may be faster on future processors
        movq    xmm0, qword ptr [esi+ecx]
        movq    xmm1, qword ptr [esi+ecx+8]
        movq    xmm2, qword ptr [esi+ecx+10H]
        movq    xmm3, qword ptr [esi+ecx+18H]
        movq    qword ptr [edi+ecx], xmm0
        movq    qword ptr [edi+ecx+8], xmm1
        movq    qword ptr [edi+ecx+10H], xmm2
        movq    qword ptr [edi+ecx+18H], xmm3
        add     ecx, 20H
A100:   cmp     ecx, -10H        
        jg      A200
        ; move 16 bytes
        movq    xmm0, qword ptr [esi+ecx]
        movq    xmm1, qword ptr [esi+ecx+8]
        movq    qword ptr [edi+ecx], xmm0
        movq    qword ptr [edi+ecx+8], xmm1
        add     ecx, 10H
A200:   cmp     ecx, -8        
        jg      A300
        ; move 8 bytes
        movq    xmm0, qword ptr [esi+ecx]
        movq    qword ptr [edi+ecx], xmm0
        add     ecx, 8
A300:   cmp     ecx, -4        
        jg      A400
        ; move 4 bytes
        mov     eax, [esi+ecx]
        mov     [edi+ecx], eax
        add     ecx, 4
        jz      A900                     ; early out if count divisible by 4
A400:   cmp     ecx, -2        
        jg      A500
        ; move 2 bytes
        movzx   eax, word ptr [esi+ecx]
        mov     [edi+ecx], ax
        add     ecx, 2
A500:   cmp     ecx, -1
        jg      A900        
        ; move 1 byte
        movzx   eax, byte ptr [esi+ecx]
        mov     [edi+ecx], al
A900:   ; finished
        pop     edi
        pop     esi
        mov     eax, [esp+4]           ; Return value = dest
        ret
        
B100:   ; count >= 64
        ; Note: this part will not always work if count < 64
        ; Calculate size of first block up to first regular boundary of dest
        mov     edx, edi
        neg     edx
        and     edx, 0FH
        jz      B300                    ; Skip if dest aligned by 16
        
        ; edx = size of first partial block, 1 - 15 bytes
        add     esi, edx
        add     edi, edx
        sub     ecx, edx
        neg     edx
        cmp     edx, -8
        jg      B200
        ; move 8 bytes
        movq    xmm0, qword ptr [esi+edx]
        movq    qword ptr [edi+edx], xmm0
        add     edx, 8
B200:   cmp     edx, -4        
        jg      B210
        ; move 4 bytes
        mov     eax, [esi+edx]
        mov     [edi+edx], eax
        add     edx, 4
B210:   cmp     edx, -2        
        jg      B220
        ; move 2 bytes
        movzx   eax, word ptr [esi+edx]
        mov     [edi+edx], ax
        add     edx, 2
B220:   cmp     edx, -1
        jg      B300
        ; move 1 byte
        movzx   eax, byte ptr [esi+edx]
        mov     [edi+edx], al
        
B300:   ; Now dest is aligned by 16. Any partial block has been moved        
        ; Find alignment of src modulo 16 at this point:
        mov     eax, esi
        and     eax, 0FH
        
        ; Set up for loop moving 32 bytes per iteration:
        mov     edx, ecx               ; Save count
        and     ecx, -20H              ; Round down to nearest multiple of 32
        add     esi, ecx               ; Point to the end
        add     edi, ecx               ; Point to the end
        sub     edx, ecx               ; Remaining data after loop
        sub     esi, eax               ; Nearest preceding aligned block of src

        ; Check if count very big
        cmp     ecx, [_CacheBypassLimit]
        ja      B400                   ; Use non-temporal store if count > _CacheBypassLimit
        neg     ecx                    ; Negative index from the end
        ; Dispatch to different codes depending on src alignment
        jmp     AlignmentDispatch[eax*4]
        
B400:   neg     ecx                    ; Negative index from the end
        ; Dispatch to different codes depending on src alignment
        jmp     AlignmentDispatchNT[eax*4]
        

C100:   ; Code for aligned src. SSE2 or later instruction set
        ; The nice case, src and dest have same alignment.

        ; Loop. ecx has negative index from the end, counting up to zero
        movaps  xmm0, [esi+ecx]
        movaps  xmm1, [esi+ecx+10H]
        movaps  [edi+ecx], xmm0
        movaps  [edi+ecx+10H], xmm1
        add     ecx, 20H
        jnz     C100
        
        ; Move the remaining edx bytes (0 - 31):
        add     esi, edx
        add     edi, edx
        neg     edx
        jz      C500                   ; Skip if no more data
        ; move 16-8-4-2-1 bytes, aligned
        cmp     edx, -10H
        jg      C200
        ; move 16 bytes
        movaps  xmm0, [esi+edx]
        movaps  [edi+edx], xmm0
        add     edx, 10H
C200:   cmp     edx, -8
        jg      C210        
        ; move 8 bytes
        movq    xmm0, qword ptr [esi+edx]
        movq    qword ptr [edi+edx], xmm0
        add     edx, 8 
        jz      C500                   ; Early skip if count divisible by 8       
C210:   cmp     edx, -4
        jg      C220        
        ; move 4 bytes
        mov     eax, [esi+edx]
        mov     [edi+edx], eax
        add     edx, 4        
C220:   cmp     edx, -2
        jg      C230        
        ; move 2 bytes
        movzx   eax, word ptr [esi+edx]
        mov     [edi+edx], ax
        add     edx, 2
C230:   cmp     edx, -1
        jg      C500        
        ; move 1 byte
        movzx   eax, byte ptr [esi+edx]
        mov     [edi+edx], al
C500:   ; finished     
        pop     edi
        pop     esi
        mov     eax, [esp+4]           ; Return value = dest
        ret
        
       
; Code for each src alignment, SSE2 instruction set:
; Make separate code for each alignment u because the shift instructions
; have the shift count as a constant:

MOVE_UNALIGNED_SSE2 MACRO u, nt
; Move ecx + edx bytes of data
; Source is misaligned. (src-dest) modulo 16 = u
; nt = 1 if non-temporal store desired
; eax = u
; esi = src - u = nearest preceding 16-bytes boundary
; edi = dest (aligned)
; ecx = - (count rounded down to nearest divisible by 32)
; edx = remaining bytes to move after loop
LOCAL L1, L2
        movdqa  xmm0, [esi+ecx]        ; Read from nearest preceding 16B boundary
L1:    ; Loop. ecx has negative index from the end, counting up to zero
        movdqa  xmm1, [esi+ecx+10H]    ; Read next two blocks aligned
        movdqa  xmm2, [esi+ecx+20H]
        movdqa  xmm3, xmm1             ; Copy because used twice
        psrldq  xmm0, u                ; shift right
        pslldq  xmm1, 16-u             ; shift left
        por     xmm0, xmm1             ; combine blocks
        IF nt eq 0
        movdqa  [edi+ecx], xmm0        ; Save aligned
        ELSE
        movntdq [edi+ecx], xmm0        ; non-temporal save
        ENDIF
        movdqa  xmm0, xmm2             ; Save for next iteration
        psrldq  xmm3, u                ; shift right
        pslldq  xmm2, 16-u             ; shift left
        por     xmm3, xmm2             ; combine blocks
        IF nt eq 0
        movdqa  [edi+ecx+10H], xmm3    ; Save aligned
        ELSE
        movntdq [edi+ecx+10H], xmm3    ; non-temporal save
        ENDIF
        add     ecx, 20H               ; Loop through negative values up to zero
        jnz     L1
        
        ; Set up for edx remaining bytes
        add     esi, edx
        add     edi, edx
        neg     edx
        cmp     edx, -10H
        jg      L2
        ; One more 16-bytes block to move
        movdqa  xmm1, [esi+edx+10H]
        psrldq  xmm0, u                ; shift right
        pslldq  xmm1, 16-u             ; shift left
        por     xmm0, xmm1             ; combine blocks
        IF nt eq 0
        movdqa  [edi+edx], xmm0        ; Save aligned
        ELSE
        movntdq [edi+edx], xmm0        ; non-temporal save
        ENDIF        
        add     edx, 10H        
L2:     ; Get src pointer back to misaligned state
        add     esi, eax
        ; Move remaining 0 - 15 bytes, unaligned
        jmp     C200
ENDM

MOVE_UNALIGNED_SSE2_4 MACRO nt
; Special case for u = 4
LOCAL L1, L2
        movaps  xmm0, [esi+ecx]        ; Read from nearest preceding 16B boundary
L1:     ; Loop. ecx has negative index from the end, counting up to zero
        movaps  xmm1, [esi+ecx+10H]    ; Read next two blocks aligned
        movss   xmm0, xmm1             ; Moves 4 bytes, leaves remaining bytes unchanged
        pshufd  xmm0, xmm0, 00111001B
        IF nt eq 0
        movdqa  [edi+ecx], xmm0        ; Save aligned
        ELSE
        movntdq [edi+ecx], xmm0        ; Non-temporal save
        ENDIF
        movaps  xmm0, [esi+ecx+20H]
        movss   xmm1, xmm0
        pshufd  xmm1, xmm1, 00111001B
        IF nt eq 0
        movdqa  [edi+ecx+10H], xmm1    ; Save aligned
        ELSE
        movntdq [edi+ecx+10H], xmm1    ; Non-temporal save
        ENDIF
        add     ecx, 20H               ; Loop through negative values up to zero
        jnz     L1        
        ; Set up for edx remaining bytes
        add     esi, edx
        add     edi, edx
        neg     edx
        cmp     edx, -10H
        jg      L2
        ; One more 16-bytes block to move
        movaps  xmm1, [esi+edx+10H]    ; Read next two blocks aligned
        movss   xmm0, xmm1
        pshufd  xmm0, xmm0, 00111001B
        IF nt eq 0
        movdqa  [edi+edx], xmm0        ; Save aligned
        ELSE
        movntdq [edi+edx], xmm0        ; Non-temporal save
        ENDIF
        add     edx, 10H        
L2:     ; Get src pointer back to misaligned state
        add     esi, eax
        ; Move remaining 0 - 15 bytes, unaligned
        jmp     C200
ENDM 

MOVE_UNALIGNED_SSE2_8 MACRO nt
; Special case for u = 8
LOCAL L1, L2
        movaps  xmm0, [esi+ecx]        ; Read from nearest preceding 16B boundary
L1:     ; Loop. ecx has negative index from the end, counting up to zero
        movaps  xmm1, [esi+ecx+10H]    ; Read next two blocks aligned
        movsd   xmm0, xmm1             ; Moves 8 bytes, leaves remaining bytes unchanged
        shufps  xmm0, xmm0, 01001110B  ; Rotate
        IF nt eq 0
        movdqa  [edi+ecx], xmm0        ; Save aligned
        ELSE
        movntdq [edi+ecx], xmm0        ; Non-temporal save
        ENDIF
        movaps  xmm0, [esi+ecx+20H]
        movsd   xmm1, xmm0
        shufps  xmm1, xmm1, 01001110B
        IF nt eq 0
        movdqa  [edi+ecx+10H], xmm1    ; Save aligned
        ELSE
        movntdq [edi+ecx+10H], xmm1    ; Non-temporal save
        ENDIF
        add     ecx, 20H               ; Loop through negative values up to zero
        jnz     L1        
        ; Set up for edx remaining bytes
        add     esi, edx
        add     edi, edx
        neg     edx
        cmp     edx, -10H
        jg      L2
        ; One more 16-bytes block to move
        movaps  xmm1, [esi+edx+10H]    ; Read next two blocks aligned
        movsd   xmm0, xmm1
        shufps  xmm0, xmm0, 01001110B
        IF nt eq 0
        movdqa  [edi+edx], xmm0        ; Save aligned
        ELSE
        movntdq [edi+edx], xmm0        ; Non-temporal save
        ENDIF
        add     edx, 10H        
L2:     ; Get src pointer back to misaligned state
        add     esi, eax
        ; Move remaining 0 - 15 bytes, unaligned
        jmp     C200
ENDM 

MOVE_UNALIGNED_SSE2_12 MACRO nt
; Special case for u = 12
LOCAL L1, L2
        movaps  xmm0, [esi+ecx]        ; Read from nearest preceding 16B boundary
        pshufd  xmm0, xmm0, 10010011B
L1:     ; Loop. ecx has negative index from the end, counting up to zero
        movaps  xmm1, [esi+ecx+10H]    ; Read next two blocks aligned
        movaps  xmm2, [esi+ecx+20H]
        pshufd  xmm1, xmm1, 10010011B
        pshufd  xmm2, xmm2, 10010011B
        movaps  xmm3, xmm2
        movss   xmm2, xmm1             ; Moves 4 bytes, leaves remaining bytes unchanged
        movss   xmm1, xmm0             ; Moves 4 bytes, leaves remaining bytes unchanged       
        IF nt eq 0
        movdqa  [edi+ecx], xmm1        ; Save aligned
        movdqa  [edi+ecx+10H], xmm2    ; Save aligned
        ELSE
        movntdq [edi+ecx], xmm1        ; Non-temporal save
        movntdq [edi+ecx+10H], xmm2    ; Non-temporal save
        ENDIF
        movaps  xmm0, xmm3             ; Save for next iteration        
        add     ecx, 20H               ; Loop through negative values up to zero
        jnz     L1        
        ; Set up for edx remaining bytes
        add     esi, edx
        add     edi, edx
        neg     edx
        cmp     edx, -10H
        jg      L2
        ; One more 16-bytes block to move
        movaps  xmm1, [esi+edx+10H]    ; Read next two blocks aligned
        pshufd  xmm1, xmm1, 10010011B
        movss   xmm1, xmm0             ; Moves 4 bytes, leaves remaining bytes unchanged       
        IF nt eq 0
        movdqa  [edi+edx], xmm1        ; Save aligned
        ELSE
        movntdq [edi+edx], xmm1        ; Non-temporal save
        ENDIF
        add     edx, 10H        
L2:     ; Get src pointer back to misaligned state
        add     esi, eax
        ; Move remaining 0 - 15 bytes, unaligned
        jmp     C200
ENDM 

; Code for each src alignment, Suppl.SSE3 instruction set:
; Make separate code for each alignment u because the palignr instruction
; has the shift count as a constant:

MOVE_UNALIGNED_SSSE3 MACRO u
; Move ecx + edx bytes of data
; Source is misaligned. (src-dest) modulo 16 = u
; eax = u
; esi = src - u = nearest preceding 16-bytes boundary
; edi = dest (aligned)
; ecx = - (count rounded down to nearest divisible by 32)
; edx = remaining bytes to move after loop
LOCAL L1, L2
        movdqa  xmm0, [esi+ecx]        ; Read from nearest preceding 16B boundary
        
L1:     ; Loop. ecx has negative index from the end, counting up to zero
        movdqa  xmm2, [esi+ecx+10H]    ; Read next two blocks
        movdqa  xmm3, [esi+ecx+20H]
        movdqa  xmm1, xmm0             ; Save xmm0
        movdqa  xmm0, xmm3             ; Save for next iteration
        palignr xmm3, xmm2, u          ; Combine parts into aligned block
        palignr xmm2, xmm1, u          ; Combine parts into aligned block
        movdqa  [edi+ecx], xmm2        ; Save aligned
        movdqa  [edi+ecx+10H], xmm3    ; Save aligned
        add     ecx, 20H
        jnz     L1
        
        ; Set up for edx remaining bytes
        add     esi, edx
        add     edi, edx
        neg     edx
        cmp     edx, -10H
        jg      L2
        ; One more 16-bytes block to move
        movdqa  xmm2, [esi+edx+10H]
        palignr xmm2, xmm0, u
        movdqa  [edi+edx], xmm2
        add     edx, 10H        
L2:     ; Get src pointer back to misaligned state
        add     esi, eax
        ; Move remaining 0 - 15 bytes
        jmp     C200
ENDM        

; Make 15 instances of SSE2 macro for each value of the alignment u.
; These are pointed to by the jump table AlignmentDispatchSSE2 below

D101:   MOVE_UNALIGNED_SSE2 1,   0
D102:   MOVE_UNALIGNED_SSE2 2,   0
D103:   MOVE_UNALIGNED_SSE2 3,   0
D104:   MOVE_UNALIGNED_SSE2_4    0
D105:   MOVE_UNALIGNED_SSE2 5,   0
D106:   MOVE_UNALIGNED_SSE2 6,   0
D107:   MOVE_UNALIGNED_SSE2 7,   0
D108:   MOVE_UNALIGNED_SSE2_8    0
D109:   MOVE_UNALIGNED_SSE2 9,   0
D10A:   MOVE_UNALIGNED_SSE2 0AH, 0
D10B:   MOVE_UNALIGNED_SSE2 0BH, 0
D10C:   MOVE_UNALIGNED_SSE2_12   0
D10D:   MOVE_UNALIGNED_SSE2 0DH, 0
D10E:   MOVE_UNALIGNED_SSE2 0EH, 0
D10F:   MOVE_UNALIGNED_SSE2 0FH, 0
        
; Make 15 instances of Sup.SSE3 macro for each value of the alignment u.
; These are pointed to by the jump table AlignmentDispatchSupSSE3 below

E101:   MOVE_UNALIGNED_SSSE3 1
E102:   MOVE_UNALIGNED_SSSE3 2
E103:   MOVE_UNALIGNED_SSSE3 3
E104:   MOVE_UNALIGNED_SSSE3 4
E105:   MOVE_UNALIGNED_SSSE3 5
E106:   MOVE_UNALIGNED_SSSE3 6
E107:   MOVE_UNALIGNED_SSSE3 7
E108:   MOVE_UNALIGNED_SSSE3 8
E109:   MOVE_UNALIGNED_SSSE3 9
E10A:   MOVE_UNALIGNED_SSSE3 0AH
E10B:   MOVE_UNALIGNED_SSSE3 0BH
E10C:   MOVE_UNALIGNED_SSSE3 0CH
E10D:   MOVE_UNALIGNED_SSSE3 0DH
E10E:   MOVE_UNALIGNED_SSSE3 0EH
E10F:   MOVE_UNALIGNED_SSSE3 0FH


; Codes for non-temporal move. Aligned case first

F100:   ; Non-temporal move, src and dest have same alignment.
        ; Loop. ecx has negative index from the end, counting up to zero
        movaps  xmm0, [esi+ecx]        ; Read
        movaps  xmm1, [esi+ecx+10H]
        movntps [edi+ecx], xmm0        ; Write non-temporal (bypass cache)
        movntps [edi+ecx+10H], xmm1
        add     ecx, 20H
        jnz     F100                   ; Loop through negative ecx up to zero
                
        ; Move the remaining edx bytes (0 - 31):
        add     esi, edx
        add     edi, edx
        neg     edx
        jz      C500                   ; Skip if no more data
        ; Check if we can more one more 16-bytes block
        cmp     edx, -10H
        jg      C200
        ; move 16 bytes, aligned
        movaps  xmm0, [esi+edx]
        movntps [edi+edx], xmm0
        add     edx, 10H
        ; move the remaining 0 - 15 bytes
        jmp     C200

; Make 15 instances of MOVE_UNALIGNED_SSE2 macro for each value of 
; the alignment u.
; These are pointed to by the jump table AlignmentDispatchNT below

F101:   MOVE_UNALIGNED_SSE2 1,   1
F102:   MOVE_UNALIGNED_SSE2 2,   1
F103:   MOVE_UNALIGNED_SSE2 3,   1
F104:   MOVE_UNALIGNED_SSE2_4    1
F105:   MOVE_UNALIGNED_SSE2 5,   1
F106:   MOVE_UNALIGNED_SSE2 6,   1
F107:   MOVE_UNALIGNED_SSE2 7,   1
F108:   MOVE_UNALIGNED_SSE2_8    1
F109:   MOVE_UNALIGNED_SSE2 9,   1
F10A:   MOVE_UNALIGNED_SSE2 0AH, 1
F10B:   MOVE_UNALIGNED_SSE2 0BH, 1
F10C:   MOVE_UNALIGNED_SSE2_12   1
F10D:   MOVE_UNALIGNED_SSE2 0DH, 1
F10E:   MOVE_UNALIGNED_SSE2 0EH, 1
F10F:   MOVE_UNALIGNED_SSE2 0FH, 1


Q100:   ; CPU dispatcher, check for SupSSE3 instruction set
        ; This part is executed only once, optimized for size
        pushad                         ; All registers must be saved
        mov     eax, 1
        cpuid                          ; Get feature flags
        mov     esi, offset AlignmentDispatchSSE2
        bt      ecx, 9                 ; Test bit for SupSSE3
        jnc     Q200
        mov     esi, offset AlignmentDispatchSupSSE3
Q200:   ; Insert appropriate table
        mov     edi, offset AlignmentDispatch
        mov     ecx, 16
        rep     movsd
        popad
        ; Jump according to the replaced table
        jmp     AlignmentDispatch[eax*4]


; Data segment must be included in function namespace
.data

; Jump table for alignments 0 - 15:
; The first table initially points to a CPU dispatcher
; The CPU dispatcher replaces the table with one of the
; tables below, according to the available instruction set:

AlignmentDispatch label dword
DD Q100, Q100, Q100, Q100, Q100, Q100, Q100, Q100
DD Q100, Q100, Q100, Q100, Q100, Q100, Q100, Q100

; Code pointer for each alignment for SSE2 instruction set
AlignmentDispatchSSE2 label dword
DD C100, D101, D102, D103, D104, D105, D106, D107
DD D108, D109, D10A, D10B, D10C, D10D, D10E, D10F

; Code pointer for each alignment for Suppl.SSE3 instruction set
AlignmentDispatchSupSSE3 label dword
DD C100, E101, E102, E103, E104, E105, E106, E107
DD E108, E109, E10A, E10B, E10C, E10D, E10E, E10F

; Code pointer for each alignment for non-temporal store
AlignmentDispatchNT label dword
DD F100, F101, F102, F103, F104, F105, F106, F107
DD F108, F109, F10A, F10B, F10C, F10D, F10E, F10F

; Bypass cache by using non-temporal moves if count > _CacheBypassLimit
; The optimal value of _CacheBypassLimit is difficult to estimate, but
; a reasonable value is half the size of the largest level cache:
_CacheBypassLimit DD 400000H           ; 400000H = 4 Megabytes
public _CacheBypassLimit

.code
_memcpy ENDP                           ; End of function namespace

END

comment #

Alternative implementations:
============================

1. Use PSHUFB and avoid the many branches
-----------------------------------------

; An alternative method without dispatching into 16 different branches for 
; each alignment uses the PSHUFB instruction for shifting left and right.
; This requires the Supplementary-SSE3 instruction set:

.data

; Masks for using PSHUFB instruction as shift instruction:
; The 16 bytes from ShiftMask[16+u] will shift right u bytes
; The 16 bytes from ShiftMask[16-u] will shift left u bytes
ShiftMask label xmmword
        DB -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
        DB  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15
        DB -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1

.code 

; Code for unaligned src, SSE2 instruction set
; Move ecx + edx bytes of data
; Source is misaligned. (src-dest) modulo 16 = u
; eax = u
; esi = src - u = nearest preceding 16-bytes boundary
; edi = dest (aligned)
; ecx = - (count rounded down to nearest divisible by 32)
; edx = remaining bytes to move after loop

        movdqu  xmm4, [ShiftMask+10H+eax]   ; Mask for shift right by u
        movdqu  xmm5, [ShiftMask+eax]       ; Mask for shift left by 16-u
        movdqa  xmm0, [esi+ecx]             ; Read from nearest preceding 16B boundary
        
R200:   ; Loop. ecx has negative index from the end, counting up to zero
        movdqa  xmm1, [esi+ecx+10H]    ; Read next two blocks aligned
        movdqa  xmm2, [esi+ecx+20H]
        movdqa  xmm3, xmm1             ; Copy because used twice
        pshufb  xmm0, xmm4             ; shift right
        pshufb  xmm1, xmm5             ; shift left
        por     xmm0, xmm1             ; combine blocks
        movdqa  [edi+ecx], xmm0        ; Save aligned
        movdqa  xmm0, xmm2             ; Save for next iteration
        pshufb  xmm3, xmm4             ; shift right
        pshufb  xmm2, xmm5             ; shift left
        por     xmm3, xmm2             ; combine blocks
        movdqa  [edi+ecx+10H], xmm3    ; Save aligned
        add     ecx, 20H               ; Loop through negative values up to zero
        jnz     R200
        
        ; Set up for edx remaining bytes
        add     esi, edx
        add     edi, edx
        neg     edx
        cmp     edx, -10H
        jg      R300
        ; One more 16-bytes block to move
        movdqa  xmm1, [esi+edx+10H]
        pshufb  xmm0, xmm4             ; shift right
        pshufb  xmm1, xmm5             ; shift left
        por     xmm0, xmm1             ; combine blocks
        movdqa  [edi+edx], xmm0        ; Save aligned
        add     edx, 10H
        
R300:   ; Get src pointer back to misaligned state
        add     esi, eax
        ; Move remaining 0 - 15 bytes, unaligned
        jmp     C200


2. Future AMD SSE5 instruction set
----------------------------------
The future AMD SSE5 instruction set allows a simpler implementation:

.data
PermMask label xmmword
        DB  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15
        DB 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31
        
.code
; Code for unaligned src, AMD SSE5 instruction set
; Move ecx + edx bytes of data
; Source is misaligned. (src-dest) modulo 16 = u
; eax = u
; esi = src - u = nearest preceding 16-bytes boundary
; edi = dest (aligned)
; ecx = - (count rounded down to nearest divisible by 32)
; edx = remaining bytes to move after loop

        movdqu  xmm4, [PermMask+eax]      ; Mask for permutation by u
        movdqa  xmm1, [esi+ecx]           ; Read from nearest preceding 16B boundary
        
P200:   ; Loop. ecx has negative index from the end, counting up to zero
        movdqa  xmm2, [esi+ecx+10H]       ; Read next block aligned
        pperm   xmm1, xmm1, xmm2, xmm4    ; Combine bytes from xmm1 and xmm2
        movdqa  [edi+ecx], xmm1           ; Save aligned
        movdqa  xmm1, [esi+ecx+20H]       ; Read next block aligned
        pperm   xmm2, xmm2, xmm1, xmm4    ; Combine bytes from xmm2 and xmm1
        movdqa  [edi+ecx], xmm2           ; Save aligned
        add     ecx, 20H                  ; Loop through negative values up to zero
        jnz     P200
        
        ; Set up for edx remaining bytes
        add     esi, edx
        add     edi, edx
        neg     edx
        cmp     edx, -10H
        jg      P300
        ; One more 16-bytes block to move
        movdqa  xmm2, [esi+edx+10H]       ; Read next block aligned
        pperm   xmm1, xmm1, xmm2, xmm4    ; Combine bytes from xmm1 and xmm2
        movdqa  [edi+edx], xmm1           ; Save aligned
        add     edx, 10H
        
P300:   ; Get src pointer back to misaligned state
        add     esi, eax
        ; Move remaining 0 - 15 bytes, unaligned
        jmp     C200


2. Future Intel AVX instruction set
-----------------------------------
Use the same method as above in the MOVE_UNALIGNED_SSSE3 macro.
The register-to-register moves can be eliminated by using a
3-register version of the palignr instruction.

It is possible to read and write 32 bytes (256 bits) at a time,
but there is no 256-bit version of palignr in the AVX instruction
set (hopefully in a later instruction set). Therefore, it is 
necessary to split the 256-bit YMM registers into two 128-bit
XMM registes with the vextractf128 instruction and join them 
together again with vinsertf128. If source and destination are 
aligned by 4 relative to each other, then the vpermil2ps 
instruction can be used in the same way as pperm in the SSE5
example above.

The code must end with vzeroupper in 64-bit Windows, vzeroall in
other systems.

# ; End of comment
