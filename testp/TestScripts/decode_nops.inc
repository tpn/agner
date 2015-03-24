;----------------------------------------------------------------------------
;                       nops.inc                            2012-01-26 Agner Fog
;
;                PMC Test program for different NOP instructions
;                           YASM syntax
;
; The following macros can be defined on the command line or in include files:
; 
; noptype:      1: pseudo-NOPs such as LEA eax,[eax+0]
;               2: long NOPs (0F 1F ...)
;               3: 66 NOPs (simple NOP with up to 14 operand size prefixes)
;               4: long NOPs up to 11, then other instructions with max 3 prefixes up to 14 (for processors that have penalties for > 3 prefixes)
;               5: seg NOPs (simple NOP with up to 14 segment prefixes)
; 
; nopsize:      Length of each NOP in bytes, 1 - 15
; 
; ncount:       Number of consequtive NOPs inside a loop
;
; loopcount:    Number of repetitions of loop
;
; nthreads:     Number of simultaneous threads (default = 1)
; 
; counters:     A comma-separated list of PMC counter numbers 
;               (referring to CounterDefinitions in PMCTestA.cpp)
;
; (c) Copyright 2012 by Agner Fog. GNU General Public License www.gnu.org/licenses
;-----------------------------------------------------------------------------
; Define any undefined macros

%ifndef noptype
   %define noptype 2
%endif

%ifndef nopsize
   %define nopsize 1
%endif

%ifndef ncount
   %define ncount 100
%endif

%ifndef loopcount
   %define loopcount  1000
%endif

%ifndef nthreads
   %define nthreads 1
%endif

; define nops of all lengths

%if noptype == 1 || noptype == 2 || noptype == 3

   %include "nops.inc"
   
%elif noptype == 4         ; long NOPs up to 11, then other instructions with max 3 prefixes up to 14 
                           ; (for processors that have penalties for > 3 prefixes)
   %define nop1  db 90H
   %define nop2  db 66H, 90H
   %define nop3  db 0FH, 1FH, 0C0H
   %define nop4  db 0FH, 1FH, 40H, 00H
   %define nop5  db 0FH, 1FH, 44H, 00H, 00H
   %define nop6  db 66H, 0FH, 1FH, 44H, 00H, 00H
   %define nop7  db 0FH, 1FH, 80H, 00H, 00H, 00H, 00H
   %define nop8  db 0FH, 1FH, 84H, 00H, 00H, 00H, 00H, 00H
   %define nop9  db 66H, 0FH, 1FH, 84H, 00H, 00H, 00H, 00H, 00H
   %define nop10 db 66H, 66H, 0FH, 1FH, 84H, 00H, 00H, 00H, 00H, 00H
   %define nop11 db 66H, 66H, 66H, 0FH, 1FH, 84H, 00H, 00H, 00H, 00H, 00H
   %macro nop12 0
      db 26h, 26h
      mov rax, 123456789abcdef0h
   %endmacro
   %macro nop13 0
      db 26h, 26h
      cmp dword [rsi+rbp*2+100h],12345678h
   %endmacro
   %macro nop14 0
      db 26h, 26h, 26h
      cmp dword [rsi+rbp*2+100h],12345678h
   %endmacro
   %macro nop15 0
      db 26h, 26h, 26h, 26h
      cmp dword [rsi+rbp*2+100h],12345678h
   %endmacro

%elif noptype == 5              ; simple NOP with up to 14 segment prefixes
   %define nop1  db 90H
   %define nop2  db 26h, 90H
   %define nop3  db 26h, 26h, 90H
   %define nop4  db 26h, 26h, 26h, 90H
   %define nop5  db 26h, 26h, 26h, 26h, 90H
   %define nop6  db 26h, 26h, 26h, 26h, 26h, 90H
   %define nop7  db 26h, 26h, 26h, 26h, 26h, 26h, 90H
   %define nop8  db 26h, 26h, 26h, 26h, 26h, 26h, 26h, 90H
   %define nop9  db 26h, 26h, 26h, 26h, 26h, 26h, 26h, 26h, 90H
   %define nop10 db 26h, 26h, 26h, 26h, 26h, 26h, 26h, 26h, 26h, 90H
   %define nop11 db 26h, 26h, 26h, 26h, 26h, 26h, 26h, 26h, 26h, 26h, 90H
   %define nop12 db 26h, 26h, 26h, 26h, 26h, 26h, 26h, 26h, 26h, 26h, 26h, 90H
   %define nop13 db 26h, 26h, 26h, 26h, 26h, 26h, 26h, 26h, 26h, 26h, 26h, 26h, 90H
   %define nop14 db 26h, 26h, 26h, 26h, 26h, 26h, 26h, 26h, 26h, 26h, 26h, 26h, 26h, 90H
   %define nop15 db 26h, 26h, 26h, 26h, 26h, 26h, 26h, 26h, 26h, 26h, 26h, 26h, 26h, 26H, 90H
%else
   %error unknown noptype
%endif



;##############################################################################
;#
;#                 Test code macro
;#
;##############################################################################

; main testcode macro
%macro testcode 0

mov ebx, loopcount
align 16

LOOPSTART1:

   %rep ncount

      %if nopsize == 1
         nop1
      %elif nopsize == 2
         nop2
      %elif nopsize == 3
         nop3
      %elif nopsize == 4
         nop4
      %elif nopsize == 5
         nop5
      %elif nopsize == 6
         nop6
      %elif nopsize == 7
         nop7
      %elif nopsize == 8
         nop8
      %elif nopsize == 9
         nop9
      %elif nopsize == 10
         nop10
      %elif nopsize == 11
         nop11
      %elif nopsize == 12
         nop12
      %elif nopsize == 13
         nop13
      %elif nopsize == 14
         nop14
      %elif nopsize == 15
         nop15
      %else
         %error unsupported nopsize
      %endif

   %endrep

dec ebx
jnz LOOPSTART1

%endmacro

; disable default test loops
%define repeat1 1
%define repeat2 1

