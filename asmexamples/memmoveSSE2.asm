;*************************  memmoveSSE2.asm  *********************************
; Author:           Agner Fog
; Date created:     2008-07-09
; Last modified:    2008-07-09
; Syntax:           MASM/ML 6.x, 32 bit
; Operating system: Windows, Linux, BSD or Mac, 32-bit x86
; Instruction set:  SSE2 required, Suppl.SSE3 used if available
; Description:
; Standard memmove function:
; void *memmove(void *dest, const void *src, size_t count);
; Copy 'count' bytes from 'src' to 'dest'. Source and destination may overlap.
;
; Optimization:
; Uses XMM registers to copy 16 bytes at a time, aligned.
; If source and destination are misaligned relative to each other
; then the code will combine parts of every two consecutive 16-bytes 
; blocks from the source into one 16-bytes register which is written 
; to the destination, aligned.
; This method is 2 - 6 times faster than the implementations in the
; standard C libraries (MS, Intel, Gnu) when src or dest are misaligned.
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

public _memmove                        ; Function memcpy

; Imported from memcpySSE2.asm:
extern _NonTempMoveLimit: dword        ; Bypass cache if count > _NonTempMoveLimit
extern $memcpyEntry2: near             ; Entry from memmove


.code

; extern "C" void *memmove(void *dest, const void *src, size_t count);
; Function entry:
_memmove PROC    NEAR
        push    esi
        push    edi
        mov     edi, [esp+12]          ; dest
        mov     esi, [esp+16]          ; src
        mov     ecx, [esp+20]          ; count
        
        ; Check if dest overlaps src
        mov     eax, edi
        sub     eax, esi
        cmp     eax, ecx
        ; We can avoid testing for dest < src by using unsigned compare:
        ; Must move backwards if unsigned(dest-src) < count
        jae     $memcpyEntry2          ; Jump to memcpy if we can move forwards
        
        ; Must move backwards because of overlap between src and dest
        cmp     ecx, 40H
        jae     B100                    ; Use simpler code if count < 64
        
        ; count < 64. Move 32-16-8-4-2-1 bytes
        test    ecx, 20H
        jz      A100
        ; move 32 bytes
        ; movq is faster than movdqu on current processors,
        ; movdqu may be faster on future processors
        sub     ecx, 20H
        movq    xmm0, qword ptr [esi+ecx+18H]
        movq    xmm1, qword ptr [esi+ecx+10H]
        movq    xmm2, qword ptr [esi+ecx+8]
        movq    xmm3, qword ptr [esi+ecx]
        movq    qword ptr [edi+ecx+18H], xmm0
        movq    qword ptr [edi+ecx+10H], xmm1
        movq    qword ptr [edi+ecx+8], xmm2
        movq    qword ptr [edi+ecx], xmm3
A100:   test    ecx, 10H
        jz      A200
        ; move 16 bytes
        sub     ecx, 10H
        movq    xmm0, qword ptr [esi+ecx+8]
        movq    xmm1, qword ptr [esi+ecx]
        movq    qword ptr [edi+ecx+8], xmm0
        movq    qword ptr [edi+ecx], xmm1
A200:   test    ecx, 8
        jz      A300
        ; move 8 bytes
        sub     ecx, 8
        movq    xmm0, qword ptr [esi+ecx]
        movq    qword ptr [edi+ecx], xmm0
A300:   test    ecx, 4
        jz      A400
        ; move 4 bytes
        sub     ecx, 4
        mov     eax, [esi+ecx]
        mov     [edi+ecx], eax
        jz      A900                     ; early out if count divisible by 4
A400:   test    ecx, 2
        jz      A500
        ; move 2 bytes
        sub     ecx, 2
        movzx   eax, word ptr [esi+ecx]
        mov     [edi+ecx], ax
A500:   test    ecx, 1
        jz      A900
        ; move 1 byte
        movzx   eax, byte ptr [esi]
        mov     [edi], al
A900:   ; finished
        pop     edi
        pop     esi
        mov     eax, [esp+4]           ; Return value = dest
        ret
        
B100:   ; count >= 64
        ; Note: this part will not always work if count < 64
        ; Calculate size of last block after last regular boundary of dest
        lea     edx, [edi+ecx]         ; end of dext
        and     edx, 0FH
        jz      B300                   ; Skip if end of dest aligned by 16
        
        ; edx = size of last partial block, 1 - 15 bytes
        test    edx, 8
        jz      B200
        ; move 8 bytes
        sub     ecx, 8
        movq    xmm0, qword ptr [esi+ecx]
        movq    qword ptr [edi+ecx], xmm0
B200:   test    edx, 4
        jz      B210
        ; move 4 bytes
        sub     ecx, 4
        mov     eax, [esi+ecx]
        mov     [edi+ecx], eax
B210:   test    edx, 2
        jz      B220
        ; move 2 bytes
        sub     ecx, 2
        movzx   eax, word ptr [esi+ecx]
        mov     [edi+ecx], ax
B220:   test    edx, 1
        jz      B300
        ; move 1 byte
        dec     ecx
        movzx   eax, byte ptr [esi+ecx]
        mov     [edi+ecx], al
        
B300:   ; Now end of dest is aligned by 16. Any partial block has been moved        
        ; Find alignment of end of src modulo 16 at this point:
        lea     eax, [esi+ecx]
        and     eax, 0FH
        
        ; Set up for loop moving 32 bytes per iteration:
        mov     edx, ecx               ; Save count
        and     ecx, -20H              ; Round down to nearest multiple of 32
        sub     edx, ecx               ; Remaining data after loop
        sub     esi, eax               ; Nearest preceding aligned block of src
        ; Add the same to esi and edi as we have subtracted from ecx
        add     esi, edx
        add     edi, edx
        
        ; Check if count very big
        cmp     ecx, [$NonTempMoveLimit]
        ja      B500                   ; Use non-temporal store if count > $NonTempMoveLimit

        ; Dispatch to different codes depending on src alignment
        jmp     MAlignmentDispatch[eax*4]
        
B500:   ; Same, non-temporal moves to bypass cache
        jmp     MAlignmentDispatchNT[eax*4]
        

C100:   ; Code for aligned src. SSE2 or later instruction set
        ; The nice case, src and dest have same alignment.

        ; Loop. ecx has positive index from the beginning, counting down to zero
        sub     ecx, 20H
        movaps  xmm0, [esi+ecx+10H]
        movaps  xmm1, [esi+ecx]
        movaps  [edi+ecx+10H], xmm0
        movaps  [edi+ecx], xmm1
        jnz     C100
        
        ; Move the remaining edx bytes (0 - 31):
        ; move 16-8-4-2-1 bytes, aligned
        test    edx, edx
        jz      C500                   ; Early out if no more data
        test    edx, 10H
        jz      C200
        ; move 16 bytes
        sub     ecx, 10H
        movaps  xmm0, [esi+ecx]
        movaps  [edi+ecx], xmm0
C200:   ; Other branches come in here
        test    edx, edx
        jz      C500                   ; Early out if no more data
        test    edx, 8
        jz      C210        
        ; move 8 bytes
        sub     ecx, 8 
        movq    xmm0, qword ptr [esi+ecx]
        movq    qword ptr [edi+ecx], xmm0
C210:   test    edx, 4
        jz      C220        
        ; move 4 bytes
        sub     ecx, 4        
        mov     eax, [esi+ecx]
        mov     [edi+ecx], eax
        jz      C500                   ; Early out if count divisible by 4
C220:   test    edx, 2
        jz      C230        
        ; move 2 bytes
        sub     ecx, 2
        movzx   eax, word ptr [esi+ecx]
        mov     [edi+ecx], ax
C230:   test    edx, 1
        jz      C500        
        ; move 1 byte
        dec     ecx
        movzx   eax, byte ptr [esi+ecx]
        mov     [edi+ecx], al
C500:   ; finished     
        pop     edi
        pop     esi
        mov     eax, [esp+4]           ; Return value = dest
        ret

        
; Code for each src alignment, SSE2 instruction set:
; Make separate code for each alignment u because the shift instructions
; have the shift count as a constant:

MOVE_REVERSE_UNALIGNED_SSE2 MACRO u, nt
; Move ecx + edx bytes of data
; Source is misaligned. (src-dest) modulo 16 = u
; nt = 1 if non-temporal store desired
; eax = u
; esi = src - u = nearest preceding 16-bytes boundary
; edi = dest (aligned)
; ecx = count rounded down to nearest divisible by 32
; edx = remaining bytes to move after loop
LOCAL L1, L2
        movdqa  xmm0, [esi+ecx]        ; Read from nearest following 16B boundary        
L1:     ; Loop. ecx has positive index from the beginning, counting down to zero
        sub     ecx, 20H
        movdqa  xmm1, [esi+ecx+10H]    ; Read next two blocks aligned
        movdqa  xmm2, [esi+ecx]
        movdqa  xmm3, xmm1             ; Copy because used twice
        pslldq  xmm0, 16-u             ; shift left
        psrldq  xmm1, u                ; shift right
        por     xmm0, xmm1             ; combine blocks
        IF nt eq 0
        movdqa  [edi+ecx+10H], xmm0    ; Save aligned
        ELSE
        movntdq [edi+ecx+10H], xmm0    ; Save aligned
        ENDIF
        movdqa  xmm0, xmm2             ; Save for next iteration
        pslldq  xmm3, 16-u             ; shift left
        psrldq  xmm2, u                ; shift right
        por     xmm3, xmm2             ; combine blocks
        IF nt eq 0
        movdqa  [edi+ecx], xmm3        ; Save aligned
        ELSE
        movntdq [edi+ecx], xmm3        ; Save aligned
        ENDIF
        jnz     L1
                
        ; Move edx remaining bytes
        test    edx, 10H
        jz      L2
        ; One more 16-bytes block to move
        sub     ecx, 10H
        movdqa  xmm1, [esi+ecx]
        pslldq  xmm0, 16-u             ; shift left
        psrldq  xmm1, u                ; shift right
        por     xmm0, xmm1             ; combine blocks
        IF nt eq 0
        movdqa  [edi+ecx], xmm0        ; Save aligned
        ELSE
        movntdq [edi+ecx], xmm0        ; Save aligned
        ENDIF        
L2:     ; Get src pointer back to misaligned state
        add     esi, eax
        ; Move remaining 0 - 15 bytes, unaligned
        jmp     C200
ENDM       

MOVE_REVERSE_UNALIGNED_SSE2_4 MACRO nt
; Special case: u = 4
LOCAL L1, L2
        movdqa  xmm0, [esi+ecx]        ; Read from nearest following 16B boundary
L1:     ; Loop. ecx has positive index from the beginning, counting down to zero
        sub     ecx, 20H
        movdqa  xmm1, [esi+ecx+10H]    ; Read next two blocks aligned
        movdqa  xmm2, [esi+ecx]
        movdqa  xmm3, xmm0
        movdqa  xmm0, xmm2        
        movss   xmm2, xmm1
        pshufd  xmm2, xmm2, 00111001B  ; Rotate right
        movss   xmm1, xmm3
        pshufd  xmm1, xmm1, 00111001B  ; Rotate right
        IF nt eq 0
        movdqa  [edi+ecx+10H], xmm1    ; Save aligned
        movdqa  [edi+ecx], xmm2        ; Save aligned
        ELSE
        movntdq [edi+ecx+10H], xmm1    ; Non-temporal save
        movntdq [edi+ecx], xmm2        ; Non-temporal save
        ENDIF
        jnz     L1
                
        ; Move edx remaining bytes
        test    edx, 10H
        jz      L2
        ; One more 16-bytes block to move
        sub     ecx, 10H
        movdqa  xmm1, [esi+ecx]
        movss   xmm1, xmm0
        pshufd  xmm1, xmm1, 00111001B  ; Rotate right
        IF nt eq 0
        movdqa  [edi+ecx], xmm1        ; Save aligned
        ELSE
        movntdq [edi+ecx], xmm1        ; Non-temporal save
        ENDIF        
L2:     ; Get src pointer back to misaligned state
        add     esi, eax
        ; Move remaining 0 - 15 bytes, unaligned
        jmp     C200
ENDM       

MOVE_REVERSE_UNALIGNED_SSE2_8 MACRO nt
; Special case: u = 8
LOCAL L1, L2
        movdqa  xmm0, [esi+ecx]        ; Read from nearest following 16B boundary
        shufps  xmm0, xmm0, 01001110B  ; Rotate
L1:     ; Loop. ecx has positive index from the beginning, counting down to zero
        sub     ecx, 20H
        movdqa  xmm1, [esi+ecx+10H]    ; Read next two blocks aligned
        shufps  xmm1, xmm1, 01001110B  ; Rotate
        movsd   xmm0, xmm1
        IF nt eq 0
        movdqa  [edi+ecx+10H], xmm0    ; Save aligned
        ELSE
        movntdq [edi+ecx+10H], xmm0    ; Non-temporal save
        ENDIF
        movdqa  xmm0, [esi+ecx]
        shufps  xmm0, xmm0, 01001110B  ; Rotate
        movsd   xmm1, xmm0
        IF nt eq 0
        movdqa  [edi+ecx], xmm1        ; Save aligned
        ELSE
        movntdq [edi+ecx], xmm1        ; Non-temporal save
        ENDIF
        jnz     L1
                
        ; Move edx remaining bytes
        test    edx, 10H
        jz      L2
        ; One more 16-bytes block to move
        sub     ecx, 10H
        movdqa  xmm1, [esi+ecx]
        shufps  xmm1, xmm1, 01001110B  ; Rotate 
        movsd   xmm0, xmm1
        IF nt eq 0
        movdqa  [edi+ecx], xmm0        ; Save aligned
        ELSE
        movntdq [edi+ecx], xmm0        ; Non-temporal save
        ENDIF        
L2:     ; Get src pointer back to misaligned state
        add     esi, eax
        ; Move remaining 0 - 15 bytes, unaligned
        jmp     C200
ENDM       

MOVE_REVERSE_UNALIGNED_SSE2_12 MACRO nt
; Special case: u = 12
LOCAL L1, L2
        movdqa  xmm0, [esi+ecx]        ; Read from nearest following 16B boundary
        pshufd  xmm0, xmm0, 10010011B  ; Rotate right
L1:     ; Loop. ecx has positive index from the beginning, counting down to zero
        sub     ecx, 20H
        movdqa  xmm1, [esi+ecx+10H]    ; Read next two blocks aligned
        pshufd  xmm1, xmm1, 10010011B  ; Rotate left
        movss   xmm0, xmm1
        IF nt eq 0
        movdqa  [edi+ecx+10H], xmm0    ; Save aligned
        ELSE
        movntdq [edi+ecx+10H], xmm0    ; Non-temporal save
        ENDIF
        movdqa  xmm0, [esi+ecx]
        pshufd  xmm0, xmm0, 10010011B  ; Rotate left
        movss   xmm1, xmm0
        IF nt eq 0
        movdqa  [edi+ecx], xmm1        ; Save aligned
        ELSE
        movntdq [edi+ecx], xmm1        ; Non-temporal save
        ENDIF
        jnz     L1
                
        ; Move edx remaining bytes
        test    edx, 10H
        jz      L2
        ; One more 16-bytes block to move
        sub     ecx, 10H
        movdqa  xmm1, [esi+ecx]
        pshufd  xmm1, xmm1, 10010011B  ; Rotate left
        movss   xmm0, xmm1
        IF nt eq 0
        movdqa  [edi+ecx], xmm0        ; Save aligned
        ELSE
        movntdq [edi+ecx], xmm0        ; Non-temporal save
        ENDIF        
L2:     ; Get src pointer back to misaligned state
        add     esi, eax
        ; Move remaining 0 - 15 bytes, unaligned
        jmp     C200
ENDM       

; Code for each src alignment, Suppl.SSE3 instruction set:
; Code for unaligned src, Suppl.SSE3 instruction set.
; Make separate code for each alignment u because the palignr instruction
; has the shift count as a constant:

MOVE_REVERSE_UNALIGNED_SSSE3 MACRO u
; Move ecx + edx bytes of data
; Source is misaligned. (src-dest) modulo 16 = u
; eax = u
; esi = src - u = nearest preceding 16-bytes boundary
; edi = dest (aligned)
; ecx = - (count rounded down to nearest divisible by 32)
; edx = remaining bytes to move after loop
LOCAL L1, L2
        movdqa  xmm0, [esi+ecx]        ; Read from nearest following 16B boundary
        
L1:     ; Loop. ecx has positive index from the beginning, counting down to zero
        sub     ecx, 20H
        movdqa  xmm1, [esi+ecx+10H]    ; Read next two blocks        
        palignr xmm0, xmm1, u          ; Combine parts into aligned block
        movdqa  [edi+ecx+10H], xmm0    ; Save aligned
        movdqa  xmm0, [esi+ecx]
        palignr xmm1, xmm0, u          ; Combine parts into aligned block
        movdqa  [edi+ecx], xmm1        ; Save aligned
        jnz     L1
        
        ; Set up for edx remaining bytes
        test    edx, 10H
        jz      L2
        ; One more 16-bytes block to move
        sub     ecx, 10H
        movdqa  xmm1, [esi+ecx]        ; Read next two blocks        
        palignr xmm0, xmm1, u          ; Combine parts into aligned block
        movdqa  [edi+ecx], xmm0        ; Save aligned
        
L2:     ; Get src pointer back to misaligned state
        add     esi, eax
        ; Move remaining 0 - 15 bytes
        jmp     C200
ENDM        

; Make 15 instances of SSE2 macro for each value of the alignment u.
; These are pointed to by the jump table MAlignmentDispatchSSE2 below

D101:   MOVE_REVERSE_UNALIGNED_SSE2 1,   0
D102:   MOVE_REVERSE_UNALIGNED_SSE2 2,   0
D103:   MOVE_REVERSE_UNALIGNED_SSE2 3,   0
D104:   MOVE_REVERSE_UNALIGNED_SSE2_4    0
D105:   MOVE_REVERSE_UNALIGNED_SSE2 5,   0
D106:   MOVE_REVERSE_UNALIGNED_SSE2 6,   0
D107:   MOVE_REVERSE_UNALIGNED_SSE2 7,   0
D108:   MOVE_REVERSE_UNALIGNED_SSE2_8    0
D109:   MOVE_REVERSE_UNALIGNED_SSE2 9,   0
D10A:   MOVE_REVERSE_UNALIGNED_SSE2 0AH, 0
D10B:   MOVE_REVERSE_UNALIGNED_SSE2 0BH, 0
D10C:   MOVE_REVERSE_UNALIGNED_SSE2_12   0
D10D:   MOVE_REVERSE_UNALIGNED_SSE2 0DH, 0
D10E:   MOVE_REVERSE_UNALIGNED_SSE2 0EH, 0
D10F:   MOVE_REVERSE_UNALIGNED_SSE2 0FH, 0

; Make 15 instances of Sup.SSE3 macro for each value of the alignment u.
; These are pointed to by the jump table MAlignmentDispatchSupSSE3 below

E101:   MOVE_REVERSE_UNALIGNED_SSSE3 1
E102:   MOVE_REVERSE_UNALIGNED_SSSE3 2
E103:   MOVE_REVERSE_UNALIGNED_SSSE3 3
E104:   MOVE_REVERSE_UNALIGNED_SSSE3 4
E105:   MOVE_REVERSE_UNALIGNED_SSSE3 5
E106:   MOVE_REVERSE_UNALIGNED_SSSE3 6
E107:   MOVE_REVERSE_UNALIGNED_SSSE3 7
E108:   MOVE_REVERSE_UNALIGNED_SSSE3 8
E109:   MOVE_REVERSE_UNALIGNED_SSSE3 9
E10A:   MOVE_REVERSE_UNALIGNED_SSSE3 0AH
E10B:   MOVE_REVERSE_UNALIGNED_SSSE3 0BH
E10C:   MOVE_REVERSE_UNALIGNED_SSSE3 0CH
E10D:   MOVE_REVERSE_UNALIGNED_SSSE3 0DH
E10E:   MOVE_REVERSE_UNALIGNED_SSSE3 0EH
E10F:   MOVE_REVERSE_UNALIGNED_SSSE3 0FH

        
F100:   ; Non-temporal move, src and dest have same alignment.
        ; Loop. ecx has positive index from the beginning, counting down to zero
        sub     ecx, 20H
        movaps  xmm0, [esi+ecx+10H]
        movaps  xmm1, [esi+ecx]
        movntps [edi+ecx+10H], xmm0
        movntps [edi+ecx], xmm1
        jnz     F100
        
        ; Move the remaining edx bytes (0 - 31):
        ; move 16-8-4-2-1 bytes, aligned
        test    edx, 10H
        jz      C200
        ; move 16 bytes
        sub     ecx, 10H
        movaps  xmm0, [esi+ecx]
        movntps  [edi+ecx], xmm0
        ; move the remaining 0 - 15 bytes
        jmp     C200

; Non-temporal move, src and dest have different alignment.
; Make 15 instances of SSE2 macro for each value of the alignment u.
; These are pointed to by the jump table MAlignmentDispatchNT below

F101:   MOVE_REVERSE_UNALIGNED_SSE2 1,   1
F102:   MOVE_REVERSE_UNALIGNED_SSE2 2,   1
F103:   MOVE_REVERSE_UNALIGNED_SSE2 3,   1
F104:   MOVE_REVERSE_UNALIGNED_SSE2_4    1
F105:   MOVE_REVERSE_UNALIGNED_SSE2 5,   1
F106:   MOVE_REVERSE_UNALIGNED_SSE2 6,   1
F107:   MOVE_REVERSE_UNALIGNED_SSE2 7,   1
F108:   MOVE_REVERSE_UNALIGNED_SSE2_8    1
F109:   MOVE_REVERSE_UNALIGNED_SSE2 9,   1
F10A:   MOVE_REVERSE_UNALIGNED_SSE2 0AH, 1
F10B:   MOVE_REVERSE_UNALIGNED_SSE2 0BH, 1
F10C:   MOVE_REVERSE_UNALIGNED_SSE2_12   1
F10D:   MOVE_REVERSE_UNALIGNED_SSE2 0DH, 1
F10E:   MOVE_REVERSE_UNALIGNED_SSE2 0EH, 1
F10F:   MOVE_REVERSE_UNALIGNED_SSE2 0FH, 1

        
Q100:   ; CPU dispatcher, check for SupSSE3 instruction set
        ; This part is executed only once, optimized for size
        pushad                         ; All registers must be saved
        mov     eax, 1
        cpuid                          ; Get feature flags
        mov     esi, offset MAlignmentDispatchSSE2
        bt      ecx, 9                 ; Test bit for SupSSE3
        jnc     Q200
        mov     esi, offset MAlignmentDispatchSupSSE3
Q200:   ; Insert appropriate table
        mov     edi, offset MAlignmentDispatch
        mov     ecx, 16
        rep     movsd
        popad
        ; Jump according to the replaced table
        jmp     MAlignmentDispatch[eax*4]


; Data segment must be included in function namespace
.data

; Jump table for alignments 0 - 15:
; This table initially points to a CPU dispatcher
; The CPU dispatcher replaces the table with one of the
; tables below, according to the available instruction set:

MAlignmentDispatch label dword
DD Q100, Q100, Q100, Q100, Q100, Q100, Q100, Q100
DD Q100, Q100, Q100, Q100, Q100, Q100, Q100, Q100

MAlignmentDispatchSSE2 label dword
DD C100, D101, D102, D103, D104, D105, D106, D107
DD D108, D109, D10A, D10B, D10C, D10D, D10E, D10F

MAlignmentDispatchSupSSE3 label dword
DD C100, E101, E102, E103, E104, E105, E106, E107
DD E108, E109, E10A, E10B, E10C, E10D, E10E, E10F

MAlignmentDispatchNT label dword
DD F100, F101, F102, F103, F104, F105, F106, F107
DD F108, F109, F10A, F10B, F10C, F10D, F10E, F10F

.code

_memmove ENDP                           ; End of function namespace

END
