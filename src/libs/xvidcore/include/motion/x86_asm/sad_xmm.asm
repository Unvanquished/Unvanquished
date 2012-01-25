;/****************************************************************************
; *
; *  XVID MPEG-4 VIDEO CODEC
; *  - K7 optimized SAD operators -
; *
; *  Copyright(C) 2001 Peter Ross <pross@xvid.org>
; *               2001-2008 Michael Militzer <michael@xvid.org>
; *               2002 Pascal Massimino <skal@planet-d.net>
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
; * $Id: sad_xmm.asm,v 1.13.2.2 2009/09/16 17:11:39 Isibaar Exp $
; *
; ***************************************************************************/

%include "nasm.inc"

;=============================================================================
; Read only data
;=============================================================================

DATA

ALIGN SECTION_ALIGN
mmx_one: times 4 dw 1

;=============================================================================
; Helper macros
;=============================================================================

%macro SAD_16x16_SSE 0
  movq mm0, [_EAX]
  psadbw mm0, [TMP1]
  movq mm1, [_EAX+8]
  add _EAX, TMP0
  psadbw mm1, [TMP1+8]
  paddusw mm5, mm0
  add TMP1, TMP0
  paddusw mm6, mm1
%endmacro

%macro SAD_8x8_SSE 0
  movq mm0, [_EAX]
  movq mm1, [_EAX+TMP0]
  psadbw mm0, [TMP1]
  psadbw mm1, [TMP1+TMP0]
  add _EAX, _EBX
  add TMP1, _EBX
	paddusw	mm5, mm0
	paddusw	mm6, mm1
%endmacro

%macro SADBI_16x16_SSE 0
  movq mm0, [_EAX]
  movq mm1, [_EAX+8]
  movq mm2, [TMP1]
  movq mm3, [TMP1+8]
  pavgb mm2, [_EBX]
  add TMP1, TMP0
  pavgb mm3, [_EBX+8]
  add _EBX, TMP0
  psadbw mm0, mm2
  add _EAX, TMP0
  psadbw mm1, mm3
  paddusw mm5, mm0
  paddusw mm6, mm1
%endmacro

%macro SADBI_8x8_XMM 0
  movq mm0, [_EAX]
  movq mm1, [_EAX+TMP0]
  movq mm2, [TMP1]
  movq mm3, [TMP1+TMP0]
  pavgb mm2, [_EBX]
  lea TMP1, [TMP1+2*TMP0]
  pavgb mm3, [_EBX+TMP0]
  lea _EBX, [_EBX+2*TMP0]
  psadbw mm0, mm2
  lea _EAX, [_EAX+2*TMP0]
  psadbw mm1, mm3
  paddusw mm5, mm0
  paddusw mm6, mm1
%endmacro

%macro MEAN_16x16_SSE 0
  movq mm0, [_EAX]
  movq mm1, [_EAX+8]
  psadbw mm0, mm7
  psadbw mm1, mm7
  add _EAX, TMP0
  paddw mm5, mm0
  paddw mm6, mm1
%endmacro

%macro ABS_16x16_SSE 0
  movq mm0, [_EAX]
  movq mm1, [_EAX+8]
  psadbw mm0, mm4
  psadbw mm1, mm4
  lea _EAX, [_EAX+TMP0]
  paddw mm5, mm0
  paddw mm6, mm1
%endmacro

;=============================================================================
; Code
;=============================================================================

TEXT

cglobal sad16_xmm
cglobal sad8_xmm
cglobal sad16bi_xmm
cglobal sad8bi_xmm
cglobal dev16_xmm
cglobal sad16v_xmm

;-----------------------------------------------------------------------------
;
; uint32_t sad16_xmm(const uint8_t * const cur,
;					const uint8_t * const ref,
;					const uint32_t stride,
;					const uint32_t best_sad);
;
;-----------------------------------------------------------------------------

ALIGN SECTION_ALIGN
sad16_xmm:

  mov _EAX, prm1 ; Src1
  mov TMP1, prm2 ; Src2
  mov TMP0, prm3 ; Stride

  pxor mm5, mm5 ; accum1
  pxor mm6, mm6 ; accum2

  SAD_16x16_SSE
  SAD_16x16_SSE
  SAD_16x16_SSE
  SAD_16x16_SSE
  SAD_16x16_SSE
  SAD_16x16_SSE
  SAD_16x16_SSE
  SAD_16x16_SSE

  SAD_16x16_SSE
  SAD_16x16_SSE
  SAD_16x16_SSE
  SAD_16x16_SSE
  SAD_16x16_SSE
  SAD_16x16_SSE
  SAD_16x16_SSE
  SAD_16x16_SSE

  paddusw mm6,mm5
  movd eax, mm6
  ret
ENDFUNC


;-----------------------------------------------------------------------------
;
; uint32_t sad8_xmm(const uint8_t * const cur,
;					const uint8_t * const ref,
;					const uint32_t stride);
;
;-----------------------------------------------------------------------------

ALIGN SECTION_ALIGN
sad8_xmm:

  mov _EAX, prm1 ; Src1
  mov TMP1, prm2 ; Src2
  mov TMP0, prm3 ; Stride
  push _EBX
  lea _EBX, [TMP0+TMP0]

  pxor mm5, mm5 ; accum1
  pxor mm6, mm6 ; accum2

  SAD_8x8_SSE
  SAD_8x8_SSE
  SAD_8x8_SSE

  movq mm0, [_EAX]
  movq mm1, [_EAX+TMP0]
  psadbw mm0, [TMP1]
  psadbw mm1, [TMP1+TMP0]

  pop _EBX

  paddusw mm5,mm0
  paddusw mm6,mm1

  paddusw mm6,mm5
  movd eax, mm6

  ret
ENDFUNC


;-----------------------------------------------------------------------------
;
; uint32_t sad16bi_xmm(const uint8_t * const cur,
;					const uint8_t * const ref1,
;					const uint8_t * const ref2,
;					const uint32_t stride);
;
;-----------------------------------------------------------------------------

ALIGN SECTION_ALIGN
sad16bi_xmm:
  mov _EAX, prm1 ; Src
  mov TMP1, prm2 ; Ref1
  mov TMP0, prm4 ; Stride

  push _EBX
%ifdef ARCH_IS_X86_64
  mov _EBX, prm3
%else
  mov _EBX, [_ESP+4+12] ; Ref2
%endif
  pxor mm5, mm5 ; accum1
  pxor mm6, mm6 ; accum2

  SADBI_16x16_SSE
  SADBI_16x16_SSE
  SADBI_16x16_SSE
  SADBI_16x16_SSE
  SADBI_16x16_SSE
  SADBI_16x16_SSE
  SADBI_16x16_SSE
  SADBI_16x16_SSE

  SADBI_16x16_SSE
  SADBI_16x16_SSE
  SADBI_16x16_SSE
  SADBI_16x16_SSE
  SADBI_16x16_SSE
  SADBI_16x16_SSE
  SADBI_16x16_SSE
  SADBI_16x16_SSE

  paddusw mm6,mm5
  movd eax, mm6
  pop _EBX
  ret
ENDFUNC

;-----------------------------------------------------------------------------
;
; uint32_t sad8bi_xmm(const uint8_t * const cur,
; const uint8_t * const ref1,
; const uint8_t * const ref2,
; const uint32_t stride);
;
;-----------------------------------------------------------------------------

ALIGN SECTION_ALIGN
sad8bi_xmm:
  mov _EAX, prm1 ; Src
  mov TMP1, prm2 ; Ref1
  mov TMP0, prm4 ; Stride

  push _EBX
%ifdef ARCH_IS_X86_64
  mov _EBX, prm3
%else
  mov _EBX, [_ESP+4+12] ; Ref2
%endif

  pxor mm5, mm5 ; accum1
  pxor mm6, mm6 ; accum2
.Loop:
  SADBI_8x8_XMM
  SADBI_8x8_XMM
  SADBI_8x8_XMM
  SADBI_8x8_XMM

  paddusw mm6,mm5
  movd eax, mm6
  pop _EBX
  ret
ENDFUNC


;-----------------------------------------------------------------------------
;
; uint32_t dev16_xmm(const uint8_t * const cur,
;					const uint32_t stride);
;
;-----------------------------------------------------------------------------

ALIGN SECTION_ALIGN
dev16_xmm:

  mov _EAX, prm1 ; Src
  mov TMP0, prm2 ; Stride

  pxor mm7, mm7 ; zero
  pxor mm5, mm5 ; mean accums
  pxor mm6, mm6

  MEAN_16x16_SSE
  MEAN_16x16_SSE
  MEAN_16x16_SSE
  MEAN_16x16_SSE
  MEAN_16x16_SSE
  MEAN_16x16_SSE
  MEAN_16x16_SSE
  MEAN_16x16_SSE

  MEAN_16x16_SSE
  MEAN_16x16_SSE
  MEAN_16x16_SSE
  MEAN_16x16_SSE
  MEAN_16x16_SSE
  MEAN_16x16_SSE
  MEAN_16x16_SSE
  MEAN_16x16_SSE

  paddusw mm6, mm5

  movq mm4, mm6
  psllq mm4, 32
  paddd mm4, mm6
  psrld mm4, 8	  ; /= (16*16)

  packssdw mm4, mm4
  packuswb mm4, mm4

	; mm4 contains the mean

  mov _EAX, prm1 ; Src


  pxor mm5, mm5 ; sums
  pxor mm6, mm6

  ABS_16x16_SSE
  ABS_16x16_SSE
  ABS_16x16_SSE
  ABS_16x16_SSE
  ABS_16x16_SSE
  ABS_16x16_SSE
  ABS_16x16_SSE
  ABS_16x16_SSE

  ABS_16x16_SSE
  ABS_16x16_SSE
  ABS_16x16_SSE
  ABS_16x16_SSE
  ABS_16x16_SSE
  ABS_16x16_SSE
  ABS_16x16_SSE
  ABS_16x16_SSE

  paddusw mm6, mm5
  movq mm7, mm6
  psllq mm7, 32
  paddd mm6, mm7

  movd eax, mm6
  ret
ENDFUNC

;-----------------------------------------------------------------------------
;int sad16v_xmm(const uint8_t * const cur,
;               const uint8_t * const ref,
;               const uint32_t stride,
;               int* sad8);
;-----------------------------------------------------------------------------

ALIGN SECTION_ALIGN
sad16v_xmm:
  mov _EAX, prm1 ; Src1
  mov TMP1, prm2 ; Src2
  mov TMP0, prm3 ; Stride

  push _EBX
%ifdef ARCH_IS_X86_64
  mov _EBX, prm4
%else
  mov _EBX, [_ESP+4+16] ; sad ptr
%endif

  pxor mm5, mm5 ; accum1
  pxor mm6, mm6 ; accum2
  pxor mm7, mm7 ; total

  SAD_16x16_SSE
  SAD_16x16_SSE
  SAD_16x16_SSE
  SAD_16x16_SSE
  SAD_16x16_SSE
  SAD_16x16_SSE
  SAD_16x16_SSE
  SAD_16x16_SSE

  paddusw mm7, mm5
  paddusw mm7, mm6
  movd [_EBX], mm5
  movd [_EBX+4], mm6

  pxor mm5, mm5 ; accum1
  pxor mm6, mm6 ; accum2

  SAD_16x16_SSE
  SAD_16x16_SSE
  SAD_16x16_SSE
  SAD_16x16_SSE
  SAD_16x16_SSE
  SAD_16x16_SSE
  SAD_16x16_SSE
  SAD_16x16_SSE

  paddusw mm7, mm5
  paddusw mm7, mm6
  movd [_EBX+8], mm5
  movd [_EBX+12], mm6

  movd eax, mm7
  pop _EBX
  ret
ENDFUNC

NON_EXEC_STACK
