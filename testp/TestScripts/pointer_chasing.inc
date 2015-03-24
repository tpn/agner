; pointer_chasing.inc                                                        2013-04-24 AgF

; Test cache access time
; (c) 2013 by Agner Fog. GNU General Public License www.gnu.org/licenses

; Parameters:
;
; nesting: function nesting level (minimum value = 2)

; define test data
%macro testdata 0
pointers dq 0,0,0,0
%endmacro

; initialization of pointers
%macro testinit1 0
    lea rsi, [pointers]
    lea rdi, [rsi+8]
    mov [rsi], rdi
    add rdi, 8
    mov [rsi+8], rdi
    add rdi, 8
    mov [rsi+16], rdi
    add rdi, 8
    mov [rsi+24], rsi
    mov rdi, rsi
%endmacro


; main testcode macro
%macro testcode 0
    mov rdi, [rdi]
%endmacro

; disable default test loops
%define repeat1 1000
%define repeat2 100
