;/****************************************************************************
; *
; *  XVID MPEG-4 VIDEO CODEC
; *  - MMX and XMM forward discrete cosine transform -
; *
; *  Copyright(C) 2001 Peter Ross <pross@xvid.org>
; *               2002 Jaan Kalda
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
; * $Id: idct_3dne.asm,v 1.9.2.2 2009/09/16 17:11:39 Isibaar Exp $
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

; ***************************************************************************/
; this 3dne function is compatible with iSSE, but is optimized specifically for
; K7 pipelines (ca 5% gain), for implementation details see the idct_mmx.asm
; file
;
; ----------------------------------------------------------------------------
; Athlon optimizations contributed by Jaan Kalda
;-----------------------------------------------------------------------------

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
  dw 13036,  13036,  13036,  13036     ; tg * (2<<16) + 0.5
tg_2_16:
  dw 27146,  27146,  27146,  27146     ; tg * (2<<16) + 0.5
tg_3_16:
  dw -21746, -21746, -21746, -21746    ; tg * (2<<16) + 0.5
cos_4_16:
  dw -19195, -19195, -19195, -19195    ; cos * (2<<16) + 0.5
ocos_4_16:
  dw 23170,  23170,  23170,  23170     ; cos * (2<<15) + 0.5
otg_3_16:
  dw 21895, 21895, 21895, 21895        ; tg * (2<<16) + 0.5

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
; Code
;=============================================================================

TEXT

cglobal idct_3dne

;-----------------------------------------------------------------------------
; void idct_3dne(uint16_t block[64]);
;-----------------------------------------------------------------------------

ALIGN SECTION_ALIGN
idct_3dne:
  mov _ECX, prm1

;   DCT_8_INV_ROW_1_s [_ECX+64], [_ECX+64], tab_i_04_sse, rounder_4 ;rounder_4=0
  pshufw mm0, [_ECX+64],10001000b        ; x2 x0 x2 x0
  movq mm3, [tab_i_04_xmm]          ; 3     ; w05 w04 w01 w00
  pshufw mm1, [_ECX+64+8],10001000b  ; x6 x4 x6 x4
  movq mm4, [tab_i_04_xmm+8]        ; 4     ; w07 w06 w03 w02
  pshufw mm2, [_ECX+64],11011101b        ; x3 x1 x3 x1
  pshufw mm5, [_ECX+64+8],11011101b  ; x7 x5 x7 x5
  movq mm6, [tab_i_04_xmm+32]   ; 6     ; w21 w20 w17 w16
  pmaddwd mm3, mm0              ; x2*w05+x0*w04 x2*w01+x0*w00
  movq mm7, [tab_i_04_xmm+40]   ; 7     ; w23 w22 w19 w18 ;
  pmaddwd mm0, [tab_i_04_xmm+16]; x2*w13+x0*w12 x2*w09+x0*w08
  pmaddwd mm4, mm1              ; x6*w07+x4*w06 x6*w03+x4*w02
  pmaddwd mm1, [tab_i_04_xmm+24]; x6*w15+x4*w14 x6*w11+x4*w10
  pmaddwd mm6, mm2              ; x3*w21+x1*w20 x3*w17+x1*w16
  pmaddwd mm2, [tab_i_04_xmm+48]; x3*w29+x1*w28 x3*w25+x1*w24
  pmaddwd mm7, mm5              ; 7     ; x7*w23+x5*w22 x7*w19+x5*w18 ; w23 w22 w19 w18
  pmaddwd mm5, [tab_i_04_xmm+56]; x7*w31+x5*w30 x7*w27+x5*w26
  paddd mm3, mm4                ; 4 free    ; a1=sum(even1) a0=sum(even0)
  paddd mm0, mm1                ; 1 free    ; a3=sum(even3) a2=sum(even2)
  pshufw mm1, [_ECX+80+8],10001000b  ; x6 x4 x6 x4
  movq mm4, mm3                 ; 4     ; a1 a0
  paddd mm6, mm7                ; 7 free    ; b1=sum(odd1) b0=sum(odd0)
  paddd mm2, mm5                ; 5 free    ; b3=sum(odd3) b2=sum(odd2)
  pshufw mm5, [_ECX+80],10001000b; x2 x0 x2 x0   mm5 & mm0 exchanged for next cycle
  movq mm7, mm0                 ; 7     ; a3 a2
  psubd mm4, mm6                ; 6 free    ; a1-b1 a0-b0
  paddd mm6, mm3                ; mm6 = mm3+mm6+mm5+mm4; a1+b1 a0+b0
  movq mm3, [tab_i_35_xmm]      ; 3     ; w05 w04 w01 w00
  psubd mm7, mm2                ; ; a3-b3 a2-b2
  paddd mm0, mm2                ; 0 free a3+b3 a2+b2
  pshufw mm2, [_ECX+80],11011101b; x3 x1 x3 x1
  pmaddwd mm3, mm5              ; x2*w05+x0*w04 x2*w01+x0*w00
  pmaddwd mm5, [tab_i_35_xmm+16]; x2*w13+x0*w12 x2*w09+x0*w08
  psrad mm4, SHIFT_INV_ROW      ; y6=a1-b1 y7=a0-b0
  psrad mm7, SHIFT_INV_ROW      ; y4=a3-b3 y5=a2-b2
  psrad mm6, SHIFT_INV_ROW      ; y1=a1+b1 y0=a0+b0
  psrad mm0, SHIFT_INV_ROW      ; y3=a3+b3 y2=a2+b2
  packssdw mm7, mm4             ; 4     ; y6 y7 y4 y5
  packssdw mm6, mm0             ; 0 free    ; y3 y2 y1 y0
  pshufw mm0, [_ECX+80+8],11011101b  ; x7 x5 x7 x5
  movq [_ECX+64], mm6            ; 3     ; save y3 y2 y1 y0 stall2

;   DCT_8_INV_ROW_1_s [_ECX+80], [_ECX+80], tab_i_35_xmm, rounder_5
  movq mm4, [tab_i_35_xmm+8]    ; 4     ; w07 w06 w03 w02
  movq mm6, [tab_i_35_xmm+32]   ; 6     ; w21 w20 w17 w16
  pshufw mm7, mm7, 10110001b    ; y7 y6 y5 y4
  paddd mm3, [rounder_5]        ; +rounder stall 6
  paddd mm5, [rounder_5]        ; +rounder
  movq [_ECX+64+8], mm7          ; 7     ; save y7 y6 y5 y4
  movq mm7, [tab_i_35_xmm+40]   ; 7     ; w23 w22 w19 w18
  pmaddwd mm4, mm1              ; x6*w07+x4*w06 x6*w03+x4*w02
  pmaddwd mm1, [tab_i_35_xmm+24]; x6*w15+x4*w14 x6*w11+x4*w10
  pmaddwd mm6, mm2              ; x3*w21+x1*w20 x3*w17+x1*w16
  pmaddwd mm2, [tab_i_35_xmm+48]; x3*w29+x1*w28 x3*w25+x1*w24
  pmaddwd mm7, mm0              ; 7     ; x7*w23+x5*w22 x7*w19+x5*w18 ; w23 w22 w19 w18
  pmaddwd mm0, [tab_i_35_xmm+56]; x7*w31+x5*w30 x7*w27+x5*w26
  paddd mm3, mm4                ; 4 free    ; a1=sum(even1) a0=sum(even0)
  paddd mm5, mm1                ; 1 free    ; a3=sum(even3) a2=sum(even2)
  pshufw mm1, [_ECX+96+8],10001000b  ; x6 x4 x6 x4
  movq mm4, mm3                 ; 4     ; a1 a0
  paddd mm6, mm7                ; 7 free    ; b1=sum(odd1) b0=sum(odd0)
  paddd mm2, mm0                ; 5 free    ; b3=sum(odd3) b2=sum(odd2)
  pshufw mm0, [_ECX+96],10001000b    ; x2 x0 x2 x0
  movq mm7, mm5                 ; 7     ; a3 a2
  psubd mm4, mm6                ; 6 free    ; a1-b1 a0-b0 stall 5
  paddd mm6, mm3                ; mm3 = mm3+mm6+mm5+mm4; a1+b1 a0+b0
  movq mm3, [tab_i_26_xmm]      ; 3     ; w05 w04 w01 w00
  psubd mm7, mm2                ; ; a3-b3 a2-b2
  paddd mm5, mm2                ; 0 free a3+b3 a2+b2
  pshufw mm2, [_ECX+96],11011101b; x3 x1 x3 x1
  pmaddwd mm3, mm0              ; x2*w05+x0*w04 x2*w01+x0*w00
  pmaddwd mm0, [tab_i_26_xmm+16]; x2*w13+x0*w12 x2*w09+x0*w08
  psrad mm4, SHIFT_INV_ROW      ; y6=a1-b1 y7=a0-b0
  psrad mm7, SHIFT_INV_ROW      ; y4=a3-b3 y5=a2-b2
  psrad mm6, SHIFT_INV_ROW      ; y1=a1+b1 y0=a0+b0
  psrad mm5, SHIFT_INV_ROW      ; y3=a3+b3 y2=a2+b2
  packssdw mm7, mm4             ; 4     ; y6 y7 y4 y5
  packssdw mm6, mm5             ; 0 free    ; y3 y2 y1 y0
  pshufw mm5, [_ECX+96+8],11011101b  ; x7 x5 x7 x5
  movq [_ECX+80], mm6            ; 3     ; save y3 y2 y1 y0

;   DCT_8_INV_ROW_1_s [_ECX+96], [_ECX+96], tab_i_26_xmm, rounder_6
  movq mm4, [tab_i_26_xmm+8]    ; 4     ; w07 w06 w03 w02
  movq mm6, [tab_i_26_xmm+32]   ; 6     ; w21 w20 w17 w16
  pshufw mm7, mm7, 10110001b    ; y7 y6 y5 y4 STALL 6
  paddd mm3, [rounder_6]        ; +rounder
  paddd mm0, [rounder_6]        ; +rounder
  movq [_ECX+80+8], mm7          ; 7     ; save y7 y6
  movq mm7, [tab_i_26_xmm+40]   ; 7     ; w23 w22 w19 w18
  pmaddwd mm4, mm1              ; x6*w07+x4*w06 x6*w03+x4*w02
  pmaddwd mm1, [tab_i_26_xmm+24]; x6*w15+x4*w14 x6*w11+x4*w10
  pmaddwd mm6, mm2              ; x3*w21+x1*w20 x3*w17+x1*w16
  pmaddwd mm2, [tab_i_26_xmm+48]; x3*w29+x1*w28 x3*w25+x1*w24
  pmaddwd mm7, mm5              ; 7     ; x7*w23+x5*w22 x7*w19+x5*w18 ; w23 w22 w19 w18
  pmaddwd mm5, [tab_i_26_xmm+56]; x7*w31+x5*w30 x7*w27+x5*w26
  paddd mm3, mm4                ; 4 free    ; a1=sum(even1) a0=sum(even0)
  paddd mm0, mm1                ; 1 free    ; a3=sum(even3) a2=sum(even2)
  pshufw mm1, [_ECX+112+8],10001000b ; x6 x4 x6 x4
  movq mm4, mm3                 ; 4     ; a1 a0
  paddd mm6, mm7                ; 7 free    ; b1=sum(odd1) b0=sum(odd0)
  paddd mm2, mm5                ; 5 free    ; b3=sum(odd3) b2=sum(odd2)
  pshufw mm5, [_ECX+112],10001000b; x2 x0 x2 x0  mm5 & mm0 exchanged for next cycle
  movq mm7, mm0                 ; 7     ; a3 a2
  psubd mm4, mm6                ; 6 free    ; a1-b1 a0-b0
  paddd mm6, mm3                ; mm6 = mm3+mm6+mm5+mm4; a1+b1 a0+b0
  movq mm3, [tab_i_17_xmm]      ; 3     ; w05 w04 w01 w00
  psubd mm7, mm2                ; ; a3-b3 a2-b2
  paddd mm0, mm2                ; 0 free a3+b3 a2+b2
  pshufw mm2, [_ECX+112],11011101b; x3 x1 x3 x1
  pmaddwd mm3, mm5              ; x2*w05+x0*w04 x2*w01+x0*w00
  pmaddwd mm5, [tab_i_17_xmm+16]; x2*w13+x0*w12 x2*w09+x0*w08
  psrad mm4, SHIFT_INV_ROW      ; y6=a1-b1 y7=a0-b0
  psrad mm7, SHIFT_INV_ROW      ; y4=a3-b3 y5=a2-b2
  psrad mm6, SHIFT_INV_ROW      ; y1=a1+b1 y0=a0+b0
  psrad mm0, SHIFT_INV_ROW      ; y3=a3+b3 y2=a2+b2
  packssdw mm7, mm4             ; 4     ; y6 y7 y4 y5
  packssdw mm6, mm0             ; 0 free    ; y3 y2 y1 y0
  pshufw mm0, [_ECX+112+8],11011101b ; x7 x5 x7 x5
  movq [_ECX+96], mm6            ; 3     ; save y3 y2 y1 y0 stall2

;   DCT_8_INV_ROW_1_s [_ECX+112], [_ECX+112], tab_i_17_xmm, rounder_7
  movq mm4, [tab_i_17_xmm+8]    ; 4     ; w07 w06 w03 w02
  movq mm6, [tab_i_17_xmm+32]   ; 6     ; w21 w20 w17 w16
  pshufw mm7, mm7, 10110001b    ; y7 y6 y5 y4
  paddd mm3, [rounder_7]        ; +rounder stall 6
  paddd mm5, [rounder_7]        ; +rounder
  movq [_ECX+96+8], mm7          ; 7     ; save y7 y6 y5 y4
  movq mm7, [tab_i_17_xmm+40]   ; 7     ; w23 w22 w19 w18
  pmaddwd mm4, mm1              ; x6*w07+x4*w06 x6*w03+x4*w02
  pmaddwd mm1, [tab_i_17_xmm+24]; x6*w15+x4*w14 x6*w11+x4*w10
  pmaddwd mm6, mm2              ; x3*w21+x1*w20 x3*w17+x1*w16
  pmaddwd mm2, [tab_i_17_xmm+48]; x3*w29+x1*w28 x3*w25+x1*w24
  pmaddwd mm7, mm0              ; 7     ; x7*w23+x5*w22 x7*w19+x5*w18 ; w23 w22 w19 w18
  pmaddwd mm0, [tab_i_17_xmm+56]; x7*w31+x5*w30 x7*w27+x5*w26
  paddd mm3, mm4                ; 4 free    ; a1=sum(even1) a0=sum(even0)
  paddd mm5, mm1                ; 1 free    ; a3=sum(even3) a2=sum(even2)
  pshufw mm1, [_ECX+0+8],10001000b; x6 x4 x6 x4
  movq mm4, mm3                 ; 4     ; a1 a0
  paddd mm6, mm7                ; 7 free    ; b1=sum(odd1) b0=sum(odd0)
  paddd mm2, mm0                ; 5 free    ; b3=sum(odd3) b2=sum(odd2)
  pshufw mm0, [_ECX+0],10001000b ; x2 x0 x2 x0
  movq mm7, mm5                 ; 7     ; a3 a2
  psubd mm4, mm6                ; 6 free    ; a1-b1 a0-b0 stall 5
  paddd mm6, mm3                ; mm3 = mm3+mm6+mm5+mm4; a1+b1 a0+b0
  movq mm3, [tab_i_04_xmm]      ; 3     ; w05 w04 w01 w00
  psubd mm7, mm2                ; ; a3-b3 a2-b2
  paddd mm5, mm2                ; 0 free a3+b3 a2+b2
  pshufw mm2, [_ECX+0],11011101b ; x3 x1 x3 x1
  pmaddwd mm3, mm0              ; x2*w05+x0*w04 x2*w01+x0*w00
  pmaddwd mm0, [tab_i_04_xmm+16]; x2*w13+x0*w12 x2*w09+x0*w08
  psrad mm4, SHIFT_INV_ROW      ; y6=a1-b1 y7=a0-b0
  psrad mm7, SHIFT_INV_ROW      ; y4=a3-b3 y5=a2-b2
  psrad mm6, SHIFT_INV_ROW      ; y1=a1+b1 y0=a0+b0
  psrad mm5, SHIFT_INV_ROW      ; y3=a3+b3 y2=a2+b2
  packssdw mm7, mm4             ; 4     ; y6 y7 y4 y5
  packssdw mm6, mm5             ; 0 free    ; y3 y2 y1 y0
  pshufw mm5, [_ECX+0+8],11011101b; x7 x5 x7 x5
  movq [_ECX+112], mm6           ; 3     ; save y3 y2 y1 y0

;   DCT_8_INV_ROW_1_s [_ECX+0],  0, tab_i_04_xmm, rounder_0
  movq mm4, [tab_i_04_xmm+8]    ; 4     ; w07 w06 w03 w02
  movq mm6, [tab_i_04_xmm+32]   ; 6     ; w21 w20 w17 w16
  pshufw mm7, mm7, 10110001b    ; y7 y6 y5 y4 STALL 6
  paddd mm3, [rounder_0]        ; +rounder
  paddd mm0, [rounder_0]        ; +rounder
  movq [_ECX+112+8], mm7         ; 7     ; save y7 y6
  movq mm7, [tab_i_04_xmm+40]   ; 7     ; w23 w22 w19 w18
  pmaddwd mm4, mm1              ; x6*w07+x4*w06 x6*w03+x4*w02
  pmaddwd mm1, [tab_i_04_xmm+24]; x6*w15+x4*w14 x6*w11+x4*w10
  pmaddwd mm6, mm2              ; x3*w21+x1*w20 x3*w17+x1*w16
  pmaddwd mm2, [tab_i_04_xmm+48]; x3*w29+x1*w28 x3*w25+x1*w24
  pmaddwd mm7, mm5              ; 7     ; x7*w23+x5*w22 x7*w19+x5*w18 ; w23 w22 w19 w18
  pmaddwd mm5, [tab_i_04_xmm+56]; x7*w31+x5*w30 x7*w27+x5*w26
  paddd mm3, mm4                ; 4 free    ; a1=sum(even1) a0=sum(even0)
  paddd mm0, mm1                ; 1
  pshufw mm1, [_ECX+16+8],10001000b  ; x6 x4 x6 x4
  movq mm4, mm3                 ; 4     ; a1 a0
  paddd mm6, mm7                ; 7 free    ; b1=sum(odd1) b0=sum(odd0)
  paddd mm2, mm5                ; 5 free    ; b3=sum(odd3) b2=sum(odd2)
  pshufw mm5, [_ECX+16],10001000b; x2 x0 x2 x0   mm5 & mm0 exchanged for next cycle
  movq mm7, mm0                 ; 7     ; a3 a2
  psubd mm4, mm6                ; 6 free    ; a1-b1 a0-b0
  paddd mm6, mm3                ; mm6 = mm3+mm6+mm5+mm4; a1+b1 a0+b0
  movq mm3, [tab_i_17_xmm]      ; 3     ; w05 w04 w01 w00
  psubd mm7, mm2                ; ; a3-b3 a2-b2
  paddd mm0, mm2                ; 0 free a3+b3 a2+b2
  pshufw mm2, [_ECX+16],11011101b; x3 x1 x3 x1
  pmaddwd mm3, mm5              ; x2*w05+x0*w04 x2*w01+x0*w00
  pmaddwd mm5, [tab_i_17_xmm+16]; x2*w13+x0*w12 x2*w09+x0*w08
  psrad mm4, SHIFT_INV_ROW      ; y6=a1-b1 y7=a0-b0
  psrad mm7, SHIFT_INV_ROW      ; y4=a3-b3 y5=a2-b2
  psrad mm6, SHIFT_INV_ROW      ; y1=a1+b1 y0=a0+b0
  psrad mm0, SHIFT_INV_ROW      ; y3=a3+b3 y2=a2+b2
  packssdw mm7, mm4             ; 4     ; y6 y7 y4 y5
  packssdw mm6, mm0             ; 0 free    ; y3 y2 y1 y0
  pshufw mm0, [_ECX+16+8],11011101b  ; x7 x5 x7 x5
  movq [_ECX+0], mm6             ; 3     ; save y3 y2 y1 y0 stall2

; DCT_8_INV_ROW_1_s [_ECX+16], 16, tab_i_17_xmm, rounder_1
  movq mm4, [tab_i_17_xmm+8]    ; 4     ; w07 w06 w03 w02
  movq mm6, [tab_i_17_xmm+32]   ; 6     ; w21 w20 w17 w16
  pshufw mm7, mm7, 10110001b    ; y7 y6 y5 y4
  paddd mm3, [rounder_1]        ; +rounder stall 6
  paddd mm5, [rounder_1]        ; +rounder
  movq [_ECX+0+8], mm7           ; 7     ; save y7 y6 y5 y4
  movq mm7, [tab_i_17_xmm+40]   ; 7     ; w23 w22 w19 w18
  pmaddwd mm4, mm1              ; x6*w07+x4*w06 x6*w03+x4*w02
  pmaddwd mm1, [tab_i_17_xmm+24]; x6*w15+x4*w14 x6*w11+x4*w10
  pmaddwd mm6, mm2              ; x3*w21+x1*w20 x3*w17+x1*w16
  pmaddwd mm2, [tab_i_17_xmm+48]; x3*w29+x1*w28 x3*w25+x1*w24
  pmaddwd mm7, mm0              ; 7     ; x7*w23+x5*w22 x7*w19+x5*w18 ; w23 w22 w19 w18
  pmaddwd mm0, [tab_i_17_xmm+56]; x7*w31+x5*w30 x7*w27+x5*w26
  paddd mm3, mm4                ; 4 free    ; a1=sum(even1) a0=sum(even0)
  paddd mm5, mm1                ; 1 free    ; a3=sum(even3) a2=sum(even2)
  pshufw mm1, [_ECX+32+8],10001000b  ; x6 x4 x6 x4
  movq mm4, mm3                 ; 4     ; a1 a0
  paddd mm6, mm7                ; 7 free    ; b1=sum(odd1) b0=sum(odd0)
  paddd mm2, mm0                ; 5 free    ; b3=sum(odd3) b2=sum(odd2)
  pshufw mm0, [_ECX+32],10001000b; x2 x0 x2 x0
  movq mm7, mm5                 ; 7     ; a3 a2
  psubd mm4, mm6                ; 6 free    ; a1-b1 a0-b0 stall 5
  paddd mm6, mm3                ; mm3 = mm3+mm6+mm5+mm4; a1+b1 a0+b0
  movq mm3, [tab_i_26_xmm]      ; 3     ; w05 w04 w01 w00
  psubd mm7, mm2                ; ; a3-b3 a2-b2
  paddd mm5, mm2                ; 0 free a3+b3 a2+b2
  pshufw mm2, [_ECX+32],11011101b; x3 x1 x3 x1
  pmaddwd mm3, mm0              ; x2*w05+x0*w04 x2*w01+x0*w00
  pmaddwd mm0, [tab_i_26_xmm+16]; x2*w13+x0*w12 x2*w09+x0*w08
  psrad mm4, SHIFT_INV_ROW      ; y6=a1-b1 y7=a0-b0
  psrad mm7, SHIFT_INV_ROW      ; y4=a3-b3 y5=a2-b2
  psrad mm6, SHIFT_INV_ROW      ; y1=a1+b1 y0=a0+b0
  psrad mm5, SHIFT_INV_ROW      ; y3=a3+b3 y2=a2+b2
  packssdw mm7, mm4             ; 4     ; y6 y7 y4 y5
  packssdw mm6, mm5             ; 0 free    ; y3 y2 y1 y0
  pshufw mm5, [_ECX+32+8],11011101b  ; x7 x5 x7 x5
  movq [_ECX+16], mm6            ; 3     ; save y3 y2 y1 y0

;   DCT_8_INV_ROW_1_s [_ECX+32], 32, tab_i_26_xmm, rounder_2
  movq mm4, [tab_i_26_xmm+8]    ; 4     ; w07 w06 w03 w02
  movq mm6, [tab_i_26_xmm+32]   ; 6     ; w21 w20 w17 w16
  pshufw mm7, mm7, 10110001b    ; y7 y6 y5 y4 STALL 6
  paddd mm3, [rounder_2]        ; +rounder
  paddd mm0, [rounder_2]        ; +rounder
  movq [_ECX+16+8], mm7          ; 7     ; save y7 y6
  movq mm7, [tab_i_26_xmm+40]   ; 7     ; w23 w22 w19 w18
  pmaddwd mm4, mm1              ; x6*w07+x4*w06 x6*w03+x4*w02
  pmaddwd mm1, [tab_i_26_xmm+24]; x6*w15+x4*w14 x6*w11+x4*w10
  pmaddwd mm6, mm2              ; x3*w21+x1*w20 x3*w17+x1*w16
  pmaddwd mm2, [tab_i_26_xmm+48]; x3*w29+x1*w28 x3*w25+x1*w24
  pmaddwd mm7, mm5              ; 7     ; x7*w23+x5*w22 x7*w19+x5*w18 ; w23 w22 w19 w18
  pmaddwd mm5, [tab_i_26_xmm+56]; x7*w31+x5*w30 x7*w27+x5*w26
  paddd mm3, mm4                ; 4 free    ; a1=sum(even1) a0=sum(even0)
  paddd mm0, mm1                ; 1 free    ; a3=sum(even3) a2=sum(even2)
  pshufw mm1, [_ECX+48+8],10001000b      ; x6 x4 x6 x4
  movq mm4, mm3                 ; 4     ; a1 a0
  paddd mm6, mm7                ; 7 free    ; b1=sum(odd1) b0=sum(odd0)
  paddd mm2, mm5                ; 5 free    ; b3=sum(odd3) b2=sum(odd2)
  pshufw mm5, [_ECX+48],10001000b; x2 x0 x2 x0   mm5 & mm0 exchanged for next cycle
  movq mm7, mm0                 ; 7     ; a3 a2
  psubd mm4, mm6                ; 6 free    ; a1-b1 a0-b0
  paddd mm6, mm3                ; mm6 = mm3+mm6+mm5+mm4; a1+b1 a0+b0
  movq mm3, [tab_i_35_xmm]      ; 3     ; w05 w04 w01 w00
  psubd mm7, mm2                ; ; a3-b3 a2-b2
  paddd mm0, mm2                ; 0 free a3+b3 a2+b2
  pshufw mm2, [_ECX+48],11011101b; x3 x1 x3 x1
  pmaddwd mm3, mm5              ; x2*w05+x0*w04 x2*w01+x0*w00
  pmaddwd mm5, [tab_i_35_xmm+16]; x2*w13+x0*w12 x2*w09+x0*w08
  psrad mm4, SHIFT_INV_ROW      ; y6=a1-b1 y7=a0-b0
  psrad mm7, SHIFT_INV_ROW      ; y4=a3-b3 y5=a2-b2
  psrad mm6, SHIFT_INV_ROW      ; y1=a1+b1 y0=a0+b0
  psrad mm0, SHIFT_INV_ROW      ; y3=a3+b3 y2=a2+b2
  packssdw mm7, mm4             ; 4     ; y6 y7 y4 y5
  packssdw mm6, mm0             ; 0 free    ; y3 y2 y1 y0
  pshufw mm0, [_ECX+48+8],11011101b  ; x7 x5 x7 x5
  movq [_ECX+32], mm6            ; 3     ; save y3 y2 y1 y0 stall2

;   DCT_8_INV_ROW_1_s [_ECX+48], [_ECX+48], tab_i_35_xmm, rounder_3
  movq mm4, [tab_i_35_xmm+8]    ; 4     ; w07 w06 w03 w02
  movq mm6, [tab_i_35_xmm+32]   ; 6     ; w21 w20 w17 w16
  pshufw mm7, mm7, 10110001b    ; y7 y6 y5 y4
  paddd mm3, [rounder_3]        ; +rounder stall 6
  paddd mm5, [rounder_3]        ; +rounder
  movq [_ECX+32+8], mm7          ; 7     ; save y7 y6 y5 y4
  movq mm7, [tab_i_35_xmm+40]   ; 7     ; w23 w22 w19 w18
  pmaddwd mm4, mm1              ; x6*w07+x4*w06 x6*w03+x4*w02
  pmaddwd mm1, [tab_i_35_xmm+24]; x6*w15+x4*w14 x6*w11+x4*w10
  pmaddwd mm6, mm2              ; x3*w21+x1*w20 x3*w17+x1*w16
  pmaddwd mm2, [tab_i_35_xmm+48]; x3*w29+x1*w28 x3*w25+x1*w24
  pmaddwd mm7, mm0              ; 7     ; x7*w23+x5*w22 x7*w19+x5*w18 ; w23 w22 w19 w18
  pmaddwd mm0, [tab_i_35_xmm+56]; x7*w31+x5*w30 x7*w27+x5*w26
  paddd mm3, mm4                ; 4 free    ; a1=sum(even1) a0=sum(even0)
  paddd mm5, mm1                ; mm1 free  ; a3=sum(even3) a2=sum(even2)
  movq mm1, [tg_3_16]
  movq mm4, mm3                 ; 4     ; a1 a0
  paddd mm6, mm7                ; 7 free    ; b1=sum(odd1) b0=sum(odd0)
  paddd mm2, mm0                ; 5 free    ; b3=sum(odd3) b2=sum(odd2)
  movq mm0, [tg_3_16]
  movq mm7, mm5                 ; 7     ; a3 a2
  psubd mm4, mm6                ; 6 free    ; a1-b1 a0-b0
  paddd mm3, mm6                ; mm3 = mm3+mm6+mm5+mm4; a1+b1 a0+b0
  psubd mm7, mm2                ; ; a3-b3 a2-b2
  paddd mm2, mm5                ; 0 free a3+b3 a2+b2
  movq mm5, [_ECX+16*5]
  psrad mm4, SHIFT_INV_ROW      ; y6=a1-b1 y7=a0-b0
  psrad mm7, SHIFT_INV_ROW      ; y4=a3-b3 y5=a2-b2
  psrad mm3, SHIFT_INV_ROW      ; y1=a1+b1 y0=a0+b0
  psrad mm2, SHIFT_INV_ROW      ; y3=a3+b3 y2=a2+b2
  movq mm6, [_ECX+16*1]
  packssdw mm7, mm4             ; 4     ; y6 y7 y4 y5
  movq mm4, [tg_1_16]
  packssdw mm3, mm2             ; 0 free    ; y3 y2 y1 y0
  pshufw mm2, mm7, 10110001b    ; y7 y6 y5 y4

;   DCT_8_INV_COL_4 [_ECX+0],[_ECX+0]
;   movq    mm3,mmword ptr [_ECX+16*3]
  movq mm7, [_ECX+16*7]
  pmulhw mm0, mm3           ; x3*(tg_3_16-1)
  pmulhw mm1, mm5           ; x5*(tg_3_16-1)
  movq [_ECX+48+8], mm2      ; 7     ; save y7 y6 y5 y4
  movq mm2, mm4             ; tg_1_16
  pmulhw mm4, mm7           ; x7*tg_1_16
  paddsw mm0, mm3           ; x3*tg_3_16
  pmulhw mm2, mm6           ; x1*tg_1_16
  paddsw mm1, mm3           ; x3+x5*(tg_3_16-1)
  psubsw mm0, mm5           ; x3*tg_3_16-x5 = tm35
  movq [_ECX+48], mm3        ; 3     ; save y3 y2 y1 y0
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
  movq [_ECX+3*16], mm5      ; save b0
  paddsw mm1, mm2           ; t1+t2
  movq [_ECX+5*16], mm6      ; save b3
  psubsw mm4, mm2           ; t1-t2
  movq mm5, [_ECX+2*16]
  movq mm0, mm7             ; tg_2_16
  movq mm6, [_ECX+6*16]
  pmulhw mm0, mm5           ; x2*tg_2_16
  pmulhw mm7, mm6           ; x6*tg_2_16
; slot
  pmulhw mm1, mm3           ; ocos_4_16*(t1+t2) = b1/2
; slot
  movq mm2, [_ECX+0*16]
  pmulhw mm4, mm3           ; ocos_4_16*(t1-t2) = b2/2
  psubsw mm0, mm6           ; t2*tg_2_16-x6 = tm26
  movq mm3, [_ECX+0*16]      ; x0
  movq mm6, [_ECX+4*16]
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
  movq mm1, [_ECX+3*16]      ; load b0
  psraw mm7, SHIFT_INV_COL  ; dst6
  movq mm4, mm5             ; a0
  psraw mm0, SHIFT_INV_COL  ; dst5
  movq [_ECX+1*16], mm3
  paddsw mm5, mm1           ; a0+b0
  movq [_ECX+2*16], mm6
  psubsw mm4, mm1           ; a0-b0
  movq mm3, [_ECX+5*16]      ; load b3
  psraw mm5, SHIFT_INV_COL  ; dst0
  movq mm6, mm2             ; a3
  psraw mm4, SHIFT_INV_COL  ; dst7
  movq [_ECX+5*16], mm0
  movq mm0, [tg_3_16]
  paddsw mm2, mm3           ; a3+b3
  movq [_ECX+6*16], mm7
  psubsw mm6, mm3           ; a3-b3
  movq mm3, [_ECX+8+16*3]
  movq [_ECX+0*16], mm5
  psraw mm2, SHIFT_INV_COL  ; dst3
  movq [_ECX+7*16], mm4

 ;  DCT_8_INV_COL_4 [_ECX+8],[_ECX+8]
  movq mm1, mm0             ; tg_3_16
  movq mm5, [_ECX+8+16*5]
  psraw mm6, SHIFT_INV_COL  ; dst4
  pmulhw mm0, mm3           ; x3*(tg_3_16-1)
  movq mm4, [tg_1_16]
  pmulhw mm1, mm5           ; x5*(tg_3_16-1)
  movq mm7, [_ECX+8+16*7]
  movq [_ECX+3*16], mm2
  movq mm2, mm4             ; tg_1_16
  movq [_ECX+4*16], mm6
  movq mm6, [_ECX+8+16*1]
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
  psubsw mm4, mm1           ; tp17-tp35 = t1
  paddsw mm2, mm0           ; tm17+tm35 = t2
  movq mm7, [tg_2_16]
  movq mm1, mm4             ; t1
  psubsw mm6, mm0           ; tm17-tm35 = b3
  movq [_ECX+8+3*16], mm5    ; save b0
  movq [_ECX+8+5*16], mm6    ; save b3
  psubsw mm4, mm2           ; t1-t2
  movq mm5, [_ECX+8+2*16]
  movq mm0, mm7             ; tg_2_16
  movq mm6, [_ECX+8+6*16]
  paddsw mm1, mm2           ; t1+t2
  pmulhw mm0, mm5           ; x2*tg_2_16
  pmulhw mm7, mm6           ; x6*tg_2_16
  movq mm2, [_ECX+8+0*16]
  pmulhw mm4, mm3           ; ocos_4_16*(t1-t2) = b2/2
  psubsw mm0, mm6           ; t2*tg_2_16-x6 = tm26
 ; slot
  pmulhw mm1, mm3           ; ocos_4_16*(t1+t2) = b1/2
 ; slot
  movq mm3, [_ECX+8+0*16]    ; x0
  movq mm6, [_ECX+8+4*16]
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
  movq mm1, [_ECX+8+3*16]    ; load b0
  psraw mm7, SHIFT_INV_COL  ; dst6
  movq mm4, mm5             ; a0
  psraw mm0, SHIFT_INV_COL  ; dst5
  movq [_ECX+8+1*16], mm3
  paddsw mm5, mm1           ; a0+b0
  movq [_ECX+8+2*16], mm6
  psubsw mm4, mm1           ; a0-b0
  movq mm3, [_ECX+8+5*16]    ; load b3
  psraw mm5, SHIFT_INV_COL  ; dst0
  movq mm6, mm2         ; a3
  psraw mm4, SHIFT_INV_COL  ; dst7
  movq [_ECX+8+5*16], mm0
  paddsw mm2, mm3           ; a3+b3
  movq [_ECX+8+6*16], mm7
  psubsw mm6, mm3           ; a3-b3
  movq [_ECX+8+0*16], mm5
  psraw mm2, SHIFT_INV_COL  ; dst3
  movq [_ECX+8+7*16], mm4
  psraw mm6, SHIFT_INV_COL  ; dst4
  movq [_ECX+8+3*16], mm2
  movq [_ECX+8+4*16], mm6

  ret
ENDFUNC

NON_EXEC_STACK
