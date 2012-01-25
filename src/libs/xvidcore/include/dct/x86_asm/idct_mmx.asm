;/****************************************************************************
; *
; *  XVID MPEG-4 VIDEO CODEC
; *  - MMX and XMM forward discrete cosine transform -
; *
; *  Copyright(C) 2001 Peter Ross <pross@xvid.org>
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
; * $Id: idct_mmx.asm,v 1.13.2.2 2009/09/16 17:11:39 Isibaar Exp $
; *
; ***************************************************************************/

; ****************************************************************************
;
; Originally provided by Intel at AP-922
; http://developer.intel.com/vtune/cbts/strmsimd/922down.htm
; (See more app notes at http://developer.intel.com/vtune/cbts/strmsimd/appnotes.htm)
; but in a limited edition.
; New macro implements a column part for precise iDCT
; The routine precision now satisfies IEEE standard 1180-1990.
;
; Copyright(C) 2000-2001 Peter Gubanov <peter@elecard.net.ru>
; Rounding trick Copyright(C) 2000 Michel Lespinasse <walken@zoy.org>
;
; http://www.elecard.com/peter/idct.html
; http://www.linuxvideo.org/mpeg2dec/
;
; ***************************************************************************/
;
; These examples contain code fragments for first stage iDCT 8x8
; (for rows) and first stage DCT 8x8 (for columns)
;

;=============================================================================
; Macros and other preprocessor constants
;=============================================================================

%include "nasm.inc"

%define BITS_INV_ACC    5                         ; 4 or 5 for IEEE
%define SHIFT_INV_ROW   16 - BITS_INV_ACC
%define SHIFT_INV_COL   1 + BITS_INV_ACC
%define RND_INV_ROW     1024 * (6 - BITS_INV_ACC) ; 1 << (SHIFT_INV_ROW-1)
%define RND_INV_COL     16 * (BITS_INV_ACC - 3)   ; 1 << (SHIFT_INV_COL-1)
%define RND_INV_CORR    RND_INV_COL - 1           ; correction -1.0 and round

%define BITS_FRW_ACC    3                         ; 2 or 3 for accuracy
%define SHIFT_FRW_COL   BITS_FRW_ACC
%define SHIFT_FRW_ROW   BITS_FRW_ACC + 17
%define RND_FRW_ROW     262144*(BITS_FRW_ACC - 1) ; 1 << (SHIFT_FRW_ROW-1)

;=============================================================================
; Local Data (Read Only)
;=============================================================================

DATA

;-----------------------------------------------------------------------------
; Various memory constants (trigonometric values or rounding values)
;-----------------------------------------------------------------------------

ALIGN SECTION_ALIGN
one_corr:
  dw 1, 1, 1, 1
round_inv_row:
  dd RND_INV_ROW,  RND_INV_ROW
round_inv_col:
  dw RND_INV_COL,  RND_INV_COL,  RND_INV_COL, RND_INV_COL
round_inv_corr:
  dw RND_INV_CORR, RND_INV_CORR, RND_INV_CORR, RND_INV_CORR
round_frw_row:
  dd RND_FRW_ROW,  RND_FRW_ROW
tg_1_16:
  dw 13036,  13036,  13036,  13036    ; tg * (2<<16) + 0.5
tg_2_16:
  dw 27146,  27146,  27146,  27146    ; tg * (2<<16) + 0.5
tg_3_16:
  dw -21746, -21746, -21746, -21746    ; tg * (2<<16) + 0.5
cos_4_16:
  dw -19195, -19195, -19195, -19195    ; cos * (2<<16) + 0.5
ocos_4_16:
  dw 23170,  23170,  23170,  23170    ; cos * (2<<15) + 0.5
otg_3_16:
  dw 21895, 21895, 21895, 21895       ; tg * (2<<16) + 0.5

%if SHIFT_INV_ROW == 12   ; assume SHIFT_INV_ROW == 12
rounder_0:
  dd 65536, 65536
rounder_4:
  dd 0, 0
rounder_1:
  dd 7195, 7195
rounder_7
  dd 1024, 1024
rounder_2:
  dd 4520, 4520
rounder_6:
  dd 1024, 1024
rounder_3:
  dd 2407, 2407
rounder_5:
  dd 240, 240

%elif SHIFT_INV_ROW == 11   ; assume SHIFT_INV_ROW == 11
rounder_0:
  dd 65536, 65536
rounder_4:
  dd 0, 0
rounder_1:
  dd 3597, 3597
rounder_7:
  dd 512, 512
rounder_2:
  dd 2260, 2260
rounder_6:
  dd 512, 512
rounder_3:
  dd 1203, 1203
rounder_5:
  dd 120, 120
%else

%error invalid SHIFT_INV_ROW

%endif

;-----------------------------------------------------------------------------
;
; The first stage iDCT 8x8 - inverse DCTs of rows
;
;-----------------------------------------------------------------------------
; The 8-point inverse DCT direct algorithm
;-----------------------------------------------------------------------------
;
; static const short w[32] = {
;	FIX(cos_4_16),  FIX(cos_2_16),  FIX(cos_4_16),  FIX(cos_6_16),
;	FIX(cos_4_16),  FIX(cos_6_16), -FIX(cos_4_16), -FIX(cos_2_16),
;	FIX(cos_4_16), -FIX(cos_6_16), -FIX(cos_4_16),  FIX(cos_2_16),
;	FIX(cos_4_16), -FIX(cos_2_16),  FIX(cos_4_16), -FIX(cos_6_16),
;	FIX(cos_1_16),  FIX(cos_3_16),  FIX(cos_5_16),  FIX(cos_7_16),
;	FIX(cos_3_16), -FIX(cos_7_16), -FIX(cos_1_16), -FIX(cos_5_16),
;	FIX(cos_5_16), -FIX(cos_1_16),  FIX(cos_7_16),  FIX(cos_3_16),
;	FIX(cos_7_16), -FIX(cos_5_16),  FIX(cos_3_16), -FIX(cos_1_16) };
;
; #define DCT_8_INV_ROW(x, y)
; {
; 	int a0, a1, a2, a3, b0, b1, b2, b3;
;
; 	a0 =x[0]*w[0]+x[2]*w[1]+x[4]*w[2]+x[6]*w[3];
; 	a1 =x[0]*w[4]+x[2]*w[5]+x[4]*w[6]+x[6]*w[7];
; 	a2 = x[0] * w[ 8] + x[2] * w[ 9] + x[4] * w[10] + x[6] * w[11];
; 	a3 = x[0] * w[12] + x[2] * w[13] + x[4] * w[14] + x[6] * w[15];
; 	b0 = x[1] * w[16] + x[3] * w[17] + x[5] * w[18] + x[7] * w[19];
; 	b1 = x[1] * w[20] + x[3] * w[21] + x[5] * w[22] + x[7] * w[23];
; 	b2 = x[1] * w[24] + x[3] * w[25] + x[5] * w[26] + x[7] * w[27];
; 	b3 = x[1] * w[28] + x[3] * w[29] + x[5] * w[30] + x[7] * w[31];
;
; 	y[0] = SHIFT_ROUND ( a0 + b0 );
; 	y[1] = SHIFT_ROUND ( a1 + b1 );
; 	y[2] = SHIFT_ROUND ( a2 + b2 );
; 	y[3] = SHIFT_ROUND ( a3 + b3 );
; 	y[4] = SHIFT_ROUND ( a3 - b3 );
; 	y[5] = SHIFT_ROUND ( a2 - b2 );
; 	y[6] = SHIFT_ROUND ( a1 - b1 );
; 	y[7] = SHIFT_ROUND ( a0 - b0 );
; }
;
;-----------------------------------------------------------------------------
;
; In this implementation the outputs of the iDCT-1D are multiplied
; 	for rows 0,4 - by cos_4_16,
; 	for rows 1,7 - by cos_1_16,
; 	for rows 2,6 - by cos_2_16,
; 	for rows 3,5 - by cos_3_16
; and are shifted to the left for better accuracy
;
; For the constants used,
; 	FIX(float_const) = (short) (float_const * (1<<15) + 0.5)
;
;-----------------------------------------------------------------------------

;-----------------------------------------------------------------------------
; Tables for mmx processors
;-----------------------------------------------------------------------------

; Table for rows 0,4 - constants are multiplied by cos_4_16
tab_i_04_mmx:
  dw  16384,  16384,  16384, -16384    ; movq-> w06 w04 w02 w00
  dw  21407,   8867,   8867, -21407    ; w07 w05 w03 w01
  dw  16384, -16384,  16384,  16384    ; w14 w12 w10 w08
  dw  -8867,  21407, -21407,  -8867    ; w15 w13 w11 w09
  dw  22725,  12873,  19266, -22725    ; w22 w20 w18 w16
  dw  19266,   4520,  -4520, -12873    ; w23 w21 w19 w17
  dw  12873,   4520,   4520,  19266    ; w30 w28 w26 w24
  dw -22725,  19266, -12873, -22725    ; w31 w29 w27 w25

; Table for rows 1,7 - constants are multiplied by cos_1_16
tab_i_17_mmx:
  dw  22725,  22725,  22725, -22725    ; movq-> w06 w04 w02 w00
  dw  29692,  12299,  12299, -29692    ; w07 w05 w03 w01
  dw  22725, -22725,  22725,  22725    ; w14 w12 w10 w08
  dw -12299,  29692, -29692, -12299    ; w15 w13 w11 w09
  dw  31521,  17855,  26722, -31521    ; w22 w20 w18 w16
  dw  26722,   6270,  -6270, -17855    ; w23 w21 w19 w17
  dw  17855,   6270,   6270,  26722    ; w30 w28 w26 w24
  dw -31521,  26722, -17855, -31521    ; w31 w29 w27 w25

; Table for rows 2,6 - constants are multiplied by cos_2_16
tab_i_26_mmx:
  dw  21407,  21407,  21407, -21407    ; movq-> w06 w04 w02 w00
  dw  27969,  11585,  11585, -27969    ; w07 w05 w03 w01
  dw  21407, -21407,  21407,  21407    ; w14 w12 w10 w08
  dw -11585,  27969, -27969, -11585    ; w15 w13 w11 w09
  dw  29692,  16819,  25172, -29692    ; w22 w20 w18 w16
  dw  25172,   5906,  -5906, -16819    ; w23 w21 w19 w17
  dw  16819,   5906,   5906,  25172    ; w30 w28 w26 w24
  dw -29692,  25172, -16819, -29692    ; w31 w29 w27 w25

; Table for rows 3,5 - constants are multiplied by cos_3_16
tab_i_35_mmx:
  dw  19266,  19266,  19266, -19266    ; movq-> w06 w04 w02 w00
  dw  25172,  10426,  10426, -25172    ; w07 w05 w03 w01
  dw  19266, -19266,  19266,  19266    ; w14 w12 w10 w08
  dw -10426,  25172, -25172, -10426    ; w15 w13 w11 w09
  dw  26722,  15137,  22654, -26722    ; w22 w20 w18 w16
  dw  22654,   5315,  -5315, -15137    ; w23 w21 w19 w17
  dw  15137,   5315,   5315,  22654    ; w30 w28 w26 w24
  dw -26722,  22654, -15137, -26722    ; w31 w29 w27 w25

;-----------------------------------------------------------------------------
; Tables for xmm processors
;-----------------------------------------------------------------------------

; %3 for rows 0,4 - constants are multiplied by cos_4_16
tab_i_04_xmm:
  dw  16384,  21407,  16384,   8867 ; movq-> w05 w04 w01 w00
  dw  16384,   8867, -16384, -21407 ; w07 w06 w03 w02
  dw  16384,  -8867,  16384, -21407 ; w13 w12 w09 w08
  dw -16384,  21407,  16384,  -8867 ; w15 w14 w11 w10
  dw  22725,  19266,  19266,  -4520 ; w21 w20 w17 w16
  dw  12873,   4520, -22725, -12873 ; w23 w22 w19 w18
  dw  12873, -22725,   4520, -12873 ; w29 w28 w25 w24
  dw   4520,  19266,  19266, -22725 ; w31 w30 w27 w26

; %3 for rows 1,7 - constants are multiplied by cos_1_16
tab_i_17_xmm:
  dw  22725,  29692,  22725,  12299 ; movq-> w05 w04 w01 w00
  dw  22725,  12299, -22725, -29692 ; w07 w06 w03 w02
  dw  22725, -12299,  22725, -29692 ; w13 w12 w09 w08
  dw -22725,  29692,  22725, -12299 ; w15 w14 w11 w10
  dw  31521,  26722,  26722,  -6270 ; w21 w20 w17 w16
  dw  17855,   6270, -31521, -17855 ; w23 w22 w19 w18
  dw  17855, -31521,   6270, -17855 ; w29 w28 w25 w24
  dw   6270,  26722,  26722, -31521 ; w31 w30 w27 w26

; %3 for rows 2,6 - constants are multiplied by cos_2_16
tab_i_26_xmm:
  dw  21407,  27969,  21407,  11585 ; movq-> w05 w04 w01 w00
  dw  21407,  11585, -21407, -27969 ; w07 w06 w03 w02
  dw  21407, -11585,  21407, -27969 ; w13 w12 w09 w08
  dw -21407,  27969,  21407, -11585 ; w15 w14 w11 w10
  dw  29692,  25172,  25172,  -5906 ; w21 w20 w17 w16
  dw  16819,   5906, -29692, -16819 ; w23 w22 w19 w18
  dw  16819, -29692,   5906, -16819 ; w29 w28 w25 w24
  dw   5906,  25172,  25172, -29692 ; w31 w30 w27 w26

; %3 for rows 3,5 - constants are multiplied by cos_3_16
tab_i_35_xmm:
  dw  19266,  25172,  19266,  10426 ; movq-> w05 w04 w01 w00
  dw  19266,  10426, -19266, -25172 ; w07 w06 w03 w02
  dw  19266, -10426,  19266, -25172 ; w13 w12 w09 w08
  dw -19266,  25172,  19266, -10426 ; w15 w14 w11 w10
  dw  26722,  22654,  22654,  -5315 ; w21 w20 w17 w16
  dw  15137,   5315, -26722, -15137 ; w23 w22 w19 w18
  dw  15137, -26722,   5315, -15137 ; w29 w28 w25 w24
  dw   5315,  22654,  22654, -26722 ; w31 w30 w27 w26

;=============================================================================
; Helper macros for the code
;=============================================================================

;-----------------------------------------------------------------------------
; DCT_8_INV_ROW_MMX  INP, OUT, TABLE, ROUNDER
;-----------------------------------------------------------------------------

%macro DCT_8_INV_ROW_MMX 4
  movq mm0, [%1]            ; 0 ; x3 x2 x1 x0
  movq mm1, [%1+8]          ; 1 ; x7 x6 x5 x4
  movq mm2, mm0             ; 2  ; x3 x2 x1 x0
  movq mm3, [%3]            ; 3 ; w06 w04 w02 w00
  punpcklwd mm0, mm1        ; x5 x1 x4 x0
  movq mm5, mm0             ; 5 ; x5 x1 x4 x0
  punpckldq mm0, mm0        ; x4 x0 x4 x0
  movq mm4, [%3+8]          ; 4 ; w07 w05 w03 w01
  punpckhwd mm2, mm1        ; 1 ; x7 x3 x6 x2
  pmaddwd mm3, mm0          ; x4*w06+x0*w04 x4*w02+x0*w00
  movq mm6, mm2             ; 6 ; x7 x3 x6 x2
  movq mm1, [%3+32]         ; 1 ; w22 w20 w18 w16
  punpckldq mm2, mm2        ; x6 x2 x6 x2
  pmaddwd mm4, mm2          ; x6*w07+x2*w05 x6*w03+x2*w01
  punpckhdq mm5, mm5        ; x5 x1 x5 x1
  pmaddwd mm0, [%3+16]      ; x4*w14+x0*w12 x4*w10+x0*w08
  punpckhdq mm6, mm6        ; x7 x3 x7 x3
  movq mm7, [%3+40]         ; 7 ; w23 w21 w19 w17
  pmaddwd mm1, mm5          ; x5*w22+x1*w20 x5*w18+x1*w16
  paddd mm3, [%4]           ; +%4
  pmaddwd mm7, mm6          ; x7*w23+x3*w21 x7*w19+x3*w17
  pmaddwd mm2, [%3+24]      ; x6*w15+x2*w13 x6*w11+x2*w09
  paddd mm3, mm4            ; 4 ; a1=sum(even1) a0=sum(even0)
  pmaddwd mm5, [%3+48]      ; x5*w30+x1*w28 x5*w26+x1*w24
  movq mm4, mm3             ; 4 ; a1 a0
  pmaddwd mm6, [%3+56]      ; x7*w31+x3*w29 x7*w27+x3*w25
  paddd mm1, mm7            ; 7 ; b1=sum(odd1) b0=sum(odd0)
  paddd mm0, [%4]           ; +%4
  psubd mm3, mm1            ; a1-b1 a0-b0
  psrad mm3, SHIFT_INV_ROW  ; y6=a1-b1 y7=a0-b0
  paddd mm1, mm4            ; 4 ; a1+b1 a0+b0
  paddd mm0, mm2            ; 2 ; a3=sum(even3) a2=sum(even2)
  psrad mm1, SHIFT_INV_ROW  ; y1=a1+b1 y0=a0+b0
  paddd mm5, mm6            ; 6 ; b3=sum(odd3) b2=sum(odd2)
  movq mm4, mm0             ; 4 ; a3 a2
  paddd mm0, mm5            ; a3+b3 a2+b2
  psubd mm4, mm5            ; 5 ; a3-b3 a2-b2
  psrad mm0, SHIFT_INV_ROW  ; y3=a3+b3 y2=a2+b2
  psrad mm4, SHIFT_INV_ROW  ; y4=a3-b3 y5=a2-b2
  packssdw mm1, mm0         ; 0 ; y3 y2 y1 y0
  packssdw mm4, mm3         ; 3 ; y6 y7 y4 y5
  movq mm7, mm4             ; 7 ; y6 y7 y4 y5
  psrld mm4, 16             ; 0 y6 0 y4
  pslld mm7, 16             ; y7 0 y5 0
  movq [%2], mm1            ; 1 ; save y3 y2 y1 y0
  por mm7, mm4              ; 4 ; y7 y6 y5 y4
  movq [%2+8], mm7          ; 7 ; save y7 y6 y5 y4
%endmacro

;-----------------------------------------------------------------------------
; DCT_8_INV_ROW_XMM  INP, OUT, TABLE, ROUNDER
;-----------------------------------------------------------------------------

%macro DCT_8_INV_ROW_XMM 4
  movq mm0, [%1]                ; 0     ; x3 x2 x1 x0
  movq mm1, [%1+8]              ; 1     ; x7 x6 x5 x4
  movq mm2, mm0                 ; 2     ; x3 x2 x1 x0
  movq mm3, [%3]                ; 3     ; w05 w04 w01 w00
  pshufw mm0, mm0, 10001000b    ; x2 x0 x2 x0
  movq mm4, [%3+8]              ; 4     ; w07 w06 w03 w02
  movq mm5, mm1                 ; 5     ; x7 x6 x5 x4
  pmaddwd mm3, mm0              ; x2*w05+x0*w04 x2*w01+x0*w00
  movq mm6, [%3+32]             ; 6     ; w21 w20 w17 w16
  pshufw mm1, mm1, 10001000b    ; x6 x4 x6 x4
  pmaddwd mm4, mm1              ; x6*w07+x4*w06 x6*w03+x4*w02
  movq mm7, [%3+40]             ; 7    ; w23 w22 w19 w18
  pshufw mm2, mm2, 11011101b    ; x3 x1 x3 x1
  pmaddwd mm6, mm2              ; x3*w21+x1*w20 x3*w17+x1*w16
  pshufw mm5, mm5, 11011101b    ; x7 x5 x7 x5
  pmaddwd mm7, mm5              ; x7*w23+x5*w22 x7*w19+x5*w18
  paddd mm3, [%4]               ; +%4
  pmaddwd mm0, [%3+16]          ; x2*w13+x0*w12 x2*w09+x0*w08
  paddd mm3, mm4                ; 4     ; a1=sum(even1) a0=sum(even0)
  pmaddwd mm1, [%3+24]          ; x6*w15+x4*w14 x6*w11+x4*w10
  movq mm4, mm3                 ; 4     ; a1 a0
  pmaddwd mm2, [%3+48]          ; x3*w29+x1*w28 x3*w25+x1*w24
  paddd mm6, mm7                ; 7     ; b1=sum(odd1) b0=sum(odd0)
  pmaddwd mm5, [%3+56]          ; x7*w31+x5*w30 x7*w27+x5*w26
  paddd mm3, mm6                ; a1+b1 a0+b0
  paddd mm0, [%4]               ; +%4
  psrad mm3, SHIFT_INV_ROW      ; y1=a1+b1 y0=a0+b0
  paddd mm0, mm1                ; 1     ; a3=sum(even3) a2=sum(even2)
  psubd mm4, mm6                ; 6     ; a1-b1 a0-b0
  movq mm7, mm0                 ; 7     ; a3 a2
  paddd mm2, mm5                ; 5     ; b3=sum(odd3) b2=sum(odd2)
  paddd mm0, mm2                ; a3+b3 a2+b2
  psrad mm4, SHIFT_INV_ROW      ; y6=a1-b1 y7=a0-b0
  psubd mm7, mm2                ; 2     ; a3-b3 a2-b2
  psrad mm0, SHIFT_INV_ROW      ; y3=a3+b3 y2=a2+b2
  psrad mm7, SHIFT_INV_ROW      ; y4=a3-b3 y5=a2-b2
  packssdw mm3, mm0             ; 0     ; y3 y2 y1 y0
  packssdw mm7, mm4             ; 4     ; y6 y7 y4 y5
  movq [%2], mm3                ; 3     ; save y3 y2 y1 y0
  pshufw mm7, mm7, 10110001b    ; y7 y6 y5 y4
  movq [%2+8], mm7              ; 7     ; save y7 y6 y5 y4
%endmacro

;-----------------------------------------------------------------------------
;
; The first stage DCT 8x8 - forward DCTs of columns
;
; The %2puts are multiplied
; for rows 0,4 - on cos_4_16,
; for rows 1,7 - on cos_1_16,
; for rows 2,6 - on cos_2_16,
; for rows 3,5 - on cos_3_16
; and are shifted to the left for rise of accuracy
;
;-----------------------------------------------------------------------------
;
; The 8-point scaled forward DCT algorithm (26a8m)
;
;-----------------------------------------------------------------------------
;
; #define DCT_8_FRW_COL(x, y)
;{
; short t0, t1, t2, t3, t4, t5, t6, t7;
; short tp03, tm03, tp12, tm12, tp65, tm65;
; short tp465, tm465, tp765, tm765;
;
; t0 = LEFT_SHIFT ( x[0] + x[7] );
; t1 = LEFT_SHIFT ( x[1] + x[6] );
; t2 = LEFT_SHIFT ( x[2] + x[5] );
; t3 = LEFT_SHIFT ( x[3] + x[4] );
; t4 = LEFT_SHIFT ( x[3] - x[4] );
; t5 = LEFT_SHIFT ( x[2] - x[5] );
; t6 = LEFT_SHIFT ( x[1] - x[6] );
; t7 = LEFT_SHIFT ( x[0] - x[7] );
;
; tp03 = t0 + t3;
; tm03 = t0 - t3;
; tp12 = t1 + t2;
; tm12 = t1 - t2;
;
; y[0] = tp03 + tp12;
; y[4] = tp03 - tp12;
;
; y[2] = tm03 + tm12 * tg_2_16;
; y[6] = tm03 * tg_2_16 - tm12;
;
; tp65 =(t6 +t5 )*cos_4_16;
; tm65 =(t6 -t5 )*cos_4_16;
;
; tp765 = t7 + tp65;
; tm765 = t7 - tp65;
; tp465 = t4 + tm65;
; tm465 = t4 - tm65;
;
; y[1] = tp765 + tp465 * tg_1_16;
; y[7] = tp765 * tg_1_16 - tp465;
; y[5] = tm765 * tg_3_16 + tm465;
; y[3] = tm765 - tm465 * tg_3_16;
;}
;
;-----------------------------------------------------------------------------

;-----------------------------------------------------------------------------
; DCT_8_INV_COL_4  INP,OUT
;-----------------------------------------------------------------------------

%macro DCT_8_INV_COL 2
  movq mm0, [tg_3_16]
  movq mm3, [%1+16*3]
  movq mm1, mm0             ; tg_3_16
  movq mm5, [%1+16*5]
  pmulhw mm0, mm3           ; x3*(tg_3_16-1)
  movq mm4, [tg_1_16]
  pmulhw mm1, mm5           ; x5*(tg_3_16-1)
  movq mm7, [%1+16*7]
  movq mm2, mm4             ; tg_1_16
  movq mm6, [%1+16*1]
  pmulhw mm4, mm7           ; x7*tg_1_16
  paddsw mm0, mm3           ; x3*tg_3_16
  pmulhw mm2, mm6           ; x1*tg_1_16
  paddsw mm1, mm3           ; x3+x5*(tg_3_16-1)
  psubsw mm0, mm5           ; x3*tg_3_16-x5 = tm35
  movq mm3, [ocos_4_16]
  paddsw mm1, mm5           ; x3+x5*tg_3_16 = tp35
  paddsw mm4, mm6           ; x1+tg_1_16*x7 = tp17
  psubsw mm2, mm7           ; x1*tg_1_16-x7 = tm17
  movq mm5, mm4             ; tp17
  movq mm6, mm2             ; tm17
  paddsw mm5, mm1           ; tp17+tp35 = b0
  psubsw mm6, mm0           ; tm17-tm35 = b3
  psubsw mm4, mm1           ; tp17-tp35 = t1
  paddsw mm2, mm0           ; tm17+tm35 = t2
  movq mm7, [tg_2_16]
  movq mm1, mm4             ; t1
;  movq [SCRATCH+0], mm5    ; save b0
  movq [%2+3*16], mm5       ; save b0
  paddsw mm1, mm2           ; t1+t2
;  movq [SCRATCH+8], mm6    ; save b3
  movq [%2+5*16], mm6       ; save b3
  psubsw mm4, mm2           ; t1-t2
  movq mm5, [%1+2*16]
  movq mm0, mm7             ; tg_2_16
  movq mm6, [%1+6*16]
  pmulhw mm0, mm5           ; x2*tg_2_16
  pmulhw mm7, mm6           ; x6*tg_2_16
; slot
  pmulhw mm1, mm3           ; ocos_4_16*(t1+t2) = b1/2
; slot
  movq mm2, [%1+0*16]
  pmulhw mm4, mm3           ; ocos_4_16*(t1-t2) = b2/2
  psubsw mm0, mm6           ; t2*tg_2_16-x6 = tm26
  movq mm3, mm2             ; x0
  movq mm6, [%1+4*16]
  paddsw mm7, mm5           ; x2+x6*tg_2_16 = tp26
  paddsw mm2, mm6           ; x0+x4 = tp04
  psubsw mm3, mm6           ; x0-x4 = tm04
  movq mm5, mm2             ; tp04
  movq mm6, mm3             ; tm04
  psubsw mm2, mm7           ; tp04-tp26 = a3
  paddsw mm3, mm0           ; tm04+tm26 = a1
  paddsw mm1, mm1           ; b1
  paddsw mm4, mm4           ; b2
  paddsw mm5, mm7           ; tp04+tp26 = a0
  psubsw mm6, mm0           ; tm04-tm26 = a2
  movq mm7, mm3             ; a1
  movq mm0, mm6             ; a2
  paddsw mm3, mm1           ; a1+b1
  paddsw mm6, mm4           ; a2+b2
  psraw mm3, SHIFT_INV_COL  ; dst1
  psubsw mm7, mm1           ; a1-b1
  psraw mm6, SHIFT_INV_COL  ; dst2
  psubsw mm0, mm4           ; a2-b2
;  movq mm1, [SCRATCH+0]    ; load b0
  movq mm1, [%2+3*16]       ; load b0
  psraw mm7, SHIFT_INV_COL  ; dst6
  movq mm4, mm5             ; a0
  psraw mm0, SHIFT_INV_COL  ; dst5
  movq [%2+1*16], mm3
  paddsw mm5, mm1           ; a0+b0
  movq [%2+2*16], mm6
  psubsw mm4, mm1           ; a0-b0
;  movq mm3, [SCRATCH+8]    ; load b3
  movq mm3, [%2+5*16]       ; load b3
  psraw mm5, SHIFT_INV_COL  ; dst0
  movq mm6, mm2             ; a3
  psraw mm4, SHIFT_INV_COL  ; dst7
  movq [%2+5*16], mm0
  paddsw mm2, mm3           ; a3+b3
  movq [%2+6*16], mm7
  psubsw mm6, mm3           ; a3-b3
  movq [%2+0*16], mm5
  psraw mm2, SHIFT_INV_COL  ; dst3
  movq [%2+7*16], mm4
  psraw mm6, SHIFT_INV_COL  ; dst4
  movq [%2+3*16], mm2
  movq [%2+4*16], mm6
%endmacro

;=============================================================================
; Code
;=============================================================================

TEXT

cglobal idct_mmx
cglobal idct_xmm

;-----------------------------------------------------------------------------
; void idct_mmx(uint16_t block[64]);
;-----------------------------------------------------------------------------

ALIGN SECTION_ALIGN
idct_mmx:
    mov TMP0, prm1

	;; Process each row
    DCT_8_INV_ROW_MMX TMP0+0*16, TMP0+0*16, tab_i_04_mmx, rounder_0
    DCT_8_INV_ROW_MMX TMP0+1*16, TMP0+1*16, tab_i_17_mmx, rounder_1
    DCT_8_INV_ROW_MMX TMP0+2*16, TMP0+2*16, tab_i_26_mmx, rounder_2
    DCT_8_INV_ROW_MMX TMP0+3*16, TMP0+3*16, tab_i_35_mmx, rounder_3
    DCT_8_INV_ROW_MMX TMP0+4*16, TMP0+4*16, tab_i_04_mmx, rounder_4
    DCT_8_INV_ROW_MMX TMP0+5*16, TMP0+5*16, tab_i_35_mmx, rounder_5
    DCT_8_INV_ROW_MMX TMP0+6*16, TMP0+6*16, tab_i_26_mmx, rounder_6
    DCT_8_INV_ROW_MMX TMP0+7*16, TMP0+7*16, tab_i_17_mmx, rounder_7

	;; Process the columns (4 at a time)
    DCT_8_INV_COL TMP0+0, TMP0+0
    DCT_8_INV_COL TMP0+8, TMP0+8

    ret
ENDFUNC

;-----------------------------------------------------------------------------
; void idct_xmm(uint16_t block[64]);
;-----------------------------------------------------------------------------

ALIGN SECTION_ALIGN
idct_xmm:
    mov TMP0, prm1

	;; Process each row
    DCT_8_INV_ROW_XMM TMP0+0*16, TMP0+0*16, tab_i_04_xmm, rounder_0
    DCT_8_INV_ROW_XMM TMP0+1*16, TMP0+1*16, tab_i_17_xmm, rounder_1
    DCT_8_INV_ROW_XMM TMP0+2*16, TMP0+2*16, tab_i_26_xmm, rounder_2
    DCT_8_INV_ROW_XMM TMP0+3*16, TMP0+3*16, tab_i_35_xmm, rounder_3
    DCT_8_INV_ROW_XMM TMP0+4*16, TMP0+4*16, tab_i_04_xmm, rounder_4
    DCT_8_INV_ROW_XMM TMP0+5*16, TMP0+5*16, tab_i_35_xmm, rounder_5
    DCT_8_INV_ROW_XMM TMP0+6*16, TMP0+6*16, tab_i_26_xmm, rounder_6
    DCT_8_INV_ROW_XMM TMP0+7*16, TMP0+7*16, tab_i_17_xmm, rounder_7

	;; Process the columns (4 at a time)
    DCT_8_INV_COL TMP0+0, TMP0+0
    DCT_8_INV_COL TMP0+8, TMP0+8

    ret
ENDFUNC

NON_EXEC_STACK
