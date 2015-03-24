; fvecdiv.inc
; Define test code for floating point vector division and squareroot, etc
; (c) Copyright 2012 by Agner Fog. GNU General Public License www.gnu.org/licenses

; Parameters:
;
; instruct:   Instruction to test, e.g. vdivps
;
; instructt:  Instruction type: div, vdiv, sqrt
;
; suff:       Instruction suffix: ss, sd, ps, pd
;
; tmode:      Test mode:
;             L:  measure latency
;             T:  measure throughput
;             M:  measure throughput with memory source operand
;
; tcase:      best:  trivial data for fastest case, 
;             worst: non-trivial data for slow case
;
; regsize:    register size: 128 256
;
; dividend:   dividend in division
;
; divisor:    divisor in division

; Default values:

%ifndef repeat1
   %define repeat1 1000
%endif

%ifndef tmode
   %define tmode  L
%endif

%ifndef tcase
   %define tcase  worst
%endif

%ifndef dividend
   %define dividend 8.1509281715106E12   ; div1
%endif

%ifndef divisor
   %define divisor 1.20278165192619   ; div2
%endif

; get precision from suffix
%ifidni suff, ss
   %define precision 32
%elifidni suff, ps
   %define precision 32
%elifidni suff, sd
   %define precision 64
%elifidni suff, pd
   %define precision 64
%else
   %error unknown suffix suff
%endif

; define static data
%macro testdata 0
   times 1000H  DB 0
   %if precision == 32
      one    times 8 dd 1.0
      div1   times 8 dd dividend
      div2   times 8 dd divisor
      large  times 8 dd 8.3610378602352937E30
   %else ; double precision
      one    times 4 dq 1.0
      div1   times 4 dq dividend
      div2   times 4 dq divisor
      large  times 4 dq 8.3610378602352937E30
   %endif
%endmacro


%ifidni instructt, div

   %macro testinit2 0
      %ifidni tcase, best
         movaps reg0, [one]
         movaps reg1, [one]
         movaps reg2, [one]
         lea rsi, [one]
      %else  ; worst case
         movaps reg0, [div1]
         movaps reg1, [div1]
         movaps reg2, [div2]
         lea rsi, [div2]
      %endif
   %endmacro
   %macro testafter1 0
      %ifidni tmode, L
         %if precision == 32
            addps reg1, reg0
         %else
            addpd reg1, reg0
         %endif
      %endif
   %endmacro
   %macro testcode 0
      %ifidni tmode, L
         instruct reg1,reg2
      %elifidni tmode, T
         instruct reg1,reg2
         movaps reg1, reg0
      %elifidni tmode, M
         instruct reg1,[rsi]
         movaps reg1, reg0
      %endif
   %endmacro

%elifidni instructt, vdiv

   %macro testinit2 0
      %ifidni tcase, best
         vmovaps reg0, [one]
         vmovaps reg1, [one]
         lea rsi, [one]
      %else  ; worst case
         vmovaps reg0, [div1]
         vmovaps reg1, [div1]
         vmovaps reg2, [div2]
         lea rsi, [div2]
      %endif
   %endmacro
   %macro testafter1 0
      %ifidni tmode, L
         %if precision == 32
            vaddps reg1, reg1, reg0
         %else
            vaddpd reg1, reg1, reg0
         %endif
      %endif
   %endmacro
   %macro testcode 0
      %ifidni tmode, L
         instruct reg1,reg1,reg2
      %elifidni tmode, T
         instruct reg3,reg1,reg2
      %elifidni tmode, M
         instruct reg3,reg1,[rsi]
      %endif
   %endmacro


%elifidni instructt, sqrt

   %define repeat2 1
   %macro testinit2 0
      %ifidni tcase, best
         movaps reg0, [one]
         movaps reg1, reg0
         movaps reg2, reg0
         movaps reg3, reg0
         movaps reg4, reg0
         movaps reg5, reg0
         lea rsi, [one]
      %else  ; worst case
         movaps reg0, [large]
         movaps reg1, reg0
         movaps reg2, reg0
         movaps reg3, reg0
         movaps reg4, reg0
         movaps reg5, reg0
         lea rsi, [large]
      %endif
   %endmacro
   %macro testafter1 0
      %ifidni tmode, L
         %if precision == 32
            addps reg1, reg0
         %else
            addpd reg1, reg0
         %endif
      %endif
   %endmacro
   %macro testcode 0
      %ifidni tmode, L
         %rep 100
            instruct reg1,reg1
         %endrep
      %elifidni tmode, T
         %rep 20
            instruct reg1,reg0
            instruct reg2,reg0
            instruct reg3,reg0
            instruct reg4,reg0
            instruct reg5,reg0
         %endrep
      %elifidni tmode, M
         %rep 20
            instruct reg1,[rsi]
            instruct reg2,[rsi]
            instruct reg3,[rsi]
            instruct reg4,[rsi]
            instruct reg5,[rsi]
         %endrep
      %endif
   %endmacro

%elifidni instructt, vsqrt

   %define repeat2 1
   %macro testinit2 0
      %ifidni tcase, best
         vmovaps reg0, [one]
         vmovaps reg1, reg0
         vmovaps reg2, reg0
         vmovaps reg3, reg0
         vmovaps reg4, reg0
         vmovaps reg5, reg0
         lea rsi, [one]
      %else  ; worst case
         vmovaps reg0, [large]
         vmovaps reg1, reg0
         vmovaps reg2, reg0
         vmovaps reg3, reg0
         vmovaps reg4, reg0
         vmovaps reg5, reg0
         lea rsi, [large]
      %endif
   %endmacro
   %macro testafter1 0
      %ifidni tmode, L
         %if precision == 32
            vaddps reg1, reg1, reg0
         %else
            vaddpd reg1, reg1, reg0
         %endif
      %endif
   %endmacro
   %macro testcode 0
      %ifidni tmode, L
         %rep 100
            instruct reg1,reg1
         %endrep
      %elifidni tmode, T
         %rep 20
            instruct reg1,reg0
            instruct reg2,reg0
            instruct reg3,reg0
            instruct reg4,reg0
            instruct reg5,reg0
         %endrep
      %elifidni tmode, M
         %rep 20
            instruct reg1,[rsi]
            instruct reg2,[rsi]
            instruct reg3,[rsi]
            instruct reg4,[rsi]
            instruct reg5,[rsi]
         %endrep
      %endif
   %endmacro

%elifidni instructt, rcp  ; reciprocal and reciprocal squareroot

   %define repeat2 1
   %macro testinit2 0
      %if regsize == 128
         movaps reg0, [large]
         movaps reg1, reg0
         movaps reg2, reg0
         movaps reg3, reg0
         movaps reg4, reg0
         movaps reg5, reg0
      %else
         vmovaps reg0, [large]
         vmovaps reg1, reg0
         vmovaps reg2, reg0
         vmovaps reg3, reg0
         vmovaps reg4, reg0
         vmovaps reg5, reg0
      %endif
      lea rsi, [large]
   %endmacro
   %macro testafter1 0
      %ifidni tmode, L
         %if regsize == 128
            addps reg1, reg0
         %else
            vaddps reg1, reg1, reg0
         %endif
      %endif
   %endmacro
   %macro testcode 0
      %ifidni tmode, L
         %rep 100
            instruct reg1,reg1
         %endrep
      %elifidni tmode, T
         %rep 20
            instruct reg1,reg0
            instruct reg2,reg0
            instruct reg3,reg0
            instruct reg4,reg0
            instruct reg5,reg0
         %endrep
      %elifidni tmode, M
         %rep 20
            instruct reg1,[rsi]
            instruct reg2,[rsi]
            instruct reg3,[rsi]
            instruct reg4,[rsi]
            instruct reg5,[rsi]
         %endrep
      %endif
   %endmacro





%endif

