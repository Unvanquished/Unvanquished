;/*****************************************************************************
; *
; *  XVID MPEG-4 VIDEO CODEC
; *  - Quarter-pixel interpolation -
; *  Copyright(C) 2002 Pascal Massimino <skal@planet-d.net>
; *
; *  This file is part of XviD, a free MPEG-4 video encoder/decoder
; *
; *  XviD is free software; you can rDST_PTRstribute it and/or modify it
; *  under the terms of the GNU General Public License as published by
; *  the Free Software Foundation; either version 2 of the License, or
; *  (at your option) any later version.
; *
; *  This program is distributed in the hope that it will be useful,
; *  but WITHOUT ANY WARRANTY; without even the implied warranty of
; *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; *  GNU General Public License for more details.
; *
; *  You should have received a copy of the GNU General Public License
; *  along with this program; if not, write to the Free Software
; *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
; *
; * $Id: qpel_mmx.asm,v 1.9.2.2 2009/09/16 17:11:39 Isibaar Exp $
; *
; *************************************************************************/

;/**************************************************************************
; *
; *	History:
; *
; * 22.10.2002  initial coding. unoptimized 'proof of concept',
; *             just to heft the qpel filtering. - Skal -
; *
; *************************************************************************/


%define USE_TABLES      ; in order to use xvid_FIR_x_x_x_x tables
                        ; instead of xvid_Expand_mmx...


%include "nasm.inc"

;//////////////////////////////////////////////////////////////////////
;// Declarations
;//   all signatures are:
;// void XXX(uint8_t *dst, const uint8_t *src,
;//          int32_t length, int32_t stride, int32_t rounding)
;//////////////////////////////////////////////////////////////////////

cglobal xvid_H_Pass_16_mmx
cglobal xvid_H_Pass_Avrg_16_mmx
cglobal xvid_H_Pass_Avrg_Up_16_mmx
cglobal xvid_V_Pass_16_mmx
cglobal xvid_V_Pass_Avrg_16_mmx
cglobal xvid_V_Pass_Avrg_Up_16_mmx
cglobal xvid_H_Pass_8_mmx
cglobal xvid_H_Pass_Avrg_8_mmx
cglobal xvid_H_Pass_Avrg_Up_8_mmx
cglobal xvid_V_Pass_8_mmx
cglobal xvid_V_Pass_Avrg_8_mmx
cglobal xvid_V_Pass_Avrg_Up_8_mmx

cglobal xvid_H_Pass_Add_16_mmx
cglobal xvid_H_Pass_Avrg_Add_16_mmx
cglobal xvid_H_Pass_Avrg_Up_Add_16_mmx
cglobal xvid_V_Pass_Add_16_mmx
cglobal xvid_V_Pass_Avrg_Add_16_mmx
cglobal xvid_V_Pass_Avrg_Up_Add_16_mmx
cglobal xvid_H_Pass_8_Add_mmx
cglobal xvid_H_Pass_Avrg_8_Add_mmx
cglobal xvid_H_Pass_Avrg_Up_8_Add_mmx
cglobal xvid_V_Pass_8_Add_mmx
cglobal xvid_V_Pass_Avrg_8_Add_mmx
cglobal xvid_V_Pass_Avrg_Up_8_Add_mmx

cglobal xvid_Expand_mmx

cglobal xvid_FIR_1_0_0_0
cglobal xvid_FIR_3_1_0_0
cglobal xvid_FIR_6_3_1_0
cglobal xvid_FIR_14_3_2_1
cglobal xvid_FIR_20_6_3_1
cglobal xvid_FIR_20_20_6_3
cglobal xvid_FIR_23_19_6_3
cglobal xvid_FIR_7_20_20_6
cglobal xvid_FIR_6_20_20_6
cglobal xvid_FIR_6_20_20_7
cglobal xvid_FIR_3_6_20_20
cglobal xvid_FIR_3_6_19_23
cglobal xvid_FIR_1_3_6_20
cglobal xvid_FIR_1_2_3_14
cglobal xvid_FIR_0_1_3_6
cglobal xvid_FIR_0_0_1_3
cglobal xvid_FIR_0_0_0_1

SECTION .data align=SECTION_ALIGN

align SECTION_ALIGN
xvid_Expand_mmx:
times 256*4 dw 0        ; uint16_t xvid_Expand_mmx[256][4]
ENDFUNC

xvid_FIR_1_0_0_0:
times 256*4 dw 0
ENDFUNC

xvid_FIR_3_1_0_0:
times 256*4 dw 0
ENDFUNC

xvid_FIR_6_3_1_0:
times 256*4 dw 0
ENDFUNC

xvid_FIR_14_3_2_1:
times 256*4 dw 0
ENDFUNC

xvid_FIR_20_6_3_1:
times 256*4 dw 0
ENDFUNC

xvid_FIR_20_20_6_3:
times 256*4 dw 0
ENDFUNC

xvid_FIR_23_19_6_3:
times 256*4 dw 0
ENDFUNC

xvid_FIR_7_20_20_6:
times 256*4 dw 0
ENDFUNC

xvid_FIR_6_20_20_6:
times 256*4 dw 0
ENDFUNC

xvid_FIR_6_20_20_7:
times 256*4 dw 0
ENDFUNC

xvid_FIR_3_6_20_20:
times 256*4 dw 0
ENDFUNC

xvid_FIR_3_6_19_23:
times 256*4 dw 0
ENDFUNC

xvid_FIR_1_3_6_20:
times 256*4 dw 0
ENDFUNC

xvid_FIR_1_2_3_14:
times 256*4 dw 0
ENDFUNC

xvid_FIR_0_1_3_6:
times 256*4 dw 0
ENDFUNC

xvid_FIR_0_0_1_3:
times 256*4 dw 0
ENDFUNC

xvid_FIR_0_0_0_1:
times 256*4 dw 0
ENDFUNC

;//////////////////////////////////////////////////////////////////////

DATA

align SECTION_ALIGN
Rounder1_MMX:
times 4 dw 1
Rounder0_MMX:
times 4 dw 0

align SECTION_ALIGN
Rounder_QP_MMX:
times 4 dw 16
times 4 dw 15

%ifndef USE_TABLES

align SECTION_ALIGN

  ; H-Pass table shared by 16x? and 8x? filters

FIR_R0:  dw 14, -3,  2, -1
align SECTION_ALIGN
FIR_R1:  dw 23, 19, -6,  3,   -1,  0,  0,  0

FIR_R2:  dw -7, 20, 20, -6,    3, -1,  0,  0

FIR_R3:  dw  3, -6, 20, 20,   -6,  3, -1,  0

FIR_R4:  dw -1,  3, -6, 20,   20, -6,  3, -1

FIR_R5:  dw  0, -1,  3, -6,   20, 20, -6,  3,   -1,  0,  0,  0
align SECTION_ALIGN
FIR_R6:  dw  0,  0, -1,  3,   -6, 20, 20, -6,    3, -1,  0,  0
align SECTION_ALIGN
FIR_R7:  dw  0,  0,  0, -1,    3, -6, 20, 20,   -6,  3, -1,  0
align SECTION_ALIGN
FIR_R8:  dw                   -1,  3, -6, 20,   20, -6,  3, -1

FIR_R9:  dw                    0, -1,  3, -6,   20, 20, -6,  3,   -1,  0,  0,  0
align SECTION_ALIGN
FIR_R10: dw                    0,  0, -1,  3,   -6, 20, 20, -6,    3, -1,  0,  0
align SECTION_ALIGN
FIR_R11: dw                    0,  0,  0, -1,    3, -6, 20, 20,   -6,  3, -1,  0
align SECTION_ALIGN
FIR_R12: dw                                     -1,  3, -6, 20,   20, -6,  3, -1

FIR_R13: dw                                      0, -1,  3, -6,   20, 20, -6,  3

FIR_R14: dw                                      0,  0, -1,  3,   -6, 20, 20, -7

FIR_R15: dw                                      0,  0,  0, -1,    3, -6, 19, 23

FIR_R16: dw                                                       -1,  2, -3, 14

%endif  ; !USE_TABLES

  ; V-Pass taps

align SECTION_ALIGN
FIR_Cm7: times 4 dw -7
FIR_Cm6: times 4 dw -6
FIR_Cm3: times 4 dw -3
FIR_Cm1: times 4 dw -1
FIR_C2:  times 4 dw  2
FIR_C3:  times 4 dw  3
FIR_C14: times 4 dw 14
FIR_C19: times 4 dw 19
FIR_C20: times 4 dw 20
FIR_C23: times 4 dw 23

TEXT

;//////////////////////////////////////////////////////////////////////
;// Here we go with the Q-Pel mess.
;//  For horizontal passes, we process 4 *output* pixel in parallel
;//  For vertical ones, we process 4 *input* pixel in parallel.
;//////////////////////////////////////////////////////////////////////

%ifdef ARCH_IS_X86_64
%macro XVID_MOVQ 3
  lea r9, [%2]
  movq %1, [r9 + %3]
%endmacro
%macro XVID_PADDW 3
  lea r9, [%2]
  paddw %1, [r9 + %3]
%endmacro
%define SRC_PTR prm2 
%define DST_PTR prm1 
%else
%macro XVID_MOVQ 3
  movq %1, [%2 + %3]
%endmacro
%macro XVID_PADDW 3
  paddw %1, [%2 + %3]
%endmacro
%define SRC_PTR _ESI
%define DST_PTR _EDI
%endif

%macro PROLOG_NO_AVRG 0
  mov TMP0, prm3 ; Size
  mov TMP1, prm4 ; BpS
  mov eax, prm5d ; Rnd

%ifndef ARCH_IS_X86_64
  push SRC_PTR
  push DST_PTR
%endif
  push _EBP
  mov _EBP, TMP1

%ifndef ARCH_IS_X86_64
  mov DST_PTR, [_ESP+16 + 0*4] ; Dst
  mov SRC_PTR, [_ESP+16 + 1*4] ; Src
%endif
  
  and _EAX, 1
  lea TMP1, [Rounder_QP_MMX]
  movq mm7, [TMP1+_EAX*8]  ; rounder
%endmacro

%macro EPILOG_NO_AVRG 0
  pop _EBP
%ifndef ARCH_IS_X86_64
  pop DST_PTR
  pop SRC_PTR
%endif
  ret
%endmacro

%macro PROLOG_AVRG 0
  mov TMP0, prm3 ; Size
  mov TMP1, prm4 ; BpS
  mov eax, prm5d ; Rnd

  push _EBX
  push _EBP
%ifndef ARCH_IS_X86_64
  push SRC_PTR
  push DST_PTR
%endif
  mov _EBP, TMP1

%ifndef ARCH_IS_X86_64
  mov DST_PTR, [_ESP+20 + 0*4] ; Dst
  mov SRC_PTR, [_ESP+20 + 1*4] ; Src
%endif

  and _EAX, 1
  lea TMP1, [Rounder_QP_MMX]
  movq mm7, [TMP1+_EAX*8]  ; rounder
  lea TMP1, [Rounder1_MMX]
  lea _EBX, [TMP1+_EAX*8]     ; *Rounder2
%endmacro

%macro EPILOG_AVRG 0
%ifndef ARCH_IS_X86_64
  pop DST_PTR
  pop SRC_PTR
%endif
  pop _EBP
  pop _EBX
  ret
%endmacro

;//////////////////////////////////////////////////////////////////////
;//
;// All horizontal passes
;//
;//////////////////////////////////////////////////////////////////////

  ; macros for USE_TABLES

%macro TLOAD 2     ; %1,%2: src pixels
  movzx _EAX, byte [SRC_PTR+%1]
  movzx TMP1, byte [SRC_PTR+%2]
  XVID_MOVQ mm0, xvid_FIR_14_3_2_1, _EAX*8
  XVID_MOVQ mm3, xvid_FIR_1_2_3_14, TMP1*8
  paddw mm0, mm7
  paddw mm3, mm7
%endmacro

%macro TACCUM2 5   ;%1:src pixel/%2-%3:Taps tables/ %4-%5:dst regs
  movzx _EAX, byte [SRC_PTR+%1]
  XVID_PADDW %4, %2, _EAX*8
  XVID_PADDW %5, %3, _EAX*8
%endmacro

%macro TACCUM3 7   ;%1:src pixel/%2-%4:Taps tables/%5-%7:dst regs
  movzx _EAX, byte [SRC_PTR+%1]
  XVID_PADDW %5, %2, _EAX*8
  XVID_PADDW %6, %3, _EAX*8
  XVID_PADDW %7, %4, _EAX*8
%endmacro

;//////////////////////////////////////////////////////////////////////

  ; macros without USE_TABLES

%macro LOAD 2     ; %1,%2: src pixels
  movzx _EAX, byte [SRC_PTR+%1]
  movzx TMP1, byte [SRC_PTR+%2]
  XVID_MOVQ mm0, xvid_Expand_mmx, _EAX*8
  XVID_MOVQ mm3, xvid_Expand_mmx, TMP1*8
  pmullw mm0, [FIR_R0 ]
  pmullw mm3, [FIR_R16]
  paddw mm0, mm7
  paddw mm3, mm7
%endmacro

%macro ACCUM2 4   ;src pixel/Taps/dst regs #1-#2
  movzx _EAX, byte [SRC_PTR+%1]
  XVID_MOVQ mm4, xvid_Expand_mmx, _EAX*8
  movq mm5, mm4
  pmullw mm4, [%2]
  pmullw mm5, [%2+8]
  paddw %3, mm4
  paddw %4, mm5
%endmacro

%macro ACCUM3 5   ;src pixel/Taps/dst regs #1-#2-#3
  movzx _EAX, byte [SRC_PTR+%1]
  XVID_MOVQ mm4, xvid_Expand_mmx, _EAX*8
  movq mm5, mm4
  movq mm6, mm5
  pmullw mm4, [%2   ]
  pmullw mm5, [%2+ 8]
  pmullw mm6, [%2+16]
  paddw %3, mm4
  paddw %4, mm5
  paddw %5, mm6
%endmacro

;//////////////////////////////////////////////////////////////////////

%macro MIX 3   ; %1:reg, %2:src, %3:rounder
  pxor mm6, mm6
  movq mm4, [%2]
  movq mm1, %1
  movq mm5, mm4
  punpcklbw %1, mm6
  punpcklbw mm4, mm6
  punpckhbw mm1, mm6
  punpckhbw mm5, mm6
  movq mm6, [%3]   ; rounder #2
  paddusw %1, mm4
  paddusw mm1, mm5
  paddusw %1, mm6
  paddusw mm1, mm6
  psrlw %1, 1
  psrlw mm1, 1
  packuswb %1, mm1
%endmacro

;//////////////////////////////////////////////////////////////////////

%macro H_PASS_16  2   ; %1:src-op (0=NONE,1=AVRG,2=AVRG-UP), %2:dst-op (NONE/AVRG)

%if (%2==0) && (%1==0)
   PROLOG_NO_AVRG
%else
   PROLOG_AVRG
%endif

.Loop:

    ;  mm0..mm3 serves as a 4x4 delay line

%ifndef USE_TABLES

  LOAD 0, 16  ; special case for 1rst/last pixel
  movq mm1, mm7
  movq mm2, mm7

  ACCUM2 1,    FIR_R1, mm0, mm1
  ACCUM2 2,    FIR_R2, mm0, mm1
  ACCUM2 3,    FIR_R3, mm0, mm1
  ACCUM2 4,    FIR_R4, mm0, mm1

  ACCUM3 5,    FIR_R5, mm0, mm1, mm2
  ACCUM3 6,    FIR_R6, mm0, mm1, mm2
  ACCUM3 7,    FIR_R7, mm0, mm1, mm2
  ACCUM2 8,    FIR_R8, mm1, mm2
  ACCUM3 9,    FIR_R9, mm1, mm2, mm3
  ACCUM3 10,   FIR_R10,mm1, mm2, mm3
  ACCUM3 11,   FIR_R11,mm1, mm2, mm3

  ACCUM2 12,   FIR_R12, mm2, mm3
  ACCUM2 13,   FIR_R13, mm2, mm3
  ACCUM2 14,   FIR_R14, mm2, mm3
  ACCUM2 15,   FIR_R15, mm2, mm3

%else

  TLOAD 0, 16  ; special case for 1rst/last pixel
  movq mm1, mm7
  movq mm2, mm7

  TACCUM2 1,    xvid_FIR_23_19_6_3, xvid_FIR_1_0_0_0 , mm0, mm1
  TACCUM2 2,    xvid_FIR_7_20_20_6, xvid_FIR_3_1_0_0 , mm0, mm1
  TACCUM2 3,    xvid_FIR_3_6_20_20, xvid_FIR_6_3_1_0 , mm0, mm1
  TACCUM2 4,    xvid_FIR_1_3_6_20 , xvid_FIR_20_6_3_1, mm0, mm1

  TACCUM3 5,    xvid_FIR_0_1_3_6  , xvid_FIR_20_20_6_3, xvid_FIR_1_0_0_0  , mm0, mm1, mm2
  TACCUM3 6,    xvid_FIR_0_0_1_3  , xvid_FIR_6_20_20_6, xvid_FIR_3_1_0_0  , mm0, mm1, mm2
  TACCUM3 7,    xvid_FIR_0_0_0_1  , xvid_FIR_3_6_20_20, xvid_FIR_6_3_1_0  , mm0, mm1, mm2

  TACCUM2 8,                       xvid_FIR_1_3_6_20 , xvid_FIR_20_6_3_1 ,      mm1, mm2

  TACCUM3 9,                       xvid_FIR_0_1_3_6  , xvid_FIR_20_20_6_3, xvid_FIR_1_0_0_0,  mm1, mm2, mm3
  TACCUM3 10,                      xvid_FIR_0_0_1_3  , xvid_FIR_6_20_20_6, xvid_FIR_3_1_0_0,  mm1, mm2, mm3
  TACCUM3 11,                      xvid_FIR_0_0_0_1  , xvid_FIR_3_6_20_20, xvid_FIR_6_3_1_0,  mm1, mm2, mm3

  TACCUM2 12,  xvid_FIR_1_3_6_20, xvid_FIR_20_6_3_1 , mm2, mm3
  TACCUM2 13,  xvid_FIR_0_1_3_6 , xvid_FIR_20_20_6_3, mm2, mm3
  TACCUM2 14,  xvid_FIR_0_0_1_3 , xvid_FIR_6_20_20_7, mm2, mm3
  TACCUM2 15,  xvid_FIR_0_0_0_1 , xvid_FIR_3_6_19_23, mm2, mm3

%endif

  psraw mm0, 5
  psraw mm1, 5
  psraw mm2, 5
  psraw mm3, 5
  packuswb mm0, mm1
  packuswb mm2, mm3

%if (%1==1)
  MIX mm0, SRC_PTR, _EBX
%elif (%1==2)
  MIX mm0, SRC_PTR+1, _EBX
%endif
%if (%2==1)
  MIX mm0, DST_PTR, Rounder1_MMX
%endif

%if (%1==1)
  MIX mm2, SRC_PTR+8, _EBX
%elif (%1==2)
  MIX mm2, SRC_PTR+9, _EBX
%endif
%if (%2==1)
  MIX mm2, DST_PTR+8, Rounder1_MMX
%endif

  lea SRC_PTR, [SRC_PTR+_EBP]

  movq [DST_PTR+0], mm0
  movq [DST_PTR+8], mm2

  add DST_PTR, _EBP
  dec TMP0
  jg .Loop

%if (%2==0) && (%1==0)
  EPILOG_NO_AVRG
%else
  EPILOG_AVRG
%endif

%endmacro


;//////////////////////////////////////////////////////////////////////

%macro H_PASS_8  2   ; %1:src-op (0=NONE,1=AVRG,2=AVRG-UP), %2:dst-op (NONE/AVRG)

%if (%2==0) && (%1==0)
  PROLOG_NO_AVRG
%else
  PROLOG_AVRG
%endif

.Loop:
    ;  mm0..mm3 serves as a 4x4 delay line

%ifndef USE_TABLES

  LOAD 0, 8  ; special case for 1rst/last pixel
  ACCUM2 1,  FIR_R1,  mm0, mm3
  ACCUM2 2,  FIR_R2,  mm0, mm3
  ACCUM2 3,  FIR_R3,  mm0, mm3
  ACCUM2 4,  FIR_R4,  mm0, mm3

  ACCUM2 5,  FIR_R13,  mm0, mm3
  ACCUM2 6,  FIR_R14,  mm0, mm3
  ACCUM2 7,  FIR_R15,  mm0, mm3

%else

%if 0   ; test with no unrolling

  TLOAD 0, 8  ; special case for 1rst/last pixel
  TACCUM2 1,  xvid_FIR_23_19_6_3, xvid_FIR_1_0_0_0  , mm0, mm3
  TACCUM2 2,  xvid_FIR_7_20_20_6, xvid_FIR_3_1_0_0  , mm0, mm3
  TACCUM2 3,  xvid_FIR_3_6_20_20, xvid_FIR_6_3_1_0  , mm0, mm3
  TACCUM2 4,  xvid_FIR_1_3_6_20 , xvid_FIR_20_6_3_1 , mm0, mm3
  TACCUM2 5,  xvid_FIR_0_1_3_6  , xvid_FIR_20_20_6_3, mm0, mm3
  TACCUM2 6,  xvid_FIR_0_0_1_3  , xvid_FIR_6_20_20_7, mm0, mm3
  TACCUM2 7,  xvid_FIR_0_0_0_1  , xvid_FIR_3_6_19_23, mm0, mm3

%else  ; test with unrolling (little faster, but not much)

  movzx _EAX, byte [SRC_PTR]
  movzx TMP1, byte [SRC_PTR+8]
  XVID_MOVQ mm0, xvid_FIR_14_3_2_1, _EAX*8
  movzx _EAX, byte [SRC_PTR+1]
  XVID_MOVQ mm3, xvid_FIR_1_2_3_14, TMP1*8
  paddw mm0, mm7
  paddw mm3, mm7

  movzx TMP1, byte [SRC_PTR+2]
  XVID_PADDW mm0, xvid_FIR_23_19_6_3, _EAX*8
  XVID_PADDW mm3, xvid_FIR_1_0_0_0, _EAX*8

  movzx _EAX, byte [SRC_PTR+3]
  XVID_PADDW mm0, xvid_FIR_7_20_20_6, TMP1*8
  XVID_PADDW mm3, xvid_FIR_3_1_0_0, TMP1*8

  movzx TMP1, byte [SRC_PTR+4]
  XVID_PADDW mm0, xvid_FIR_3_6_20_20, _EAX*8
  XVID_PADDW mm3, xvid_FIR_6_3_1_0, _EAX*8

  movzx _EAX, byte [SRC_PTR+5]
  XVID_PADDW mm0, xvid_FIR_1_3_6_20, TMP1*8
  XVID_PADDW mm3, xvid_FIR_20_6_3_1, TMP1*8

  movzx TMP1, byte [SRC_PTR+6]
  XVID_PADDW mm0, xvid_FIR_0_1_3_6, _EAX*8
  XVID_PADDW mm3, xvid_FIR_20_20_6_3, _EAX*8

  movzx _EAX, byte [SRC_PTR+7]
  XVID_PADDW mm0, xvid_FIR_0_0_1_3, TMP1*8
  XVID_PADDW mm3, xvid_FIR_6_20_20_7, TMP1*8

  XVID_PADDW mm0, xvid_FIR_0_0_0_1, _EAX*8
  XVID_PADDW mm3, xvid_FIR_3_6_19_23, _EAX*8

%endif

%endif    ; !USE_TABLES

  psraw mm0, 5
  psraw mm3, 5
  packuswb mm0, mm3

%if (%1==1)
  MIX mm0, SRC_PTR, _EBX
%elif (%1==2)
  MIX mm0, SRC_PTR+1, _EBX
%endif
%if (%2==1)
  MIX mm0, DST_PTR, Rounder1_MMX
%endif

  movq [DST_PTR], mm0

  add DST_PTR, _EBP
  add SRC_PTR, _EBP
  dec TMP0
  jg .Loop

%if (%2==0) && (%1==0)
  EPILOG_NO_AVRG
%else
  EPILOG_AVRG
%endif

%endmacro

;//////////////////////////////////////////////////////////////////////
;// 16x? copy Functions

xvid_H_Pass_16_mmx:
  H_PASS_16 0, 0
ENDFUNC
xvid_H_Pass_Avrg_16_mmx:
  H_PASS_16 1, 0
ENDFUNC
xvid_H_Pass_Avrg_Up_16_mmx:
  H_PASS_16 2, 0
ENDFUNC

;//////////////////////////////////////////////////////////////////////
;// 8x? copy Functions

xvid_H_Pass_8_mmx:
  H_PASS_8 0, 0
ENDFUNC
xvid_H_Pass_Avrg_8_mmx:
  H_PASS_8 1, 0
ENDFUNC
xvid_H_Pass_Avrg_Up_8_mmx:
  H_PASS_8 2, 0
ENDFUNC

;//////////////////////////////////////////////////////////////////////
;// 16x? avrg Functions

xvid_H_Pass_Add_16_mmx:
  H_PASS_16 0, 1
ENDFUNC
xvid_H_Pass_Avrg_Add_16_mmx:
  H_PASS_16 1, 1
ENDFUNC
xvid_H_Pass_Avrg_Up_Add_16_mmx:
  H_PASS_16 2, 1
ENDFUNC

;//////////////////////////////////////////////////////////////////////
;// 8x? avrg Functions

xvid_H_Pass_8_Add_mmx:
  H_PASS_8 0, 1
ENDFUNC
xvid_H_Pass_Avrg_8_Add_mmx:
  H_PASS_8 1, 1
ENDFUNC
xvid_H_Pass_Avrg_Up_8_Add_mmx:
  H_PASS_8 2, 1
ENDFUNC



;//////////////////////////////////////////////////////////////////////
;//
;// All vertical passes
;//
;//////////////////////////////////////////////////////////////////////

%macro V_LOAD 1  ; %1=Last?

  movd mm4, dword [TMP1]
  pxor mm6, mm6
%if (%1==0)
  add TMP1, _EBP
%endif
  punpcklbw mm4, mm6

%endmacro

%macro V_ACC1 2   ; %1:reg; 2:tap
  pmullw mm4, [%2]
  paddw %1, mm4
%endmacro

%macro V_ACC2 4   ; %1-%2: regs, %3-%4: taps
  movq mm5, mm4
  movq mm6, mm4
  pmullw mm5, [%3]
  pmullw mm6, [%4]
  paddw %1, mm5
  paddw %2, mm6
%endmacro

%macro V_ACC2l 4   ; %1-%2: regs, %3-%4: taps
  movq mm5, mm4
  pmullw mm5, [%3]
  pmullw mm4, [%4]
  paddw %1, mm5
  paddw %2, mm4
%endmacro

%macro V_ACC4 8   ; %1-%4: regs, %5-%8: taps
  V_ACC2 %1,%2, %5,%6
  V_ACC2l %3,%4, %7,%8
%endmacro


%macro V_MIX 3  ; %1:dst-reg, %2:src, %3: rounder
  pxor mm6, mm6
  movq mm4, [%2]
  punpcklbw %1, mm6
  punpcklbw mm4, mm6
  paddusw %1, mm4
  paddusw %1, [%3]
  psrlw %1, 1
  packuswb %1, %1
%endmacro

%macro V_STORE 4    ; %1-%2: mix ops, %3: reg, %4:last?

  psraw %3, 5
  packuswb %3, %3

%if (%1==1)
  V_MIX %3, SRC_PTR, _EBX
  add SRC_PTR, _EBP
%elif (%1==2)
  add SRC_PTR, _EBP
  V_MIX %3, SRC_PTR, _EBX
%endif
%if (%2==1)
  V_MIX %3, DST_PTR, Rounder1_MMX
%endif

  movd eax, %3
  mov dword [DST_PTR], eax 

%if (%4==0)
  add DST_PTR, _EBP
%endif

%endmacro

;//////////////////////////////////////////////////////////////////////

%macro V_PASS_16  2   ; %1:src-op (0=NONE,1=AVRG,2=AVRG-UP), %2:dst-op (NONE/AVRG)

%if (%2==0) && (%1==0)
  PROLOG_NO_AVRG
%else
  PROLOG_AVRG
%endif

    ; we process one stripe of 4x16 pixel each time.
    ; the size (3rd argument) is meant to be a multiple of 4
    ;  mm0..mm3 serves as a 4x4 delay line

.Loop:

  push DST_PTR
  push SRC_PTR      ; SRC_PTR is preserved for src-mixing
  mov TMP1, SRC_PTR

    ; ouput rows [0..3], from input rows [0..8]

  movq mm0, mm7
  movq mm1, mm7
  movq mm2, mm7
  movq mm3, mm7

  V_LOAD 0
  V_ACC4  mm0, mm1, mm2, mm3, FIR_C14, FIR_Cm3, FIR_C2,  FIR_Cm1
  V_LOAD 0
  V_ACC4  mm0, mm1, mm2, mm3, FIR_C23, FIR_C19, FIR_Cm6, FIR_C3
  V_LOAD 0
  V_ACC4  mm0, mm1, mm2, mm3, FIR_Cm7, FIR_C20, FIR_C20, FIR_Cm6
  V_LOAD 0
  V_ACC4  mm0, mm1, mm2, mm3, FIR_C3,  FIR_Cm6, FIR_C20, FIR_C20
  V_LOAD 0
  V_ACC4  mm0, mm1, mm2, mm3, FIR_Cm1, FIR_C3,  FIR_Cm6, FIR_C20
  V_STORE %1, %2, mm0, 0

  V_LOAD 0
  V_ACC2 mm1, mm2, FIR_Cm1,  FIR_C3
  V_ACC1 mm3, FIR_Cm6
  V_STORE %1, %2, mm1, 0

  V_LOAD 0
  V_ACC2l mm2, mm3, FIR_Cm1, FIR_C3
  V_STORE %1, %2, mm2, 0

  V_LOAD 1
  V_ACC1 mm3, FIR_Cm1
  V_STORE %1, %2, mm3, 0

    ; ouput rows [4..7], from input rows [1..11] (!!)

  mov SRC_PTR, [_ESP]
  lea TMP1, [SRC_PTR+_EBP]

  lea SRC_PTR, [SRC_PTR+4*_EBP]  ; for src-mixing
  push SRC_PTR              ; this will be the new value for next round

  movq mm0, mm7
  movq mm1, mm7
  movq mm2, mm7
  movq mm3, mm7

  V_LOAD 0
  V_ACC1 mm0, FIR_Cm1

  V_LOAD 0
  V_ACC2l mm0, mm1, FIR_C3,  FIR_Cm1

  V_LOAD 0
  V_ACC2 mm0, mm1, FIR_Cm6,  FIR_C3
  V_ACC1 mm2, FIR_Cm1

  V_LOAD 0
  V_ACC4  mm0, mm1, mm2, mm3, FIR_C20, FIR_Cm6, FIR_C3, FIR_Cm1
  V_LOAD 0
  V_ACC4  mm0, mm1, mm2, mm3, FIR_C20, FIR_C20, FIR_Cm6, FIR_C3
  V_LOAD 0
  V_ACC4  mm0, mm1, mm2, mm3, FIR_Cm6, FIR_C20, FIR_C20, FIR_Cm6
  V_LOAD 0
  V_ACC4  mm0, mm1, mm2, mm3, FIR_C3,  FIR_Cm6, FIR_C20, FIR_C20
  V_LOAD 0
  V_ACC4  mm0, mm1, mm2, mm3, FIR_Cm1, FIR_C3,  FIR_Cm6, FIR_C20
  V_STORE %1, %2, mm0, 0

  V_LOAD 0
  V_ACC2 mm1, mm2, FIR_Cm1,  FIR_C3
  V_ACC1 mm3, FIR_Cm6
  V_STORE %1, %2, mm1, 0

  V_LOAD 0
  V_ACC2l mm2, mm3, FIR_Cm1, FIR_C3
  V_STORE %1, %2, mm2, 0

  V_LOAD 1
  V_ACC1 mm3, FIR_Cm1
  V_STORE %1, %2, mm3, 0

    ; ouput rows [8..11], from input rows [5..15]

  pop SRC_PTR
  lea TMP1, [SRC_PTR+_EBP]

  lea SRC_PTR, [SRC_PTR+4*_EBP]  ; for src-mixing
  push SRC_PTR              ; this will be the new value for next round

  movq mm0, mm7
  movq mm1, mm7
  movq mm2, mm7
  movq mm3, mm7

  V_LOAD 0
  V_ACC1 mm0, FIR_Cm1

  V_LOAD 0
  V_ACC2l mm0, mm1, FIR_C3,  FIR_Cm1

  V_LOAD 0
  V_ACC2 mm0, mm1, FIR_Cm6,  FIR_C3
  V_ACC1 mm2, FIR_Cm1

  V_LOAD 0
  V_ACC4  mm0, mm1, mm2, mm3, FIR_C20, FIR_Cm6, FIR_C3, FIR_Cm1
  V_LOAD 0
  V_ACC4  mm0, mm1, mm2, mm3, FIR_C20, FIR_C20, FIR_Cm6, FIR_C3
  V_LOAD 0
  V_ACC4  mm0, mm1, mm2, mm3, FIR_Cm6, FIR_C20, FIR_C20, FIR_Cm6
  V_LOAD 0
  V_ACC4  mm0, mm1, mm2, mm3, FIR_C3,  FIR_Cm6, FIR_C20, FIR_C20
  V_LOAD 0
  V_ACC4  mm0, mm1, mm2, mm3, FIR_Cm1, FIR_C3,  FIR_Cm6, FIR_C20

  V_STORE %1, %2, mm0, 0

  V_LOAD 0
  V_ACC2 mm1, mm2, FIR_Cm1,  FIR_C3
  V_ACC1 mm3, FIR_Cm6
  V_STORE %1, %2, mm1, 0

  V_LOAD 0
  V_ACC2l mm2, mm3, FIR_Cm1, FIR_C3
  V_STORE %1, %2, mm2, 0

  V_LOAD 1
  V_ACC1 mm3, FIR_Cm1
  V_STORE %1, %2, mm3, 0


    ; ouput rows [12..15], from input rows [9.16]

  pop SRC_PTR
  lea TMP1, [SRC_PTR+_EBP]

%if (%1!=0)
  lea SRC_PTR, [SRC_PTR+4*_EBP]  ; for src-mixing
%endif

  movq mm0, mm7
  movq mm1, mm7
  movq mm2, mm7
  movq mm3, mm7

  V_LOAD 0
  V_ACC1 mm3, FIR_Cm1

  V_LOAD 0
  V_ACC2l mm2, mm3, FIR_Cm1,  FIR_C3

  V_LOAD 0
  V_ACC2 mm1, mm2, FIR_Cm1,  FIR_C3
  V_ACC1 mm3, FIR_Cm6

  V_LOAD 0
  V_ACC4  mm0, mm1, mm2, mm3, FIR_Cm1, FIR_C3,  FIR_Cm6, FIR_C20
  V_LOAD 0
  V_ACC4  mm0, mm1, mm2, mm3, FIR_C3,  FIR_Cm6, FIR_C20, FIR_C20
  V_LOAD 0
  V_ACC4  mm0, mm1, mm2, mm3, FIR_Cm7, FIR_C20, FIR_C20, FIR_Cm6
  V_LOAD 0
  V_ACC4  mm0, mm1, mm2, mm3, FIR_C23, FIR_C19, FIR_Cm6, FIR_C3
  V_LOAD 1
  V_ACC4  mm0, mm1, mm2, mm3, FIR_C14, FIR_Cm3, FIR_C2, FIR_Cm1

  V_STORE %1, %2, mm3, 0
  V_STORE %1, %2, mm2, 0
  V_STORE %1, %2, mm1, 0
  V_STORE %1, %2, mm0, 1

    ; ... next 4 columns

  pop SRC_PTR
  pop DST_PTR
  add SRC_PTR, 4
  add DST_PTR, 4
  sub TMP0, 4
  jg .Loop

%if (%2==0) && (%1==0)
  EPILOG_NO_AVRG
%else
  EPILOG_AVRG
%endif

%endmacro

;//////////////////////////////////////////////////////////////////////

%macro V_PASS_8  2   ; %1:src-op (0=NONE,1=AVRG,2=AVRG-UP), %2:dst-op (NONE/AVRG)

%if (%2==0) && (%1==0)
  PROLOG_NO_AVRG
%else
  PROLOG_AVRG
%endif

    ; we process one stripe of 4x8 pixel each time
    ; the size (3rd argument) is meant to be a multiple of 4
    ;  mm0..mm3 serves as a 4x4 delay line
.Loop:

  push DST_PTR
  push SRC_PTR      ; SRC_PTR is preserved for src-mixing
  mov TMP1, SRC_PTR

    ; ouput rows [0..3], from input rows [0..8]

  movq mm0, mm7
  movq mm1, mm7
  movq mm2, mm7
  movq mm3, mm7

  V_LOAD 0
  V_ACC4  mm0, mm1, mm2, mm3, FIR_C14, FIR_Cm3, FIR_C2,  FIR_Cm1
  V_LOAD 0
  V_ACC4  mm0, mm1, mm2, mm3, FIR_C23, FIR_C19, FIR_Cm6, FIR_C3
  V_LOAD 0
  V_ACC4  mm0, mm1, mm2, mm3, FIR_Cm7, FIR_C20, FIR_C20, FIR_Cm6
  V_LOAD 0
  V_ACC4  mm0, mm1, mm2, mm3, FIR_C3,  FIR_Cm6, FIR_C20, FIR_C20
  V_LOAD 0
  V_ACC4  mm0, mm1, mm2, mm3, FIR_Cm1, FIR_C3,  FIR_Cm6, FIR_C20
  V_STORE %1, %2, mm0, 0

  V_LOAD 0
  V_ACC2 mm1, mm2, FIR_Cm1,  FIR_C3
  V_ACC1 mm3, FIR_Cm6

  V_STORE %1, %2, mm1, 0

  V_LOAD 0
  V_ACC2l mm2, mm3, FIR_Cm1,  FIR_C3
  V_STORE %1, %2, mm2, 0

  V_LOAD 1
  V_ACC1 mm3, FIR_Cm1
  V_STORE %1, %2, mm3, 0

    ; ouput rows [4..7], from input rows [1..9]

  mov SRC_PTR, [_ESP]
  lea TMP1, [SRC_PTR+_EBP]

%if (%1!=0)
  lea SRC_PTR, [SRC_PTR+4*_EBP]  ; for src-mixing
%endif

  movq mm0, mm7
  movq mm1, mm7
  movq mm2, mm7
  movq mm3, mm7

  V_LOAD 0
  V_ACC1 mm3, FIR_Cm1

  V_LOAD 0
  V_ACC2l mm2, mm3, FIR_Cm1,  FIR_C3

  V_LOAD 0
  V_ACC2 mm1, mm2, FIR_Cm1,  FIR_C3
  V_ACC1 mm3, FIR_Cm6

  V_LOAD 0
  V_ACC4  mm0, mm1, mm2, mm3, FIR_Cm1, FIR_C3,  FIR_Cm6, FIR_C20
  V_LOAD 0
  V_ACC4  mm0, mm1, mm2, mm3, FIR_C3,  FIR_Cm6, FIR_C20, FIR_C20
  V_LOAD 0
  V_ACC4  mm0, mm1, mm2, mm3, FIR_Cm7, FIR_C20, FIR_C20, FIR_Cm6
  V_LOAD 0
  V_ACC4  mm0, mm1, mm2, mm3, FIR_C23, FIR_C19, FIR_Cm6, FIR_C3
  V_LOAD 1
  V_ACC4  mm0, mm1, mm2, mm3, FIR_C14, FIR_Cm3, FIR_C2, FIR_Cm1

  V_STORE %1, %2, mm3, 0
  V_STORE %1, %2, mm2, 0
  V_STORE %1, %2, mm1, 0
  V_STORE %1, %2, mm0, 1

    ; ... next 4 columns

  pop SRC_PTR
  pop DST_PTR
  add SRC_PTR, 4
  add DST_PTR, 4
  sub TMP0, 4
  jg .Loop

%if (%2==0) && (%1==0)
  EPILOG_NO_AVRG
%else
  EPILOG_AVRG
%endif

%endmacro


;//////////////////////////////////////////////////////////////////////
;// 16x? copy Functions

xvid_V_Pass_16_mmx:
  V_PASS_16 0, 0
ENDFUNC
xvid_V_Pass_Avrg_16_mmx:
  V_PASS_16 1, 0
ENDFUNC
xvid_V_Pass_Avrg_Up_16_mmx:
  V_PASS_16 2, 0
ENDFUNC

;//////////////////////////////////////////////////////////////////////
;// 8x? copy Functions

xvid_V_Pass_8_mmx:
  V_PASS_8 0, 0
ENDFUNC
xvid_V_Pass_Avrg_8_mmx:
  V_PASS_8 1, 0
ENDFUNC
xvid_V_Pass_Avrg_Up_8_mmx:
  V_PASS_8 2, 0
ENDFUNC

;//////////////////////////////////////////////////////////////////////
;// 16x? avrg Functions

xvid_V_Pass_Add_16_mmx:
  V_PASS_16 0, 1
ENDFUNC
xvid_V_Pass_Avrg_Add_16_mmx:
  V_PASS_16 1, 1
ENDFUNC
xvid_V_Pass_Avrg_Up_Add_16_mmx:
  V_PASS_16 2, 1
ENDFUNC

;//////////////////////////////////////////////////////////////////////
;// 8x? avrg Functions

xvid_V_Pass_8_Add_mmx:
  V_PASS_8 0, 1
ENDFUNC
xvid_V_Pass_Avrg_8_Add_mmx:
  V_PASS_8 1, 1
ENDFUNC
xvid_V_Pass_Avrg_Up_8_Add_mmx:
  V_PASS_8 2, 1
ENDFUNC

;//////////////////////////////////////////////////////////////////////

%undef SRC_PTR
%undef DST_PTR

NON_EXEC_STACK
