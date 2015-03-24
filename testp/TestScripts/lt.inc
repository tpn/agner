; reg2instr.inc                                          2013-08-27 Agner Fog
;
; Define universal test code to test latency and throughput for instructions with 0-4 register operands
; and optionally one immediate operand, 64 bit mode.
; (c) Copyright 2013 by Agner Fog. GNU General Public License www.gnu.org/licenses
;
; Parameters:
;
; instruct:   Instruction to test
;
; regsize:    Register size: 8, 16, 32, 64, 128, 256, (or 65 = mmx). Default = 32
;
; numop:      Number of register operands (0 - 2, 3 if regsize = 256)
;
; numimm:     Number of immediate operands (0 - 2). Default = 0
;
; immvalue:   Value of first immediate operand. Default = 0
;
; tmode:      Test mode:
;             L:   measure latency
;             T:   measure throughput
;             M:   measure throughput with memory source operand
;             MR:  measure throughput with memory destination operand (only for numop = 2)
;             LMR: measure latency with memory destination operand (only for numop = 1 or 2)
;             M3:  measure throughput with memory source in operand 3 (only for numop = 4)
;             L2:  measure latency from operand 2 (only for numop = 4)
;             L4:  measure latency from operand 4 (only for numop = 4)
;             A:   clear eax (only for numop = 0)
;             D:   clear eax and edx (only for numop = 0)
;
; blockp:     Insert instructions that block a particular port or pipe when measuring throughput
;             Possible values for Intel: 0, 1, 5
;             Possible values for AMD:   0, 1
;
; cpubrand:   CPU brand: Intel, AMD, VIA

; Macro that blocks port 0, 1 or 5 on Intel Sandy Bridge or later
; Blocks port 0, 1 or none(9) on Intel silvermont
%macro blockportIntel 0
   %ifdef blockp
      %if imodel==0x37 || imodel==0x4A || imodel==0x4D
         ; Silvermont
         %if blockp   == 0
            ; Insert extra instruction that blocks port 0
            movsx r11,r11w
         %elif blockp == 1
            ; Insert extra instruction that blocks port 1
            lea r11d,[r11+r11]
         %elif blockp == 9
            ; Insert NOP to see if uop is pairable
            nop
         %endif
      %else
         ; other Intel (Sandy Bridge and later)
         %if blockp   == 0
            ; Insert extra instruction that blocks port 0
            vpmullw xmm8,xmm10,xmm10
            %define  USEAVX  1
         %elif blockp == 1
            ; Insert extra instruction that blocks port 1
            vaddss xmm9,xmm10,xmm10
            %define  USEAVX  1
         %elif blockp == 5
            ; Insert extra instruction that blocks port 5
            jo $+2
         %endif
      %endif
   %endif
%endmacro

; Macro that blocks integer unit EX0 or EX1 on AMD Bulldozer or later
%macro blockportAMD 0
   %ifdef blockp
      %if blockp   == 0
         ; Insert extra instruction that blocks integer pipe 0
         lzcnt  r8d, r9d    ; EX0, Latency 2, throughput 1/2
      %elif blockp == 1
         ; Insert extra instruction that blocks integer pipe 1
         popcnt r8d, r9d    ; EX1, Latency 4, throughput 1/2
      %endif
   %endif
%endmacro

%macro blockEven 0   ; block a port, on even iterations
   %ifdef cpubrand
      %ifidni   cpubrand, Intel
         blockportIntel
      %elifidni cpubrand, AMD
         blockportAMD
      %endif
   %endif
%endmacro

%macro blockOdd 0   ; block a port, on odd iterations
   %ifdef cpubrand
      %ifidni   cpubrand, Intel
         blockportIntel
      %elifidni cpubrand, AMD
         ; nothing because blockEven has throughput 1/2
      %endif
   %endif
%endmacro

; main testcode macro
%macro testcode 0

   %if regsize <= 64  ; reset registers
      xor eax,eax
      xor ebx,ebx
      xor ecx,ecx
      xor edx,edx
      xor edi,edi
   %endif

   ; test loop 1
   mov r12d,100
   align 32
   Testloop1:

   %if numop == 0  ; 0 register operands

      %ifidni tmode, L           ; measure latency or throughput
         %rep 50
            instruct immoperands0
            blockEven
            instruct immoperands0
            blockOdd
         %endrep
      %elifidni tmode, T         ; measure latency or throughput
         %rep 50
            instruct immoperands0
            blockEven
            instruct immoperands0
            blockOdd
         %endrep
      %elifidni tmode, A         ; clear eax
         %rep 50
            xor eax,eax
            instruct immoperands0
            blockEven
            xor eax,eax
            instruct immoperands0
            blockOdd
         %endrep
      %elifidni tmode,D         ; clear eax and edx
         %rep 50
            xor eax,eax
            xor edx,edx
            instruct immoperands0
            blockEven
            xor eax,eax
            xor edx,edx
            instruct immoperands0
            blockOdd
         %endrep
      %else
         %error unknown testmode
      %endif

   %elif numop == 1  ; 1 register operand

      %ifidni tmode, L           ; measure latency
         %rep 100 
            instruct reg0 immoperands1
         %endrep
      %elifidni tmode, T         ; measure throughput with register operands
         %rep 25
            instruct reg0 immoperands1
            blockEven
            instruct reg1 immoperands1
            blockOdd
            instruct reg2 immoperands1
            blockEven
            instruct reg3 immoperands1
            blockOdd
         %endrep
      %elifidni tmode, M         ; measure throughput with memory operand
         %rep 10
            instruct sizeptr [rsi] immoperands1
            blockEven
            instruct sizeptr [rsi+regsize] immoperands1
            blockOdd
            instruct sizeptr [rsi+regsize*2] immoperands1
            blockEven
            instruct sizeptr [rsi+regsize*3] immoperands1
            blockOdd
            instruct sizeptr [rsi+regsize*4] immoperands1
            blockEven
            instruct sizeptr [rsi+regsize*5] immoperands1
            blockOdd
            instruct sizeptr [rsi+regsize*6] immoperands1
            blockEven
            instruct sizeptr [rsi+regsize*7] immoperands1
            blockOdd
            instruct sizeptr [rsi+regsize*8] immoperands1
            blockEven
            instruct sizeptr [rsi+regsize*9] immoperands1
            blockOdd
         %endrep
      %elifidni tmode, LMR        ; measure latency with memory destination operand
         %rep 50
            instruct sizeptr [rsi] immoperands1
            blockEven
            instruct sizeptr [rsi] immoperands1
            blockOdd
         %endrep
      %else
         %error unknown testmode
      %endif

   %elif numop == 2  ; 2 register operands
      %ifidni tmode, L         ; measure latency
         %rep 50 
            instruct reg0, reg1 immoperands1
            instruct reg1, reg0 immoperands1
         %endrep
      %elifidni tmode, T         ; measure throughput with register operands
         %if regsize == 65       ; there are not 10 mmx registers
            %rep 20
               instruct reg1, reg0 immoperands1
               blockEven
               instruct reg2, reg0 immoperands1
               blockOdd
               instruct reg3, reg0 immoperands1
               blockEven
               instruct reg4, reg0 immoperands1
               blockOdd
               instruct reg5, reg0 immoperands1
               blockOdd
            %endrep
         %else
            %rep 10
               instruct reg1, reg0 immoperands1
               blockEven
               instruct reg2, reg0 immoperands1
               blockOdd
               instruct reg3, reg0 immoperands1
               blockEven
               instruct reg4, reg0 immoperands1
               blockOdd
               instruct reg5, reg0 immoperands1
               blockEven
               %if modesize == 64
               instruct reg6, reg0 immoperands1
               blockOdd
               instruct reg7, reg0 immoperands1
               blockEven
               instruct reg8, reg0 immoperands1
               blockOdd
               instruct reg9, reg0 immoperands1
               blockEven
               instruct reg10, reg0 immoperands1
               blockOdd               
               %else  ; 32 bit mode. reg8-10 not defined
               instruct reg1, reg0 immoperands1
               blockOdd
               instruct reg2, reg0 immoperands1
               blockEven
               instruct reg3, reg0 immoperands1
               blockOdd
               instruct reg4, reg0 immoperands1
               blockEven
               instruct reg5, reg0 immoperands1
               blockOdd
               %endif               
            %endrep
         %endif
         
      %elifidni tmode, M         ; measure throughput with memory source operand
         %if modesize == 64 && regsize != 65
         %rep 10
            instruct reg0, [rsi] immoperands1
            blockEven
            instruct reg1, [rsi+regsize/8] immoperands1
            blockOdd
            instruct reg2, [rsi+regsize*2/8] immoperands1
            blockEven
            instruct reg3, [rsi+regsize*3/8] immoperands1
            blockOdd
            instruct reg4, [rsi+regsize*4/8] immoperands1
            blockEven
            ; avoid reg5 = rsi if registersize = 32/64
            instruct reg6, [rsi] immoperands1
            blockOdd
            instruct reg7, [rsi+regsize/8] immoperands1
            blockEven
            instruct reg8, [rsi+regsize*2/8] immoperands1
            blockOdd
            instruct reg9, [rsi+regsize*3/8] immoperands1
            blockEven
            instruct reg10, [rsi+regsize*4/8] immoperands1
            blockOdd
         %endrep         
         %else  ; 32 bit mode. reg8-10 not defined
         %rep 20
            instruct reg0, [rsi] immoperands1
            blockEven
            instruct reg1, [rsi+regsize/8] immoperands1
            blockOdd
            instruct reg2, [rsi+regsize*2/8] immoperands1
            blockEven
            instruct reg3, [rsi+regsize*3/8] immoperands1
            blockOdd
            instruct reg4, [rsi+regsize*4/8] immoperands1  ; avoid reg5 = rsi
            blockOdd
         %endrep         
         %endif
         
      %elifidni tmode, MR        ; measure throughput with memory destination operand
         %rep 10
            instruct [rsi],             reg0
            blockEven
            instruct [rsi+regsize/8],   reg1
            blockOdd
            instruct [rsi+regsize*2/8], reg2
            blockEven
            instruct [rsi+regsize*3/8], reg3
            blockOdd
            instruct [rsi+regsize*4/8], reg4
            blockEven
            instruct [rsi+regsize*5/8], reg0
            blockOdd
            instruct [rsi+regsize*6/8], reg1
            blockEven
            instruct [rsi+regsize*7/8], reg2
            blockOdd
            instruct [rsi+regsize*8/8], reg3
            blockEven
            instruct [rsi+regsize*9/8], reg4
            blockOdd
         %endrep
      %elifidni tmode, LMR        ; measure latency with memory destination operand
         %rep 50
            instruct [rsi],             reg0
            blockEven
            instruct [rsi],             reg0
            blockOdd
         %endrep
      %else
         %error unknown testmode
      %endif

   %elif numop == 3  ; 3 register operands (AVX/XOP/FMA3)
   
      %ifidni tmode, L         ; measure latency
         %rep 50 
            instruct reg0, reg0, reg1 immoperands1
            instruct reg1, reg1, reg0 immoperands1
         %endrep
      %elifidni tmode, L0        ; measure latency from operand 0
         %rep 50 
            instruct reg0, reg1, reg2 immoperands1
            instruct reg0, reg4, reg5 immoperands1
         %endrep
      %elifidni tmode, T         ; measure throughput with register operands
         %rep 50
            instruct reg0, reg1, reg2 immoperands1
            blockEven
            ; avoid reg3 = edx, which is implicit operand in mulx instruction
            instruct reg4, reg5, reg6 immoperands1
            blockOdd
         %endrep
      %elifidni tmode, T0        ; measure throughput with register operands where op0 is both source and destination
         %if modesize == 64
         %rep 10
            instruct reg2,  reg0, reg1 immoperands1
            blockEven
            instruct reg3,  reg0, reg1 immoperands1
            blockOdd
            instruct reg4,  reg0, reg1 immoperands1
            blockEven
            instruct reg5,  reg0, reg1 immoperands1
            blockOdd
            instruct reg6,  reg0, reg1 immoperands1
            blockEven
            instruct reg7,  reg0, reg1 immoperands1
            blockOdd
            instruct reg8,  reg0, reg1 immoperands1
            blockEven
            instruct reg9,  reg0, reg1 immoperands1
            blockOdd
            instruct reg10, reg0, reg1 immoperands1
            blockEven
            instruct reg11, reg0, reg1 immoperands1
            blockOdd
         %endrep
         %else  ; 32 bit mode. reg8-11 not defined
         %rep 10
            instruct reg2,  reg0, reg1 immoperands1
            blockEven
            instruct reg3,  reg0, reg1 immoperands1
            blockOdd
            instruct reg4,  reg0, reg1 immoperands1
            blockEven
            instruct reg5,  reg0, reg1 immoperands1
            blockOdd
            instruct reg6,  reg0, reg1 immoperands1
            blockEven
            instruct reg7,  reg0, reg1 immoperands1
            blockOdd
            instruct reg3,  reg0, reg1 immoperands1
            blockEven
            instruct reg4,  reg0, reg1 immoperands1
            blockOdd
            instruct reg5,  reg0, reg1 immoperands1
            blockEven       
            instruct reg6,  reg0, reg1 immoperands1
            blockOdd
         %endrep         
         %endif         
      %elifidni tmode, M         ; measure throughput with memory source operand
         %rep 50
            instruct reg0, reg1, [rsi] immoperands1
            blockEven
            instruct reg2, reg3, [rsi+regsize/8] immoperands1
            blockOdd
         %endrep
      %elifidni tmode, M2        ; measure throughput with memory as second operand
         %rep 25
            instruct reg0, [rsi], reg1 immoperands1
            blockEven
            instruct reg2, [rsi+regsize/8], reg3 immoperands1
            blockOdd
            instruct reg4, [rsi], reg1 immoperands1
            blockEven
            instruct reg6, [rsi+regsize/8], reg3 immoperands1
            blockOdd
         %endrep         
      %elifidni tmode, M0         ; measure throughput with memory source operand where op0 is both source and destination
         %rep 10
            instruct reg1, reg0, [rsi] immoperands1
            blockEven
            instruct reg2, reg0, [rsi+regsize/8] immoperands1
            blockOdd
            instruct reg3, reg0, [rsi+2*regsize/8] immoperands1
            blockEven
            instruct reg4, reg0, [rsi+3*regsize/8] immoperands1
            blockOdd
            instruct reg6, reg0, [rsi+4*regsize/8] immoperands1
            blockEven
            instruct reg7, reg0, [rsi+0*regsize/8] immoperands1
            blockOdd
            instruct reg8, reg0, [rsi+1*regsize/8] immoperands1
            blockEven
            instruct reg9, reg0, [rsi+2*regsize/8] immoperands1
            blockOdd
            instruct reg10, reg0, [rsi+3*regsize/8] immoperands1
            blockEven
            instruct reg11,reg0, [rsi+4*regsize/8] immoperands1            
         %endrep
      %else
         %error unknown testmode
      %endif

   %elif numop == 4  ; 4 register operands (AVX/XOP/FMA4)
      %ifidni tmode, L         ; measure latency
         %rep 50 
            instruct reg0, reg1, reg1, reg1 immoperands1
            instruct reg1, reg0, reg0, reg0 immoperands1
         %endrep
      %elifidni tmode, L2       ; measure latency from operand 2
         %rep 50 
            instruct reg0, reg1, reg2, reg2 immoperands1
            instruct reg1, reg0, reg3, reg3 immoperands1
         %endrep
      %elifidni tmode, L4       ; measure latency from operand 4
         %rep 50 
            instruct reg0, reg2, reg2, reg1 immoperands1
            instruct reg1, reg3, reg3, reg0 immoperands1
         %endrep
      %elifidni tmode, T         ; measure throughput with register operands
         %rep 50
            instruct reg0, reg1, reg2, reg2 immoperands1
            blockEven
            instruct reg3, reg4, reg5, reg5 immoperands1
            blockOdd
         %endrep
      %elifidni tmode, M         ; measure throughput with memory source operand
         %rep 50
            instruct reg0, reg1, reg2, [rsi] immoperands1
            blockEven
            instruct reg3, reg4, reg5, [rsi+regsize/8] immoperands1
            blockOdd
         %endrep
      %elifidni tmode, M3        ; measure throughput with memory source in operand 3
         %rep 50
            instruct reg0, reg1, [rsi], reg2 immoperands1
            blockEven
            instruct reg3, reg4, [rsi+regsize/8], reg5 immoperands1
            blockOdd
         %endrep
      %else
         %error unknown testmode
      %endif

   %else
      %error unsupported numop
   %endif

   dec r12d
   jnz Testloop1   ; loop
%endmacro ; testcode

; disable default test loops
%define repeat1 1
%define repeat2 1

