;----------------------------------------------------------------------------
;                warmup_fp.inc                2013-07-10 Agner Fog
;
; PMC Test program for testing warm up effect of floating point unit
;
; Constants to be defined:
; 
; tcase:   1: f.p. addition
;          2: f.p. multiplication
; 
; (c) Copyright 2013 by Agner Fog. GNU General Public License www.gnu.org/licenses
;-----------------------------------------------------------------------------
; Define any undefined macros

%ifndef tcase
   %define tcase 1
%endif

%define WARMUPCOUNT 0    ; don't use warm up in TemplateB64.nasm

; Let f.p. unit cool down by using integer unit
; (This is actually the same as in TemplateB64.nasm, but that may change:)
%macro testinit1 0
        mov ecx, 100000000
        mov eax, 1
        align 16
Wuloop1:
        %rep 10
        imul eax, ecx
        %endrep
        dec ecx
        jnz Wuloop1
%endmacro

; low inner repeat counts to make effect visible
%define repeat0 20
%define repeat1 10
%define repeat2 10


; Define test cases

%if tcase == 1   ; f.p. addition

   %macro testcode 0
      vaddpd ymm1,ymm1,ymm2
   %endmacro

%elif tcase == 2   ; f.p. multiplication

   %macro testcode 0
      vmulps ymm1,ymm1,ymm2
   %endmacro

%else
   %error unknown test case tcase
%endif
