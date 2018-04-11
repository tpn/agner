;----------------------------------------------------------------------------
;                       fused_branch.inc                 2013-04-24 Agner Fog
;
;            PMC Test program for fusion of compare and branch instructions
;                           NASM syntax
;
; case:         1:  CMP + JZ/JNZ
;               2:  TEST + JZ/JNZ
;               3:  CMP + JA/JNA
;               4:  CMP + JG/JNG
;               5:  TEST + JS/JNS
;               6:  ADD + JZ/JNZ
;               7:  AND + JZ/JNZ
;               8:  DEC + JZ/JNZ
;               9:  DEC + JO/JNO
;              10:  CMP + JS/JNS
;              11:  SUB + JG/JNG
;              12:  SUB + JO/JNO
;              13:  ADC + JC/JNC
;              14:  OR  + JZ/JNZ
;              15:  NOT + JZ/JNZ
;              16:  SHR + JC/JNC
;              17:  JECXZ
;              18:  Boundary between cmp and jz
;              19:  Boundary in jump instruction

;
; optype:       Operand type for CMP or other arithmetic instruction:
;               Must be 1 if case > 3
;               1:  register, register
;               2:  register, immediate constant
;               3:  register, memory operand, rip relative
;               4:  register, memory operand with base pointer
;               5:  register, memory operand with base pointer and offset
;               6:  memory operand with base pointer, immediate
;               7:  memory operand with base pointer and offset, immediate
;
; taken:        0:  branch not taken
;               1:  branch taken
;
; (c) Copyright 2013 by Agner Fog. GNU General Public License www.gnu.org/licenses
;-----------------------------------------------------------------------------

%ifndef optype
   %define optype  1
%endif

%ifndef case
   %define case   1
%endif

%ifndef taken
   %define taken   0
%endif


;##############################################################################
;#
;#                 Test code macro
;#
;##############################################################################

; define long nops
%ifndef noptype
   %define noptype 2
%endif

%include "nops.inc"


%macro testinit1 0
   xor  ebp, ebp
   mov  edi, 1
%endmacro

%macro testinit3 0
   xor  eax, eax
   %if case == 18  ; Boundary crossing
      nop6
      nop6
   %elif case == 19  ; Boundary crossing
      nop5
      nop6
   %endif   
%endmacro

; main testcode macro
%macro testcode 0

%if case == 1
   %if optype == 1
       CMP eax, eax
   %elif optype == 2
       CMP eax, 0
   %elif optype == 3
       CMP eax, dword [UserData]
   %elif optype == 4
       CMP eax, dword [rsi]
   %elif optype == 5
       CMP eax, dword [rsi+100h]
   %elif optype == 6
       CMP dword [rsi], 0
   %elif optype == 7
       CMP dword [rsi+100h], 0
   %else
       %error unknown optype
   %endif
   %if taken == 0
       JNZ  $+3
   %else
       JZ   $+3
   %endif
%elif case == 2
   %if optype == 1
       TEST eax, eax
   %elif optype == 2
       TEST eax, 0
   %elif optype == 3
       TEST eax, dword [UserData]
   %elif optype == 4
       TEST eax, dword [rsi]
   %elif optype == 5
       TEST eax, dword [rsi+100h]
   %elif optype == 6
       TEST dword [rsi], 0
   %elif optype == 7
       TEST dword [rsi+100h], 0
   %else
       %error unknown optype
   %endif
   %if taken == 0
       JNZ  $+3
   %else
       JZ   $+3
   %endif
%elif case == 3
   %if optype == 1
       CMP eax, eax
   %elif optype == 2
       CMP eax, 0
   %elif optype == 3
       CMP eax, dword [UserData]
   %elif optype == 4
       CMP eax, dword [rsi]
   %elif optype == 5
       CMP eax, dword [rsi+100h]
   %elif optype == 6
       CMP dword [rsi], 0
   %elif optype == 7
       CMP dword [rsi+100h], 0
   %else
       %error unknown optype
   %endif
   %if taken == 0
       JA  $+3
   %else
       JNA   $+3
   %endif
%elif case == 4
   CMP eax, eax
   %if taken == 0
       JG  $+3
   %else
       JNG   $+3
   %endif
%elif case == 5
   TEST eax, eax
   %if taken == 0
       JS  $+3
   %else
       JNS   $+3
   %endif
%elif case == 6
   ADD eax, eax
   %if taken == 0
       JNZ  $+3
   %else
       JZ   $+3
   %endif
%elif case == 7
   AND eax, eax
   %if taken == 0
       JNZ  $+3
   %else
       JZ   $+3
   %endif
%elif case == 8
   DEC eax
   %if taken == 0
       JZ  $+3
   %else
       JNZ   $+3
   %endif
%elif case == 9
   DEC eax
   %if taken == 0
       JO  $+3
   %else
       JNO   $+3
   %endif
   
%elif case == 10  ; CMP + JS/JNS

   CMP eax, edi
   %if taken == 0
       JNS  $+3
   %else
       JS   $+3
   %endif   

%elif case == 11  ; SUB + JG/JNG

   SUB eax, ebp
   %if taken == 0
       JG  $+3
   %else
       JNG   $+3
   %endif   

%elif case == 12  ; SUB + JO/JNO

   SUB eax, ebp
   %if taken == 0
       JO  $+3
   %else
       JNO   $+3
   %endif   

%elif case == 13  ; ADC + JC/JNC

   ADC eax, ebp
   %if taken == 0
       JNC  $+3
   %else
       JC   $+3
   %endif   

%elif case == 14  ; OR  + JZ/JNZ

   OR eax, ebp
   %if taken == 0
       JNZ  $+3
   %else
       JZ   $+3
   %endif   

%elif case == 15  ; NOT + JZ/JNZ

   NOT eax
   %if taken == 0
       JZ  $+3
   %else
       JNZ   $+3
   %endif   

%elif case == 16  ; SHR + JC/JNC

   SHR eax,1
   %if taken == 0
       JNC  $+3
   %else
       JC   $+3
   %endif   
   
%elif case == 17  ; JECXZ

   %if taken == 0
       OR ecx,edi
       JECXZ   $+3
   %else
       AND ecx,ebp
       JECXZ   $+3
   %endif 
   
%elif case == 18  ; Boundary crossing

   CMP eax, eax
   %if taken == 0
       JNZ  $+6
   %else
       JZ   $+6
   %endif
   nop3
   
%elif case == 19  ; Boundary crossing

   CMP eax, eax
   %if taken == 0
       JNZ  $+6
   %else
       JZ   $+6
   %endif
   nop3
   
%else
   %error unknown case
%endif

   NOP

%endmacro ; testcode

; test loops
%define repeat1 100
%define repeat2 100

