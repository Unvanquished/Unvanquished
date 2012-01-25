;/****************************************************************************
; *
; *  XVID MPEG-4 VIDEO CODEC
; *  - K7 optimized SAD operators -
; *
; *  Copyright(C) 2002 Jaan Kalda
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
; * $Id: sad_3dne.asm,v 1.10.2.2 2009/09/16 17:11:39 Isibaar Exp $
; *
; ***************************************************************************/

; these 3dne functions are compatible with iSSE, but are optimized specifically
; for K7 pipelines

%include "nasm.inc"

;=============================================================================
; Read only data
;=============================================================================

DATA

ALIGN SECTION_ALIGN
mmx_one:
	times 4	dw 1

;=============================================================================
; Helper macros
;=============================================================================

	;; %1 block number (0..4)
%macro SAD_16x16_SSE 1
  movq mm7, [_EAX]
  movq mm6, [_EAX+8]
  psadbw mm7, [TMP1]
  psadbw mm6, [TMP1+8]
%if (%1)
  paddd mm1, mm5
%endif
  movq mm5, [_EAX+TMP0]
  movq mm4, [_EAX+TMP0+8]
  psadbw mm5, [TMP1+TMP0]
  psadbw mm4, [TMP1+TMP0+8]
  movq mm3, [_EAX+2*TMP0]
  movq mm2, [_EAX+2*TMP0+8]
  psadbw mm3, [TMP1+2*TMP0]
  psadbw mm2, [TMP1+2*TMP0+8]
%if (%1)
  movd [_ESP+4*(%1-1)], mm1
%else
  sub _ESP, byte 12
%endif
  movq mm1, [_EAX+_EBX]
  movq mm0, [_EAX+_EBX+8]
  psadbw mm1, [TMP1+_EBX]
  psadbw mm0, [TMP1+_EBX+8]
  lea _EAX, [_EAX+4*TMP0]
  lea TMP1, [TMP1+4*TMP0]
  paddd mm7, mm6
  paddd mm5, mm4
  paddd mm3, mm2
  paddd mm1, mm0
  paddd mm5, mm7
  paddd mm1, mm3
%endmacro

%macro SADBI_16x16_SSE0 0
  movq mm2, [TMP1]
  movq mm3, [TMP1+8]

  movq mm5, [byte _EAX]
  movq mm6, [_EAX+8]
  pavgb mm2, [byte _EBX]
  pavgb mm3, [_EBX+8]

  add TMP1, TMP0
  psadbw mm5, mm2
  psadbw mm6, mm3

  add _EAX, TMP0
  add _EBX, TMP0
  movq mm2, [byte TMP1]

  movq mm3, [TMP1+8]
  movq mm0, [byte _EAX]

  movq mm1, [_EAX+8]
  pavgb mm2, [byte _EBX]

  pavgb mm3, [_EBX+8]
  add TMP1, TMP0
  add _EAX, TMP0

  add _EBX, TMP0
  psadbw mm0, mm2
  psadbw mm1, mm3

%endmacro

%macro SADBI_16x16_SSE 0
  movq mm2, [byte TMP1]
  movq mm3, [TMP1+8]
  paddusw mm5, mm0
  paddusw mm6, mm1
  movq mm0, [_EAX]
  movq mm1, [_EAX+8]
  pavgb mm2, [_EBX]
  pavgb mm3, [_EBX+8]
  add TMP1, TMP0
  add _EAX, TMP0
  add _EBX, TMP0
  psadbw mm0, mm2
  psadbw mm1, mm3
%endmacro

%macro SADBI_8x8_3dne 0
  movq mm2, [TMP1]
  movq mm3, [TMP1+TMP0]
  pavgb mm2, [_EAX]
  pavgb mm3, [_EAX+TMP0]
  lea TMP1, [TMP1+2*TMP0]
  lea _EAX, [_EAX+2*TMP0]
  paddusw mm5, mm0
  paddusw mm6, mm1
  movq mm0, [_EBX]
  movq mm1, [_EBX+TMP0]
  lea _EBX, [_EBX+2*TMP0]
  psadbw mm0, mm2
  psadbw mm1, mm3
%endmacro

%macro ABS_16x16_SSE 1
%if (%1 == 0)
  movq mm7, [_EAX]
  psadbw mm7, mm4
  mov esi, esi
  movq mm6, [_EAX+8]
  movq mm5, [_EAX+TMP0]
  movq mm3, [_EAX+TMP0+8]
  psadbw mm6, mm4

  movq mm2, [byte _EAX+2*TMP0]
  psadbw mm5, mm4
  movq mm1, [_EAX+2*TMP0+8]
  psadbw mm3, mm4

  movq mm0, [_EAX+TMP1+0]
  psadbw mm2, mm4
  add _EAX, TMP1
  psadbw mm1, mm4
%endif
%if (%1 == 1)
  psadbw mm0, mm4
  paddd mm7, mm0
  movq mm0, [_EAX+8]
  psadbw mm0, mm4
  paddd mm6, mm0

  movq mm0, [byte _EAX+TMP0]
  psadbw mm0, mm4

  paddd mm5, mm0
  movq mm0, [_EAX+TMP0+8]

  psadbw mm0, mm4
  paddd mm3, mm0
  movq mm0, [_EAX+2*TMP0]
  psadbw mm0, mm4
  paddd mm2, mm0

  movq mm0, [_EAX+2*TMP0+8]
  add _EAX, TMP1
  psadbw mm0, mm4
  paddd mm1, mm0
  movq mm0, [_EAX]
%endif
%if (%1 == 2)
  psadbw mm0, mm4
  paddd mm7, mm0
  movq mm0, [_EAX+8]
  psadbw mm0, mm4
  paddd mm6, mm0
%endif
%endmacro

;=============================================================================
; Code
;=============================================================================

TEXT

cglobal sad16_3dne
cglobal sad8_3dne
cglobal sad16bi_3dne
cglobal sad8bi_3dne
cglobal dev16_3dne

;-----------------------------------------------------------------------------
;
; uint32_t sad16_3dne(const uint8_t * const cur,
;                     const uint8_t * const ref,
;                     const uint32_t stride,
;                     const uint32_t best_sad);
;
;-----------------------------------------------------------------------------

; optimization: 21% faster

ALIGN SECTION_ALIGN
sad16_3dne:
  mov _EAX, prm1 ; Src1
  mov TMP1, prm2 ; Src2
  mov TMP0, prm3 ; Stride

  push _EBX
  lea _EBX, [2*TMP0+TMP0]

  SAD_16x16_SSE 0
  SAD_16x16_SSE 1
  SAD_16x16_SSE 2
  SAD_16x16_SSE 3

  paddd mm1, mm5
  movd eax, mm1
  add eax, dword [_ESP]
  add eax, dword [_ESP+4]
  add eax, dword [_ESP+8]
  mov _EBX, [_ESP+12]
  add _ESP, byte PTR_SIZE+12

  ret
ENDFUNC


;-----------------------------------------------------------------------------
;
; uint32_t sad8_3dne(const uint8_t * const cur,
;					const uint8_t * const ref,
;					const uint32_t stride);
;
;-----------------------------------------------------------------------------

ALIGN SECTION_ALIGN
sad8_3dne:

  mov _EAX, prm1 ; Src1
  mov TMP0, prm3 ; Stride
  mov TMP1, prm2 ; Src2
  push _EBX
  lea _EBX, [TMP0+2*TMP0]

  movq mm0, [byte _EAX]      ;0
  psadbw mm0, [byte TMP1]
  movq mm1, [_EAX+TMP0]       ;1
  psadbw mm1, [TMP1+TMP0]

  movq mm2, [_EAX+2*TMP0]     ;2
  psadbw mm2, [TMP1+2*TMP0]
  movq mm3, [_EAX+_EBX]       ;3
  psadbw mm3, [TMP1+_EBX]

  paddd mm0, mm1

  movq mm4, [byte _EAX+4*TMP0];4
  psadbw mm4, [TMP1+4*TMP0]
  movq mm5, [_EAX+2*_EBX]     ;6
  psadbw mm5, [TMP1+2*_EBX]

  paddd mm2, mm3
  paddd mm0, mm2

  lea _EBX, [_EBX+4*TMP0]      ;3+4=7
  lea TMP0, [TMP0+4*TMP0]      ; 5
  movq mm6, [_EAX+TMP0]       ;5
  psadbw mm6, [TMP1+TMP0]
  movq mm7, [_EAX+_EBX]       ;7
  psadbw mm7, [TMP1+_EBX]
  paddd mm4, mm5
  paddd mm6, mm7
  paddd mm0, mm4
  mov _EBX, [_ESP]
  add _ESP, byte PTR_SIZE 
  paddd mm0, mm6
  movd eax, mm0

 ret
ENDFUNC


;-----------------------------------------------------------------------------
;
; uint32_t sad16bi_3dne(const uint8_t * const cur,
;					const uint8_t * const ref1,
;					const uint8_t * const ref2,
;					const uint32_t stride);
;
;-----------------------------------------------------------------------------
;optimization: 14% faster

ALIGN SECTION_ALIGN
sad16bi_3dne:
  mov _EAX, prm1 ; Src
  mov TMP1, prm2 ; Ref1
  mov TMP0, prm4 ; Stride

  push _EBX
%ifdef ARCH_IS_X86_64
  mov _EBX, prm3
%else
  mov _EBX, [_ESP+4+12] ; Ref2
%endif

  SADBI_16x16_SSE0
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
  paddusw mm5,mm0
  paddusw mm6,mm1

  pop _EBX
  paddusw mm6,mm5
  movd eax, mm6

  ret
ENDFUNC

;-----------------------------------------------------------------------------
;
; uint32_t sad8bi_3dne(const uint8_t * const cur,
; const uint8_t * const ref1,
; const uint8_t * const ref2,
; const uint32_t stride);
;
;-----------------------------------------------------------------------------

ALIGN SECTION_ALIGN
sad8bi_3dne:
  mov _EAX, prm3 ; Ref2
  mov TMP1, prm2 ; Ref1
  mov TMP0, prm4 ; Stride

  push _EBX
%ifdef ARCH_IS_X86_64
  mov _EBX, prm1
%else
  mov _EBX, [_ESP+4+ 4] ; Src
%endif

  movq mm2, [TMP1]
  movq mm3, [TMP1+TMP0]
  pavgb mm2, [_EAX]
  pavgb mm3, [_EAX+TMP0]
  lea TMP1, [TMP1+2*TMP0]
  lea _EAX, [_EAX+2*TMP0]
  movq mm5, [_EBX]
  movq mm6, [_EBX+TMP0]
  lea _EBX, [_EBX+2*TMP0]
  psadbw mm5, mm2
  psadbw mm6, mm3

  movq mm2, [TMP1]
  movq mm3, [TMP1+TMP0]
  pavgb mm2, [_EAX]
  pavgb mm3, [_EAX+TMP0]
  lea TMP1, [TMP1+2*TMP0]
  lea _EAX, [_EAX+2*TMP0]
  movq mm0, [_EBX]
  movq mm1, [_EBX+TMP0]
  lea _EBX, [_EBX+2*TMP0]
  psadbw mm0, mm2
  psadbw mm1, mm3

  movq mm2, [TMP1]
  movq mm3, [TMP1+TMP0]
  pavgb mm2, [_EAX]
  pavgb mm3, [_EAX+TMP0]
  lea TMP1, [TMP1+2*TMP0]
  lea _EAX, [_EAX+2*TMP0]
  paddusw mm5,mm0
  paddusw mm6,mm1
  movq mm0, [_EBX]
  movq mm1, [_EBX+TMP0]
  lea _EBX, [_EBX+2*TMP0]
  psadbw mm0, mm2
  psadbw mm1, mm3

  movq mm2, [TMP1]
  movq mm3, [TMP1+TMP0]
  pavgb mm2, [_EAX]
  pavgb mm3, [_EAX+TMP0]
  paddusw mm5,mm0
  paddusw mm6,mm1
  movq mm0, [_EBX]
  movq mm1, [_EBX+TMP0]
  psadbw mm0, mm2
  psadbw mm1, mm3
  paddusw mm5,mm0
  paddusw mm6,mm1

  paddusw mm6,mm5
  mov _EBX,[_ESP]
  add _ESP,byte PTR_SIZE
  movd eax, mm6

 ret
ENDFUNC


;===========================================================================
;
; uint32_t dev16_3dne(const uint8_t * const cur,
;					const uint32_t stride);
;
;===========================================================================
; optimization: 25 % faster

ALIGN SECTION_ALIGN
dev16_3dne:

  mov _EAX, prm1 ; Src
  mov TMP0, prm2 ; Stride
  lea TMP1, [TMP0+2*TMP0]

  pxor mm4, mm4

ALIGN SECTION_ALIGN
  ABS_16x16_SSE 0
  ABS_16x16_SSE 1
  ABS_16x16_SSE 1
  ABS_16x16_SSE 1
  ABS_16x16_SSE 1

  paddd mm1, mm2
  paddd mm3, mm5

  ABS_16x16_SSE 2

  paddd mm7, mm6
  paddd mm1, mm3
  mov _EAX, prm1         ; Src
  paddd mm7, mm1
  punpcklbw mm7, mm7        ;xxyyaazz
  pshufw mm4, mm7, 055h     ; mm4 contains the mean


  pxor mm1, mm1

  ABS_16x16_SSE 0
  ABS_16x16_SSE 1
  ABS_16x16_SSE 1
  ABS_16x16_SSE 1
  ABS_16x16_SSE 1

  paddd mm1, mm2
  paddd mm3, mm5

  ABS_16x16_SSE 2

  paddd mm7, mm6
  paddd mm1, mm3
  paddd mm7, mm1
  movd eax, mm7

  ret
ENDFUNC

NON_EXEC_STACK
