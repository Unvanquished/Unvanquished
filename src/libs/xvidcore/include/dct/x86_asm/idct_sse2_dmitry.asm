;/****************************************************************************
; *
; *  XVID MPEG-4 VIDEO CODEC
; *  - SSE2 inverse discrete cosine transform -
; *
; *  Copyright(C) 2002 Dmitry Rozhdestvensky
; *
; *  This program is free software; you can redistribute it and/or modify it
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
; * $Id: idct_sse2_dmitry.asm,v 1.8.2.3 2009/09/16 17:11:39 Isibaar Exp $
; *
; ***************************************************************************/

;=============================================================================
; Macros and other preprocessor constants
;=============================================================================

%include "nasm.inc"

%define BITS_INV_ACC    5                           ; 4 or 5 for IEEE
%define SHIFT_INV_ROW   16 - BITS_INV_ACC
%define SHIFT_INV_COL   1 + BITS_INV_ACC
%define RND_INV_ROW     1024 * (6 - BITS_INV_ACC)   ; 1 << (SHIFT_INV_ROW-1)
%define RND_INV_COL     16 * (BITS_INV_ACC - 3)     ; 1 << (SHIFT_INV_COL-1)
%define RND_INV_CORR    RND_INV_COL - 1             ; correction -1.0 and round

%define BITS_FRW_ACC    3                           ; 2 or 3 for accuracy
%define SHIFT_FRW_COL   BITS_FRW_ACC
%define SHIFT_FRW_ROW   BITS_FRW_ACC + 17
%define RND_FRW_ROW     262144 * (BITS_FRW_ACC - 1) ; 1 << (SHIFT_FRW_ROW-1)
	
;=============================================================================
; Local Data (Read Only)
;=============================================================================

DATA

ALIGN SECTION_ALIGN
tab_i_04:
  dw  16384,  21407,  16384,   8867 ; movq-> w05 w04 w01 w00
  dw  16384,  -8867,  16384, -21407 ; w13 w12 w09 w08
  dw  16384,   8867, -16384, -21407 ; w07 w06 w03 w02
  dw -16384,  21407,  16384,  -8867 ; w15 w14 w11 w10
  dw  22725,  19266,  19266,  -4520 ; w21 w20 w17 w16
  dw  12873, -22725,   4520, -12873 ; w29 w28 w25 w24
  dw  12873,   4520, -22725, -12873 ; w23 w22 w19 w18
  dw   4520,  19266,  19266, -22725 ; w31 w30 w27 w26

; Table for rows 1,7 - constants are multiplied by cos_1_16
tab_i_17:
  dw  22725,  29692,  22725,  12299 ; movq-> w05 w04 w01 w00
  dw  22725, -12299,  22725, -29692 ; w13 w12 w09 w08
  dw  22725,  12299, -22725, -29692 ; w07 w06 w03 w02
  dw -22725,  29692,  22725, -12299 ; w15 w14 w11 w10
  dw  31521,  26722,  26722,  -6270 ; w21 w20 w17 w16
  dw  17855, -31521,   6270, -17855 ; w29 w28 w25 w24
  dw  17855,   6270, -31521, -17855 ; w23 w22 w19 w18
  dw   6270,  26722,  26722, -31521 ; w31 w30 w27 w26

; Table for rows 2,6 - constants are multiplied by cos_2_16
tab_i_26:
  dw  21407,  27969,  21407,  11585 ; movq-> w05 w04 w01 w00
  dw  21407, -11585,  21407, -27969 ; w13 w12 w09 w08
  dw  21407,  11585, -21407, -27969 ; w07 w06 w03 w02
  dw -21407,  27969,  21407, -11585 ; w15 w14 w11 w10
  dw  29692,  25172,  25172,  -5906 ; w21 w20 w17 w16
  dw  16819, -29692,   5906, -16819 ; w29 w28 w25 w24
  dw  16819,   5906, -29692, -16819 ; w23 w22 w19 w18
  dw   5906,  25172,  25172, -29692 ; w31 w30 w27 w26

; Table for rows 3,5 - constants are multiplied by cos_3_16
tab_i_35:
  dw  19266,  25172,  19266,  10426 ; movq-> w05 w04 w01 w00
  dw  19266, -10426,  19266, -25172 ; w13 w12 w09 w08
  dw  19266,  10426, -19266, -25172 ; w07 w06 w03 w02
  dw -19266,  25172,  19266, -10426 ; w15 w14 w11 w10
  dw  26722,  22654,  22654,  -5315 ; w21 w20 w17 w16
  dw  15137, -26722,   5315, -15137 ; w29 w28 w25 w24
  dw  15137,   5315, -26722, -15137 ; w23 w22 w19 w18
  dw   5315,  22654,  22654, -26722 ; w31 w30 w27 w26

%if SHIFT_INV_ROW == 12   ; assume SHIFT_INV_ROW == 12
rounder_2_0: dd  65536, 65536
             dd  65536, 65536
rounder_2_4: dd      0,     0
             dd      0,     0
rounder_2_1: dd   7195,  7195
             dd   7195,  7195
rounder_2_7: dd   1024,  1024
             dd   1024,  1024
rounder_2_2: dd   4520,  4520
             dd   4520,  4520
rounder_2_6: dd   1024,  1024
             dd   1024,  1024
rounder_2_3: dd   2407,  2407
             dd   2407,  2407
rounder_2_5: dd    240,   240
             dd    240,   240

%elif SHIFT_INV_ROW == 11   ; assume SHIFT_INV_ROW == 11
rounder_2_0: dd  65536, 65536
             dd  65536, 65536
rounder_2_4: dd      0,     0
             dd      0,     0
rounder_2_1: dd   3597,  3597
             dd   3597,  3597
rounder_2_7: dd    512,   512
             dd    512,   512
rounder_2_2: dd   2260,  2260
             dd   2260,  2260
rounder_2_6: dd    512,   512
             dd    512,   512
rounder_2_3: dd   1203,  1203
             dd   1203,  1203
rounder_2_5: dd    120,   120
             dd    120,   120
%else

%error Invalid SHIFT_INV_ROW specified

%endif

tg_1_16: dw  13036,  13036,  13036,  13036      ; tg * (2<<16) + 0.5
         dw  13036,  13036,  13036,  13036
tg_2_16: dw  27146,  27146,  27146,  27146      ; tg * (2<<16) + 0.5
         dw  27146,  27146,  27146,  27146
tg_3_16: dw -21746, -21746, -21746, -21746      ; tg * (2<<16) + 0.5
         dw -21746, -21746, -21746, -21746
ocos_4_16: dw  23170,  23170,  23170,  23170    ; cos * (2<<15) + 0.5
           dw  23170,  23170,  23170,  23170

;=============================================================================
; Code
;=============================================================================

TEXT

cglobal idct_sse2_dmitry

;-----------------------------------------------------------------------------
; Helper macro - ROW iDCT
;-----------------------------------------------------------------------------

%macro DCT_8_INV_ROW_1_SSE2  4
  pshufhw xmm1, [%1], 11011000b     ;x 75643210
  pshuflw xmm1, xmm1, 11011000b     ;x 75643120
  pshufd xmm0, xmm1, 00000000b      ;x 20202020
  pmaddwd xmm0, [%3]                ;w 13 12 9 8 5410

  ;a 3210 first part
  pshufd xmm2, xmm1, 10101010b      ;x 64646464
  pmaddwd xmm2, [%3+16]             ;w 15 14 11 10 7632

  ;a 3210 second part
  paddd xmm2, xmm0                  ;a 3210 ready
  paddd xmm2, [%4]                  ;must be 4 dwords long, not 2 as for sse1
  movdqa xmm5, xmm2

  pshufd xmm3, xmm1, 01010101b      ;x 31313131
  pmaddwd xmm3, [%3+32]             ;w 29 28 25 24 21 20 17 16

  ;b 3210 first part
  pshufd xmm4, xmm1, 11111111b      ;x 75757575
  pmaddwd xmm4, [%3+48]             ;w 31 30 27 26 23 22 19 18

  ;b 3210 second part
  paddd xmm3,xmm4                   ;b 3210 ready

  paddd xmm2, xmm3                  ;will be y 3210
  psubd xmm5, xmm3                  ;will be y 4567
  psrad xmm2, SHIFT_INV_ROW
  psrad xmm5, SHIFT_INV_ROW
  packssdw xmm2, xmm5               ;y 45673210
  pshufhw xmm6, xmm2, 00011011b     ;y 76543210
  movdqa [%2], xmm6        
%endmacro

;-----------------------------------------------------------------------------
; Helper macro - Columns iDCT
;-----------------------------------------------------------------------------

%macro DCT_8_INV_COL_4_SSE2 2
  movdqa xmm0, [%1+16*0]          	;x0 (all columns)
  movdqa xmm2, [%1+16*4]          	;x4
  movdqa xmm1, xmm0

  movdqa xmm4, [%1+16*2]          	;x2
  movdqa xmm5, [%1+16*6]          	;x6
  movdqa xmm6, [tg_2_16]
  movdqa xmm7, xmm6

  paddsw xmm0, xmm2                  ;u04=x0+x4
  psubsw xmm1, xmm2                  ;v04=x0-x4
  movdqa xmm3, xmm0
  movdqa xmm2, xmm1

  pmulhw xmm6, xmm4
  pmulhw xmm7, xmm5
  psubsw xmm6, xmm5                  ;v26=x2*T2-x6
  paddsw xmm7, xmm4                  ;u26=x6*T2+x2

  paddsw xmm1, xmm6                  ;a1=v04+v26
  paddsw xmm0, xmm7                  ;a0=u04+u26
  psubsw xmm2, xmm6                  ;a2=v04-v26
  psubsw xmm3, xmm7                  ;a3=u04-u26

  movdqa [%2+16*0], xmm0          	;store a3-a0 to 
  movdqa [%2+16*6], xmm1          	;free registers
  movdqa [%2+16*2], xmm2
  movdqa [%2+16*4], xmm3

  movdqa xmm0, [%1+16*1]          	;x1
  movdqa xmm1, [%1+16*7]          	;x7
  movdqa xmm2, [tg_1_16]
  movdqa xmm3, xmm2

  movdqa xmm4, [%1+16*3]          	;x3
  movdqa xmm5, [%1+16*5]          	;x5
  movdqa xmm6, [tg_3_16]
  movdqa xmm7, xmm6

  pmulhw xmm2, xmm0
  pmulhw xmm3, xmm1
  psubsw xmm2, xmm1                  ;v17=x1*T1-x7
  paddsw xmm3, xmm0                  ;u17=x7*T1+x1
  movdqa xmm0, xmm3                  ;u17
  movdqa xmm1, xmm2                  ;v17

  pmulhw xmm6, xmm4                  ;x3*(t3-1)
  pmulhw xmm7, xmm5                  ;x5*(t3-1)
  paddsw xmm6, xmm4
  paddsw xmm7, xmm5
  psubsw xmm6, xmm5                  ;v35=x3*T3-x5
  paddsw xmm7, xmm4                  ;u35=x5*T3+x3

  movdqa xmm4, [ocos_4_16]

  paddsw xmm0, xmm7                 ;b0=u17+u35
  psubsw xmm1, xmm6                 ;b3=v17-v35
  psubsw xmm3, xmm7                 ;u12=u17-v35
  paddsw xmm2, xmm6                 ;v12=v17+v35

  movdqa xmm5, xmm3
  paddsw xmm3, xmm2                 ;tb1
  psubsw xmm5, xmm2                 ;tb2
  pmulhw xmm5, xmm4
  pmulhw xmm4, xmm3
  paddsw xmm5, xmm5
  paddsw xmm4, xmm4

  movdqa xmm6, [%2+16*0]          	;a0
  movdqa xmm7, xmm6
  movdqa xmm2, [%2+16*4]          	;a3
  movdqa xmm3, xmm2

  paddsw xmm6, xmm0
  psubsw xmm7, xmm0
  psraw xmm6, SHIFT_INV_COL      	;y0=a0+b0
  psraw xmm7, SHIFT_INV_COL      	;y7=a0-b0
  movdqa [%2+16*0], xmm6
  movdqa [%2+16*7], xmm7

  paddsw xmm2, xmm1
  psubsw xmm3, xmm1
  psraw xmm2, SHIFT_INV_COL      	;y3=a3+b3
  psraw xmm3, SHIFT_INV_COL      	;y4=a3-b3
  movdqa [%2+16*3], xmm2
  movdqa [%2+16*4], xmm3

  movdqa xmm0, [%2+16*6]          	;a1
  movdqa xmm1, xmm0
  movdqa xmm6, [%2+16*2]          	;a2
  movdqa xmm7, xmm6

  
  paddsw xmm0, xmm4
  psubsw xmm1, xmm4
  psraw xmm0, SHIFT_INV_COL      	;y1=a1+b1
  psraw xmm1, SHIFT_INV_COL      	;y6=a1-b1
  movdqa [%2+16*1], xmm0
  movdqa [%2+16*6], xmm1

  paddsw xmm6, xmm5
  psubsw xmm7, xmm5
  psraw xmm6, SHIFT_INV_COL      	;y2=a2+b2
  psraw xmm7, SHIFT_INV_COL      	;y5=a2-b2
  movdqa [%2+16*2], xmm6
  movdqa [%2+16*5], xmm7
%endmacro

;-----------------------------------------------------------------------------
; void idct_sse2_dmitry(int16_t coeff[64]);
;-----------------------------------------------------------------------------

ALIGN SECTION_ALIGN
idct_sse2_dmitry:
  PUSH_XMM6_XMM7

  mov _ECX, prm1

  DCT_8_INV_ROW_1_SSE2 _ECX+  0, _ECX+  0, tab_i_04, rounder_2_0
  DCT_8_INV_ROW_1_SSE2 _ECX+ 16, _ECX+ 16, tab_i_17, rounder_2_1
  DCT_8_INV_ROW_1_SSE2 _ECX+ 32, _ECX+ 32, tab_i_26, rounder_2_2
  DCT_8_INV_ROW_1_SSE2 _ECX+ 48, _ECX+ 48, tab_i_35, rounder_2_3
  DCT_8_INV_ROW_1_SSE2 _ECX+ 64, _ECX+ 64, tab_i_04, rounder_2_4
  DCT_8_INV_ROW_1_SSE2 _ECX+ 80, _ECX+ 80, tab_i_35, rounder_2_5
  DCT_8_INV_ROW_1_SSE2 _ECX+ 96, _ECX+ 96, tab_i_26, rounder_2_6
  DCT_8_INV_ROW_1_SSE2 _ECX+112, _ECX+112, tab_i_17, rounder_2_7

  DCT_8_INV_COL_4_SSE2 _ECX, _ECX

  POP_XMM6_XMM7
  ret
ENDFUNC

NON_EXEC_STACK
