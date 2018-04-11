;*************************  memsetSSE2.asm  **********************************
; Author:           Agner Fog
; Date created:     2008-07-16
; Last modified:    2008-07-16
; Syntax:           MASM/ML 6.x, 32 bit
; Operating system: Windows, Linux, BSD or Mac, 32-bit x86
; Instruction set:  SSE2
; Description:
; Standard memset function:
; void *memset(void *dest, int c, size_t count);
; Sets 'count' bytes from 'dest' to the value 'c'
;
; Optimization:
; Uses XMM registers to set 16 bytes at a time, aligned.
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

public _memset                         ; Function memset

; Imported from memcpySSE2.asm:
extern _CacheBypassLimit: dword        ; Bypass cache if count > _CacheBypassLimit

.code

; extern "C" void * memset(void *dest, int c, size_t count);
; Function entry:
_memset PROC    NEAR
        mov     edx, [esp+4]           ; dest
        movzx   eax, byte ptr [esp+8]  ; c
        mov     ecx, [esp+12]          ; count
        imul    eax, 01010101H         ; Broadcast c into all bytes of eax
        ; (multiplication is slow on Pentium 4)
        cmp     ecx, 16
        ja      M100
        jmp     MemsetJTab[ecx*4]
        
; Separate code for each count from 0 to 16:
M16:    mov     [edx+12], eax
M12:    mov     [edx+8],  eax
M08:    mov     [edx+4],  eax
M04:    mov     [edx],    eax
M00:    mov     eax, [esp+4]           ; dest
        ret

M15:    mov     [edx+11], eax
M11:    mov     [edx+7],  eax
M07:    mov     [edx+3],  eax
M03:    mov     [edx+1],  ax
M01:    mov     [edx],    al
        mov     eax, [esp+4]           ; dest
        ret
       
M14:    mov     [edx+10], eax
M10:    mov     [edx+6],  eax
M06:    mov     [edx+2],  eax
M02:    mov     [edx],    ax
        mov     eax, [esp+4]           ; dest
        ret

M13:    mov     [edx+9],  eax
M09:    mov     [edx+5],  eax
M05:    mov     [edx+1],  eax
        mov     [edx],    al
        mov     eax, [esp+4]           ; dest
        ret
        
.data
; Jump table for count from 0 to 16:
MemsetJTab DD M00, M01, M02, M03, M04, M05, M06, M07
           DD M08, M09, M10, M11, M12, M13, M14, M15, M16
           
.code

M100:   ; count > 16. Use SSE2 instruction set
        movd    xmm0, eax
        pshufd  xmm0, xmm0, 0          ; Broadcast c into all bytes of xmm0
        
        ; Store the first unaligned part.
        ; The size of this part is 1 - 16 bytes.
        ; It is faster to always write 16 bytes, possibly overlapping
        ; with the subsequent regular part, than to make possibly mispredicted
        ; branches depending on the size of the first part.
        movq    qword ptr [edx],   xmm0
        movq    qword ptr [edx+8], xmm0
        
        ; Check if count very big
        cmp     ecx, [_CacheBypassLimit]
        ja      M500                   ; Use non-temporal store if count > _CacheBypassLimit
        
        ; End of regular part:
        ; Round down dest+count to nearest preceding 16-bytes boundary
        lea     ecx, [edx+ecx-1]
        and     ecx, -10H
        
        ; Start of regular part:
        ; Round up dest to next 16-bytes boundary
        add     edx, 10H
        and     edx, -10H
        
        ; -(size of regular part)
        sub     edx, ecx
        jnl     M300                   ; Jump if not negative
        
M200:   ; Loop through regular part
        ; ecx = end of regular part
        ; edx = negative index from the end, counting up to zero
        movdqa  [ecx+edx], xmm0
        add     edx, 10H
        jnz     M200
        
M300:   ; Do the last irregular part
        ; The size of this part is 1 - 16 bytes.
        ; It is faster to always write 16 bytes, possibly overlapping
        ; with the preceding regular part, than to make possibly mispredicted
        ; branches depending on the size of the last part.
        mov     eax, [esp+4]           ; dest
        mov     ecx, [esp+12]          ; count
        movq    qword ptr [eax+ecx-10H], xmm0
        movq    qword ptr [eax+ecx-8], xmm0
        ret
   
M500:   ; Use non-temporal moves, same code as above:
        ; End of regular part:
        ; Round down dest+count to nearest preceding 16-bytes boundary
        lea     ecx, [edx+ecx-1]
        and     ecx, -10H
        
        ; Start of regular part:
        ; Round up dest to next 16-bytes boundary
        add     edx, 10H
        and     edx, -10H
        
        ; -(size of regular part)
        sub     edx, ecx
        jnl     M700                   ; Jump if not negative
        
M600:   ; Loop through regular part
        ; ecx = end of regular part
        ; edx = negative index from the end, counting up to zero
        movdqu  [ecx+edx], xmm0
        add     edx, 10H
        jnz     M600
        
M700:   ; Do the last irregular part
        ; The size of this part is 1 - 16 bytes.
        ; It is faster to always write 16 bytes, possibly overlapping
        ; with the preceding regular part, than to make possibly mispredicted
        ; branches depending on the size of the last part.
        mov     eax, [esp+4]           ; dest
        mov     ecx, [esp+12]          ; count
        movq    qword ptr [eax+ecx-10H], xmm0
        movq    qword ptr [eax+ecx-8], xmm0
        ret
        
_memset endp

END