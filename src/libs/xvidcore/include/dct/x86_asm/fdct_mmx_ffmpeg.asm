;/****************************************************************************
; *
; *  XVID MPEG-4 VIDEO CODEC
; *  - MMX and XMM forward discrete cosine transform -
; *
; *  Copyright(C) 2003 Edouard Gomez <ed.gomez@free.fr>
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
; * $Id: fdct_mmx_ffmpeg.asm,v 1.8.2.2 2009/09/16 17:11:39 Isibaar Exp $
; *
; ***************************************************************************/

;/****************************************************************************
; *
; *  Initial, but incomplete version provided by Intel at AppNote AP-922
; *    http://developer.intel.com/vtune/cbts/strmsimd/922down.htm
; *  Copyright (C) 1999 Intel Corporation
; *
; *  Completed and corrected in fdctmm32.c/fdctmm32.doc
; *    http://members.tripod.com/~liaor/
; *  Copyright (C) 2000 - Royce Shih-Wea Liao <liaor@iname.com>
; *
; *  Minimizing coefficients reordering changing the tables constants order
; *    http://ffmpeg.sourceforge.net/
; *  Copyright (C) 2001 Fabrice Bellard.
; *
; *  The version coded here is just a port to NASM syntax from the FFMPEG's
; *  version. So all credits go to the previous authors for all their
; *  respective work in order to have a nice/fast mmx fDCT.
; ***************************************************************************/

;=============================================================================
; Macros and other preprocessor constants
;=============================================================================

%include "nasm.inc"

;;; Define this if you want an unrolled version of the code
%define UNROLLED_LOOP

%define BITS_FRW_ACC   3
%define SHIFT_FRW_COL  BITS_FRW_ACC
%define SHIFT_FRW_ROW  (BITS_FRW_ACC + 17)
%define RND_FRW_ROW    (1 << (SHIFT_FRW_ROW-1))
%define RND_FRW_COL    (1 << (SHIFT_FRW_COL-1))

;=============================================================================
; Local Data (Read Only)
;=============================================================================

DATA

ALIGN SECTION_ALIGN
tab_frw_01234567:
  dw  16384,   16384,   -8867,  -21407
  dw  16384,   16384,   21407,    8867
  dw  16384,  -16384,   21407,   -8867
  dw -16384,   16384,    8867,  -21407
  dw  22725,   19266,  -22725,  -12873
  dw  12873,    4520,   19266,   -4520
  dw  12873,  -22725,   19266,  -22725
  dw   4520,   19266,    4520,  -12873

  dw  22725,   22725,  -12299,  -29692
  dw  22725,   22725,   29692,   12299
  dw  22725,  -22725,   29692,  -12299
  dw -22725,   22725,   12299,  -29692
  dw  31521,   26722,  -31521,  -17855
  dw  17855,    6270,   26722,   -6270
  dw  17855,  -31521,   26722,  -31521
  dw   6270,   26722,    6270,  -17855

  dw  21407,   21407,  -11585,  -27969
  dw  21407,   21407,   27969,   11585
  dw  21407,  -21407,   27969,  -11585
  dw -21407,   21407,   11585,  -27969
  dw  29692,   25172,  -29692,  -16819
  dw  16819,    5906,   25172,   -5906
  dw  16819,  -29692,   25172,  -29692
  dw   5906,   25172,    5906,  -16819

  dw  19266,   19266,  -10426,  -25172
  dw  19266,   19266,   25172,   10426
  dw  19266,  -19266,   25172,  -10426
  dw -19266,   19266,   10426,  -25172
  dw  26722,   22654,  -26722,  -15137
  dw  15137,    5315,   22654,   -5315
  dw  15137,  -26722,   22654,  -26722
  dw   5315,   22654,    5315,  -15137

  dw  16384,   16384,   -8867,  -21407
  dw  16384,   16384,   21407,    8867
  dw  16384,  -16384,   21407,   -8867
  dw -16384,   16384,    8867,  -21407
  dw  22725,   19266,  -22725,  -12873
  dw  12873,    4520,   19266,   -4520
  dw  12873,  -22725,   19266,  -22725
  dw   4520,   19266,    4520,  -12873

  dw  19266,   19266,  -10426,  -25172
  dw  19266,   19266,   25172,   10426
  dw  19266,  -19266,   25172,  -10426
  dw -19266,   19266,   10426,  -25172
  dw  26722,   22654,  -26722,  -15137
  dw  15137,    5315,   22654,   -5315
  dw  15137,  -26722,   22654,  -26722
  dw   5315,   22654,    5315,  -15137

  dw  21407,   21407,  -11585,  -27969
  dw  21407,   21407,   27969,   11585
  dw  21407,  -21407,   27969,  -11585
  dw -21407,   21407,   11585,  -27969
  dw  29692,   25172,  -29692,  -16819
  dw  16819,    5906,   25172,   -5906
  dw  16819,  -29692,   25172,  -29692
  dw   5906,   25172,    5906,  -16819,

  dw  22725,   22725,  -12299,  -29692
  dw  22725,   22725,   29692,   12299
  dw  22725,  -22725,   29692,  -12299
  dw -22725,   22725,   12299,  -29692
  dw  31521,   26722,  -31521,  -17855
  dw  17855,    6270,   26722,   -6270
  dw  17855,  -31521,   26722,  -31521
  dw   6270,   26722,    6270,  -17855

ALIGN SECTION_ALIGN
fdct_one_corr:
  dw 1, 1, 1, 1

ALIGN SECTION_ALIGN
fdct_tg_all_16:
  dw  13036,	13036,	13036,	13036
  dw  27146,	27146,	27146,	27146
  dw -21746, -21746, -21746, -21746

ALIGN SECTION_ALIGN
cos_4_16:
  dw -19195, -19195, -19195, -19195

ALIGN SECTION_ALIGN
ocos_4_16:
  dw 23170, 23170, 23170, 23170

ALIGN SECTION_ALIGN
fdct_r_row:
  dd RND_FRW_ROW, RND_FRW_ROW

;=============================================================================
; Factorized parts of the code turned into macros for better understanding
;=============================================================================

	;; Macro for column DCT
	;; FDCT_COLUMN_MMX(int16_t *out, const int16_t *in, int offset);
	;;  - out, register name holding the out address
	;;  - in, register name holding the in address
	;;  - column number to process
%macro FDCT_COLUMN_COMMON 3
  movq mm0, [%2 + %3*2 + 1*16]
  movq mm1, [%2 + %3*2 + 6*16]
  movq mm2, mm0
  movq mm3, [%2 + %3*2 + 2*16]
  paddsw mm0, mm1
  movq mm4, [%2 + %3*2 + 5*16]
  psllw mm0, SHIFT_FRW_COL
  movq mm5, [%2 + %3*2 + 0*16]
  paddsw mm4, mm3
  paddsw mm5, [%2 + %3*2 + 7*16]
  psllw mm4, SHIFT_FRW_COL
  movq mm6, mm0
  psubsw mm2, mm1
  movq mm1, [fdct_tg_all_16 + 4*2]
  psubsw mm0, mm4
  movq mm7, [%2 + %3*2 + 3*16]
  pmulhw mm1, mm0
  paddsw mm7, [%2 + %3*2 + 4*16]
  psllw mm5, SHIFT_FRW_COL
  paddsw mm6, mm4
  psllw mm7, SHIFT_FRW_COL
  movq mm4, mm5
  psubsw mm5, mm7
  paddsw mm1, mm5
  paddsw mm4, mm7
  por mm1, [fdct_one_corr]
  psllw mm2, SHIFT_FRW_COL + 1
  pmulhw mm5, [fdct_tg_all_16 + 4*2]
  movq mm7, mm4
  psubsw mm3, [%2 + %3*2 + 5*16]
  psubsw mm4, mm6
  movq [%1 + %3*2 + 2*16], mm1
  paddsw mm7, mm6
  movq mm1, [%2 + %3*2 + 3*16]
  psllw mm3, SHIFT_FRW_COL + 1
  psubsw mm1, [%2 + %3*2 + 4*16]
  movq mm6, mm2
  movq [%1 + %3*2 + 4*16], mm4
  paddsw mm2, mm3
  pmulhw mm2, [ocos_4_16]
  psubsw mm6, mm3
  pmulhw mm6, [ocos_4_16]
  psubsw mm5, mm0
  por mm5, [fdct_one_corr]
  psllw mm1, SHIFT_FRW_COL
  por mm2, [fdct_one_corr]
  movq mm4, mm1
  movq mm3, [%2 + %3*2 + 0*16]
  paddsw mm1, mm6
  psubsw mm3, [%2 + %3*2 + 7*16]
  psubsw mm4, mm6
  movq mm0, [fdct_tg_all_16 + 0*2]
  psllw mm3, SHIFT_FRW_COL
  movq mm6, [fdct_tg_all_16 + 8*2]
  pmulhw mm0, mm1
  movq [%1 + %3*2 + 0*16], mm7
  pmulhw mm6, mm4
  movq [%1 + %3*2 + 6*16], mm5
  movq mm7, mm3
  movq mm5, [fdct_tg_all_16 + 8*2]
  psubsw mm7, mm2
  paddsw mm3, mm2
  pmulhw mm5, mm7
  paddsw mm0, mm3
  paddsw mm6, mm4
  pmulhw mm3, [fdct_tg_all_16 + 0*2]
  por mm0, [fdct_one_corr]
  paddsw mm5, mm7
  psubsw mm7, mm6
  movq [%1 + %3*2 + 1*16], mm0
  paddsw mm5, mm4
  movq [%1 + %3*2 + 3*16], mm7
  psubsw mm3, mm1
  movq [%1 + %3*2 + 5*16], mm5
  movq [%1 + %3*2 + 7*16], mm3
%endmacro

	;; Macro for row DCT using MMX punpcklw instructions
	;; FDCT_ROW_MMX(int16_t *out, const int16_t *in, const int16_t *table);
	;;  - out, register name holding the out address
	;;  - in, register name holding the in address
	;;  - table coefficients address (register or absolute)
%macro FDCT_ROW_MMX 3
  movd mm1, [%2 + 6*2]
  punpcklwd mm1, [%2 + 4*2]
  movq mm2, mm1
  psrlq mm1, 0x20
  movq mm0, [%2 + 0*2]
  punpcklwd mm1, mm2
  movq mm5, mm0
  paddsw mm0, mm1
  psubsw mm5, mm1
  movq mm1, mm0
  movq mm6, mm5
  punpckldq mm3, mm5
  punpckhdq mm6, mm3
  movq mm3, [%3 + 0*2]
  movq mm4, [%3 + 4*2]
  punpckldq mm2, mm0
  pmaddwd mm3, mm0
  punpckhdq mm1, mm2
  movq mm2, [%3 + 16*2]
  pmaddwd mm4, mm1
  pmaddwd mm0, [%3 + 8*2]
  movq mm7, [%3 + 20*2]
  pmaddwd mm2, mm5
  paddd mm3, [fdct_r_row]
  pmaddwd mm7, mm6
  pmaddwd mm1, [%3 + 12*2]
  paddd mm3, mm4
  pmaddwd mm5, [%3 + 24*2]
  pmaddwd mm6, [%3 + 28*2]
  paddd mm2, mm7
  paddd mm0, [fdct_r_row]
  psrad mm3, SHIFT_FRW_ROW
  paddd mm2, [fdct_r_row]
  paddd mm0, mm1
  paddd mm5, [fdct_r_row]
  psrad mm2, SHIFT_FRW_ROW
  paddd mm5, mm6
  psrad mm0, SHIFT_FRW_ROW
  psrad mm5, SHIFT_FRW_ROW
  packssdw mm3, mm0
  packssdw mm2, mm5
  movq mm6, mm3
  punpcklwd mm3, mm2
  punpckhwd mm6, mm2
  movq [%1 + 0*2], mm3
  movq [%1 + 4*2], mm6
%endmacro

	;; Macro for column DCT using XMM instuction pshufw
	;; FDCT_ROW_XMM(int16_t *out, const int16_t *in, const int16_t *table);
	;;  - out, register name holding the out address
	;;  - in, register name holding the in address
	;;  - table coefficient address
%macro FDCT_ROW_XMM 3
	;; fdct_row_mmx2(const int16_t *in, int16_t *out, const int16_t *table)
  pshufw mm5, [%2 + 4*2], 0x1B
  movq mm0, [%2 + 0*2]
  movq mm1, mm0
  paddsw mm0, mm5
  psubsw mm1, mm5
  pshufw mm2, mm0, 0x4E
  pshufw mm3, mm1, 0x4E
  movq mm4, [%3 +  0*2]
  movq mm6, [%3 +  4*2]
  movq mm5, [%3 + 16*2]
  movq mm7, [%3 + 20*2]
  pmaddwd mm4, mm0
  pmaddwd mm5, mm1
  pmaddwd mm6, mm2
  pmaddwd mm7, mm3
  pmaddwd mm0, [%3 +  8*2]
  pmaddwd mm2, [%3 + 12*2]
  pmaddwd mm1, [%3 + 24*2]
  pmaddwd mm3, [%3 + 28*2]
  paddd mm4, mm6
  paddd mm5, mm7
  paddd mm0, mm2
  paddd mm1, mm3
  movq mm7, [fdct_r_row]
  paddd mm4, mm7
  paddd mm5, mm7
  paddd mm0, mm7
  paddd mm1, mm7
  psrad mm4, SHIFT_FRW_ROW
  psrad mm5, SHIFT_FRW_ROW
  psrad mm0, SHIFT_FRW_ROW
  psrad mm1, SHIFT_FRW_ROW
  packssdw mm4, mm0
  packssdw mm5, mm1
  movq mm2, mm4
  punpcklwd mm4, mm5
  punpckhwd mm2, mm5
  movq [%1 + 0*2], mm4
  movq [%1 + 4*2], mm2
%endmacro

%macro MAKE_FDCT_FUNC 2
ALIGN SECTION_ALIGN
cglobal %1
%1:
	;; Move the destination/source address to the eax register
  mov _EAX, prm1

	;; Process the columns (4 at a time)
  FDCT_COLUMN_COMMON _EAX, _EAX, 0 ; columns 0..3
  FDCT_COLUMN_COMMON _EAX, _EAX, 4 ; columns 4..7

%ifdef UNROLLED_LOOP
	; Unrolled loop version
%assign i 0
%rep 8
	;; Process the 'i'th row
  %2 _EAX+2*i*8, _EAX+2*i*8, tab_frw_01234567+2*32*i
	%assign i i+1
%endrep
%else
  mov _ECX, 8
  mov _EDX, tab_frw_01234567
ALIGN SECTION_ALIGN
.loop
  %2 _EAX, _EAX,_EDX
  add _EAX, 2*8
  add _EDX, 2*32
  dec _ECX
  jne .loop
%endif

  ret
ENDFUNC
%endmacro

;=============================================================================
; Code
;=============================================================================

TEXT

;-----------------------------------------------------------------------------
; void fdct_mmx_ffmpeg(int16_t block[64]);
;-----------------------------------------------------------------------------

MAKE_FDCT_FUNC fdct_mmx_ffmpeg, FDCT_ROW_MMX

;-----------------------------------------------------------------------------
; void fdct_xmm_ffmpeg(int16_t block[64]);
;-----------------------------------------------------------------------------

MAKE_FDCT_FUNC fdct_xmm_ffmpeg, FDCT_ROW_XMM

NON_EXEC_STACK
