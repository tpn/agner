; denormal.inc                                                                 2012-02-21 AgF
;
; Run PMCTest for floating point instructions with various combinations of denormal operands
; (c) 2012 by Agner Fog. GNU General Public License www.gnu.org/licenses

; Parameters:
;
; tcase: test case (0 - 20)
;
; flushtoz:   1 for enabling flush-to-zero mode
;
; denormalsz: 1 for enabling denormals-are-zero mode

; enable test loops
%define repeat1 100
%define repeat2 100

%macro testdata 0  ; define test cases

        DD   1.0,       1.0           ; 0:  normal + normal = normal
        DD   00803000h, 80802000h     ; 1:  normal + normal = denormal
        DD   1.0,       00001000h     ; 2:  normal + denormal = normal
        DD   00800000h, 80600000h     ; 3:  normal + denormal = denormal
        DD   00700000h, 007f0000h     ; 4:  denormal + denormal = normal
        DD   0000000fh, 0000000fh     ; 5:  denormal + denormal = denormal
        DD   7f7fff00h, 7f7fff01h     ; 6:  normal + normal = overflow
        DD   1.0,       7f800001h     ; 7:  normal + NAN = NAN
        DD   1.0,       7f800000h     ; 8:  normal + INF = INF
        DD   7f800000h, 7f800000h     ; 9:  INF + INF = INF
        DD   7f800000h, 0ff800000h    ; 10: INF - INF = NAN

        DD   1.0,       1.0           ; 11: normal * normal = normal
        DD   1.0E-30,   1.0E-10       ; 12: normal * normal = denormal
        DD   1.0E+30,   00300000h     ; 13: normal * denormal = normal
        DD   2.0,       00100000h     ; 14: normal * denormal = denormal
        DD   00300000h, 00300000h     ; 15: denormal * denormal = underflow
        DD   1.0E-30,   1.0E-30       ; 16: normal * normal = underflow
        DD   1.0E+30,   1.0E+20       ; 17: normal * normal = overflow
        DD   1.0,       7f800001h     ; 18: normal * NAN = NAN
        DD   1.0,       7f800000h     ; 19: normal * INF = INF
        DD   0.0,       7f800000h     ; 20: 0 * INF = NAN

        times 100H DD  0

%endmacro

%ifndef tcase
   %define tcase 1
%endif

%ifndef flushtoz
   %define flushtoz 0
%endif

%ifndef denormalsz
   %define denormalsz 0
%endif

%macro testinit1 0  ;  initialize

   mov  eax, tcase
   cmp  eax, 50
   jb   TCASEOK
   xor  eax, eax
   TCASEOK:
   lea rdi, [UserData+8*rax]     ; point rdi to selected test case
   movsd xmm1, [rdi]             ; load operands
   movsd xmm2, [rdi+4]
   lea rbx, [UserData+100H]  
   stmxcsr [rbx]
   mov eax, [rbx]
   %if flushtoz > 0
      bts eax,15                  ; set flush to zero mode
   %else
      btr eax,15
   %endif
   %if denormalsz > 0
      bts eax,6                   ; set denormals are zero mode
   %else
      btr eax,6
   %endif
   mov [rbx], eax
   ldmxcsr [rbx]

%endmacro

; main testcode macro
%macro testcode 0


   %if tcase < 11      ; addition

      movss xmm0, xmm1
      addss xmm0, xmm2

   %else 

      movss xmm0, xmm1
      mulss xmm0, xmm2

   %endif

%endmacro

