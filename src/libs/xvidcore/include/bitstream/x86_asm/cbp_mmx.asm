;/****************************************************************************
; *
; *  XVID MPEG-4 VIDEO CODEC
; *  - MMX CBP computation -
; *
; *  Copyright (C) 2005      Carlo Bramini <carlo.bramix@libero.it>
; *                2001-2003 Peter Ross <pross@xvid.org>
; *                2002-2003 Pascal Massimino <skal@planet-d.net>
; *
; *  This program is free software ; you can redistribute it and/or modify
; *  it under the terms of the GNU General Public License as published by
; *  the Free Software Foundation ; either version 2 of the License, or
; *  (at your option) any later version.
; *
; *  This program is distributed in the hope that it will be useful,
; *  but WITHOUT ANY WARRANTY ; without even the implied warranty of
; *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; *  GNU General Public License for more details.
; *
; *  You should have received a copy of the GNU General Public License
; *  along with this program ; if not, write to the Free Software
; *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
; *
; * $Id: cbp_mmx.asm,v 1.17.2.2 2009/09/16 17:11:39 Isibaar Exp $
; *
; ***************************************************************************/

;=============================================================================
; Macros
;=============================================================================

%include "nasm.inc"

;=============================================================================
; Local data
;=============================================================================

DATA

ALIGN SECTION_ALIGN

mult_mask:
  db 0x10,0x20,0x04,0x08,0x01,0x02,0x00,0x00
ignore_dc:
  dw 0, -1, -1, -1

;=============================================================================
; Code
;=============================================================================

TEXT

cglobal calc_cbp_mmx

;-----------------------------------------------------------------------------
; uint32_t calc_cbp_mmx(const int16_t coeff[6][64]);
;-----------------------------------------------------------------------------

%macro      MAKE_LOAD         2
  por mm0, [%2-128*1+%1*8]
  por mm1, [%2+128*0+%1*8]
  por mm2, [%2+128*1+%1*8]
  por mm3, [%2+128*2+%1*8]
  por mm4, [%2+128*3+%1*8]
  por mm5, [%2+128*4+%1*8]
%endmacro

ALIGN SECTION_ALIGN
calc_cbp_mmx:
  mov _EAX, prm1            ; coeff

  movq mm7, [ignore_dc]
  pxor mm6, mm6                ; used only for comparing
  movq mm0, [_EAX+128*0]
  movq mm1, [_EAX+128*1]
  movq mm2, [_EAX+128*2]
  movq mm3, [_EAX+128*3]
  movq mm4, [_EAX+128*4]
  movq mm5, [_EAX+128*5]
  add _EAX, 8+128
  pand mm0, mm7
  pand mm1, mm7
  pand mm2, mm7
  pand mm3, mm7
  pand mm4, mm7
  pand mm5, mm7

  MAKE_LOAD 0, _EAX
  MAKE_LOAD 1, _EAX
  MAKE_LOAD 2, _EAX
  MAKE_LOAD 3, _EAX
  MAKE_LOAD 4, _EAX
  MAKE_LOAD 5, _EAX
  MAKE_LOAD 6, _EAX
  MAKE_LOAD 7, _EAX
  MAKE_LOAD 8, _EAX
  MAKE_LOAD 9, _EAX
  MAKE_LOAD 10, _EAX
  MAKE_LOAD 11, _EAX
  MAKE_LOAD 12, _EAX
  MAKE_LOAD 13, _EAX
  MAKE_LOAD 14, _EAX

  movq mm7, [mult_mask]
  packssdw mm0, mm1
  packssdw mm2, mm3
  packssdw mm4, mm5
  packssdw mm0, mm2
  packssdw mm4, mm6
  pcmpeqw mm0, mm6
  pcmpeqw mm4, mm6
  pcmpeqw mm0, mm6
  pcmpeqw mm4, mm6
  psrlw mm0, 15
  psrlw mm4, 15
  packuswb mm0, mm4
  pmaddwd mm0, mm7

  movq mm1, mm0
  psrlq mm1, 32
  paddusb mm0, mm1

  movd eax, mm0
  shr _EAX, 8
  and _EAX, 0x3F
  ret
ENDFUNC

NON_EXEC_STACK
