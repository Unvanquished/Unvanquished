;/****************************************************************************
; *
; *  XVID MPEG-4 VIDEO CODEC
; *  - 3DNow sad operators w/o XMM instructions -
; *
; *  Copyright(C) 2002 Peter ross <pross@xvid.org>
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
; * $Id: sad_3dn.asm,v 1.12.2.2 2009/09/16 17:11:39 Isibaar Exp $
; *
; ***************************************************************************/

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
%macro SADBI_16x16_3DN 0
  movq mm0, [_EAX] ; src
  movq mm2, [_EAX+8]

  movq mm1, [TMP1] ; ref1
  movq mm3, [TMP1+8]
  pavgusb mm1, [_EBX] ; ref2
  lea TMP1, [TMP1+TMP0]
  pavgusb mm3, [_EBX+8]
  lea _EBX, [_EBX+TMP0]

  movq mm4, mm0
  lea _EAX, [_EAX+TMP0]
  psubusb mm0, mm1
  movq mm5, mm2
  psubusb mm2, mm3

  psubusb mm1, mm4
  por mm0, mm1
  psubusb mm3, mm5
  por mm2, mm3

  movq mm1, mm0
  movq mm3, mm2

  punpcklbw mm0,mm7
  punpckhbw mm1,mm7
  punpcklbw mm2,mm7
  punpckhbw mm3,mm7

  paddusw mm0,mm1
  paddusw mm2,mm3
  paddusw mm6,mm0
  paddusw mm6,mm2
%endmacro

%macro SADBI_8x8_3DN 0
  movq mm0, [_EAX] ; src
  movq mm2, [_EAX+TMP0]

  movq mm1, [TMP1] ; ref1
  movq mm3, [TMP1+TMP0]
  pavgusb mm1, [_EBX] ; ref2
  lea TMP1, [TMP1+2*TMP0]
  pavgusb mm3, [_EBX+TMP0]
  lea _EBX, [_EBX+2*TMP0]

  movq mm4, mm0
  lea _EAX, [_EAX+2*TMP0]
  psubusb mm0, mm1
  movq mm5, mm2
  psubusb mm2, mm3

  psubusb mm1, mm4
  por mm0, mm1
  psubusb mm3, mm5
  por mm2, mm3

  movq mm1, mm0
  movq mm3, mm2

  punpcklbw mm0,mm7
  punpckhbw mm1,mm7
  punpcklbw mm2,mm7
  punpckhbw mm3,mm7

  paddusw mm0,mm1
  paddusw mm2,mm3
  paddusw mm6,mm0
  paddusw mm6,mm2
%endmacro

;=============================================================================
; Code
;=============================================================================

TEXT

cglobal  sad16bi_3dn
cglobal  sad8bi_3dn

;-----------------------------------------------------------------------------
;
; uint32_t sad16bi_3dn(const uint8_t * const cur,
; const uint8_t * const ref1,
; const uint8_t * const ref2,
; const uint32_t stride);
;
;-----------------------------------------------------------------------------

ALIGN SECTION_ALIGN
sad16bi_3dn:
  mov _EAX, prm1 ; Src
  mov TMP1, prm2 ; Ref1
  mov TMP0, prm4 ; Stride

  push _EBX
%ifdef ARCH_IS_X86_64
  mov _EBX, prm3
%else
  mov _EBX, [_ESP+4+12] ; Ref2
%endif

  pxor mm6, mm6 ; accum2
  pxor mm7, mm7
.Loop:
  SADBI_16x16_3DN
  SADBI_16x16_3DN
  SADBI_16x16_3DN
  SADBI_16x16_3DN
  SADBI_16x16_3DN
  SADBI_16x16_3DN
  SADBI_16x16_3DN
  SADBI_16x16_3DN

  SADBI_16x16_3DN
  SADBI_16x16_3DN
  SADBI_16x16_3DN
  SADBI_16x16_3DN
  SADBI_16x16_3DN
  SADBI_16x16_3DN
  SADBI_16x16_3DN
  SADBI_16x16_3DN

  pmaddwd mm6, [mmx_one] ; collapse
  movq mm7, mm6
  psrlq mm7, 32
  paddd mm6, mm7

  movd eax, mm6

  pop _EBX

  ret
ENDFUNC

;-----------------------------------------------------------------------------
;
; uint32_t sad8bi_3dn(const uint8_t * const cur,
; const uint8_t * const ref1,
; const uint8_t * const ref2,
; const uint32_t stride);
;
;-----------------------------------------------------------------------------

ALIGN SECTION_ALIGN
sad8bi_3dn:
  mov _EAX, prm1 ; Src
  mov TMP1, prm2 ; Ref1
  mov TMP0, prm4 ; Stride

  push _EBX
%ifdef ARCH_IS_X86_64
  mov _EBX, prm3
%else
  mov _EBX, [_ESP+4+12] ; Ref2
%endif

  pxor mm6, mm6 ; accum2
  pxor mm7, mm7
.Loop:
  SADBI_8x8_3DN
  SADBI_8x8_3DN
  SADBI_8x8_3DN
  SADBI_8x8_3DN

  pmaddwd mm6, [mmx_one] ; collapse
  movq mm7, mm6
  psrlq mm7, 32
  paddd mm6, mm7

  movd eax, mm6

  pop _EBX

  ret
ENDFUNC

NON_EXEC_STACK
