;/****************************************************************************
; *
; *  XVID MPEG-4 VIDEO CODEC
; *  - SSE2 optimized SAD operators -
; *
; *  Copyright(C) 2003 Pascal Massimino <skal@planet-d.net>
; *
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
; * $Id: sad_sse2.asm,v 1.16.2.3 2009/09/16 17:11:39 Isibaar Exp $
; *
; ***************************************************************************/

%include "nasm.inc"

;=============================================================================
; Read only data
;=============================================================================

DATA

ALIGN SECTION_ALIGN
zero    times 4   dd 0

;=============================================================================
; Code
;=============================================================================

TEXT

cglobal  sad16_sse2
cglobal  dev16_sse2

cglobal  sad16_sse3
cglobal  dev16_sse3

;-----------------------------------------------------------------------------
; uint32_t sad16_sse2 (const uint8_t * const cur, <- assumed aligned!
;                      const uint8_t * const ref,
;	                   const uint32_t stride,
;                      const uint32_t /*ignored*/);
;-----------------------------------------------------------------------------


%macro SAD_16x16_SSE2 1
  %1  xmm0, [TMP1]
  %1  xmm1, [TMP1+TMP0]
  lea TMP1,[TMP1+2*TMP0]
  movdqa  xmm2, [_EAX]
  movdqa  xmm3, [_EAX+TMP0]
  lea _EAX,[_EAX+2*TMP0]
  psadbw  xmm0, xmm2
  paddusw xmm6,xmm0
  psadbw  xmm1, xmm3
  paddusw xmm6,xmm1
%endmacro

%macro SAD16_SSE2_SSE3 1
  PUSH_XMM6_XMM7
  mov _EAX, prm1 ; cur (assumed aligned)
  mov TMP1, prm2 ; ref
  mov TMP0, prm3 ; stride

  pxor xmm6, xmm6 ; accum

  SAD_16x16_SSE2 %1
  SAD_16x16_SSE2 %1
  SAD_16x16_SSE2 %1
  SAD_16x16_SSE2 %1
  SAD_16x16_SSE2 %1
  SAD_16x16_SSE2 %1
  SAD_16x16_SSE2 %1
  SAD_16x16_SSE2 %1

  pshufd  xmm5, xmm6, 00000010b
  paddusw xmm6, xmm5
  pextrw  eax, xmm6, 0

  POP_XMM6_XMM7
  ret
%endmacro

ALIGN SECTION_ALIGN
sad16_sse2:
  SAD16_SSE2_SSE3 movdqu
ENDFUNC


ALIGN SECTION_ALIGN
sad16_sse3:
  SAD16_SSE2_SSE3 lddqu
ENDFUNC


;-----------------------------------------------------------------------------
; uint32_t dev16_sse2(const uint8_t * const cur, const uint32_t stride);
;-----------------------------------------------------------------------------

%macro MEAN_16x16_SSE2 1  ; _EAX: src, TMP0:stride, mm7: zero or mean => mm6: result
  %1 xmm0, [_EAX]
  %1 xmm1, [_EAX+TMP0]
  lea _EAX, [_EAX+2*TMP0]    ; + 2*stride
  psadbw xmm0, xmm7
  paddusw xmm6, xmm0
  psadbw xmm1, xmm7
  paddusw xmm6, xmm1
%endmacro


%macro MEAN16_SSE2_SSE3 1
  PUSH_XMM6_XMM7
  mov _EAX, prm1   ; src
  mov TMP0, prm2   ; stride

  pxor xmm6, xmm6     ; accum
  pxor xmm7, xmm7     ; zero

  MEAN_16x16_SSE2 %1
  MEAN_16x16_SSE2 %1
  MEAN_16x16_SSE2 %1
  MEAN_16x16_SSE2 %1

  MEAN_16x16_SSE2 %1
  MEAN_16x16_SSE2 %1
  MEAN_16x16_SSE2 %1
  MEAN_16x16_SSE2 %1

  mov _EAX, prm1       ; src again

  pshufd   xmm7, xmm6, 10b
  paddusw  xmm7, xmm6
  pxor     xmm6, xmm6     ; zero accum
  psrlw    xmm7, 8        ; => Mean
  pshuflw  xmm7, xmm7, 0  ; replicate Mean
  packuswb xmm7, xmm7
  pshufd   xmm7, xmm7, 00000000b

  MEAN_16x16_SSE2 %1
  MEAN_16x16_SSE2 %1
  MEAN_16x16_SSE2 %1
  MEAN_16x16_SSE2 %1

  MEAN_16x16_SSE2 %1
  MEAN_16x16_SSE2 %1
  MEAN_16x16_SSE2 %1
  MEAN_16x16_SSE2 %1

  pshufd   xmm7, xmm6, 10b
  paddusw  xmm7, xmm6
  pextrw eax, xmm7, 0

  POP_XMM6_XMM7
  ret
%endmacro

ALIGN SECTION_ALIGN
dev16_sse2:
  MEAN16_SSE2_SSE3 movdqu
ENDFUNC

ALIGN SECTION_ALIGN
dev16_sse3:
  MEAN16_SSE2_SSE3 lddqu
ENDFUNC

NON_EXEC_STACK
