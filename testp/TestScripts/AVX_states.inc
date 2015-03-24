;----------------------------------------------------------------------------
;                AVX_states.inc                          2013-07-09 Agner Fog
;
; PMC Test program for transition between the three AVX states.
; There are three AVX states:
; Clean:    The upper half of all ymm registers is zero and unused
; Modified: The upper half of at least one ymm register has been modified
; Saved:    The upper half of all ymm registers is saved while executing non-AVX 128-bit instructions
;
; Constants to be defined:
; 
; tcase:   1: Transition clean to modified
;          2: Transition modified to saved. (AVX -> non-AVX)
;          3: Transition saved to modified. (non-AVX -> AVX)
;          4: Transition modified to clean. (vzeroupper)
;          5: Transition saved to clean. (vzeroupper)
;          6: 100 transitions modified to saved to modified, same register. (Alternating AVX <-> non-AVX)
;          7: 100 transitions modified to saved to modified, different register. (Alternating AVX <-> non-AVX)
;          8: 100 transitions 128-bit AVX <-> 128-bit non-AVX
;          9: 100 transitions 256-bit AVX read only <-> 128-bit non-AVX
; 
; nthreads:     Number of simultaneous threads (default = 1)
; 
; counters:     A comma-separated list of PMC counter numbers 
;               (referring to CounterDefinitions in PMCTestA.cpp)
;
; (c) Copyright 2013 by Agner Fog. GNU General Public License www.gnu.org/licenses
;-----------------------------------------------------------------------------
; Define any undefined macros

%ifndef tcase
   %define tcase 7
%endif

; Define test cases

%if tcase == 1   ; Transition clean to modified

   %macro testinitc 0
      vzeroupper
   %endmacro

   %macro testcode 0
      vorps ymm1,ymm1
   %endmacro

%elif tcase == 2   ; Transition modified to saved. (AVX -> non-AVX)

   %macro testinitc 0
      vorps ymm1,ymm2
   %endmacro

   %macro testcode 0
      por xmm3,xmm4
   %endmacro

%elif tcase == 3   ; Transition saved to modified. (non-AVX -> AVX)

   %macro testinitc 0
      por xmm3,xmm4
   %endmacro

   %macro testcode 0
      vorps ymm1,ymm2
   %endmacro

%elif tcase == 4   ; Transition modified to clean. (vzeroupper)

   %macro testinitc 0
      vorps ymm1,ymm2
   %endmacro

   %macro testcode 0
      vzeroupper
   %endmacro

%elif tcase == 5   ; Transition saved to clean. (vzeroupper)

   %macro testinitc 0
      vorps ymm1,ymm2
      por xmm3,xmm4
   %endmacro

   %macro testcode 0
      vzeroupper
   %endmacro

%elif tcase == 6   ; Transition modified to saved to modified, same register. (Alternating AVX <-> non-AVX)

   %macro testinitc 0
      vzeroupper
      vorps ymm1,ymm2
   %endmacro

   %macro testcode 0
      %rep 100        ; repeat 100 times
         por xmm1,xmm2
         vorps ymm1,ymm2
      %endrep
   %endmacro

%elif tcase == 7   ; Transition modified to saved to modified, different register. (Alternating AVX <-> non-AVX)

   %macro testinitc 0
      vzeroupper
      vorps ymm1,ymm2
   %endmacro

   %macro testcode 0
      %rep 100        ; repeat 100 times
         por xmm3,xmm4
         vorps ymm5,ymm6
      %endrep
   %endmacro

%elif tcase == 8   ; 100 transitions 128-bit AVX <-> 128-bit non-AVX

   %macro testinitc 0
      vzeroupper
      vorps xmm1,xmm2
   %endmacro

   %macro testcode 0
      %rep 100        ; repeat 100 times
         por xmm3,xmm4
         vorps xmm1,xmm2
      %endrep
   %endmacro

%elif tcase == 9   ; 100 transitions 256-bit AVX read only <-> 128-bit non-AVX
   
   %macro testinitc 0
      vzeroupper
      vmovaps yword [psi], ymm0
   %endmacro

   %macro testcode 0
      %rep 100        ; repeat 100 times
         por xmm1,xmm2
         vmovaps yword [psi], ymm0
      %endrep
   %endmacro

%else
   %error unknown test case tcase
%endif

; disable default test loops
%define repeat1 1
%define repeat2 1
