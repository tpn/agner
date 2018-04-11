;----------------------------------------------------------------------------
;        loop_buffer.inc                          2013-07-10 Agner Fog
;
# PMC Test program for testing any loop buffer or microop cache
;
; Constants to be defined:
; 
; nopsize:   Size of NOP instructions. (1 - 15)
; 
; noptype:   2: long NOPs (0F 1F ...)
;            3: 66 NOPs (simple NOP with up to 14 operand size prefixes)
; 
; repeat1:   Number of loop repetitions
;
; repeat2:   Number of NOPs in loop
;
; (c) Copyright 2013 by Agner Fog. GNU General Public License www.gnu.org/licenses
;-----------------------------------------------------------------------------
; Define any undefined macros

; define long nops
%ifndef noptype
   %define noptype 2
%endif

%include "nops.inc"

%if nopsize == 1
   %macro testcode 0
      nop1
   %endmacro   
%elif nopsize == 2
   %macro testcode 0
      nop2
   %endmacro
%elif nopsize == 3
   %macro testcode 0
      nop3
   %endmacro
%elif nopsize == 4
   %macro testcode 0
      nop4
   %endmacro
%elif nopsize == 5
   %macro testcode 0
      nop5
   %endmacro
%elif nopsize == 6
   %macro testcode 0
      nop6
   %endmacro
%elif nopsize == 7
   %macro testcode 0
      nop7
   %endmacro
%elif nopsize == 8
   %macro testcode 0
      nop8
   %endmacro
%elif nopsize == 9
   %macro testcode 0
      nop9
   %endmacro
%elif nopsize == 10
   %macro testcode 0
      nop10
   %endmacro
%elif nopsize == 11
   %macro testcode 0
      nop11
   %endmacro
%elif nopsize == 12
   %macro testcode 0
      nop12
   %endmacro
%elif nopsize == 13
   %macro testcode 0
      nop13
   %endmacro
%elif nopsize == 14
   %macro testcode 0
      nop14
   %endmacro
%elif nopsize == 15
   %macro testcode 0
      nop15
   %endmacro
%else
   %error wrong nop size nopsize
%endif
