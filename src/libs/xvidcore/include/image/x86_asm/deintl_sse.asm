;/*****************************************************************************
; *
; *  XVID MPEG-4 VIDEO CODEC
; *  - simple de-interlacer
; *  Copyright(C) 2006 Pascal Massimino <skal@xvid.org>
; *
; *  This file is part of XviD, a free MPEG-4 video encoder/decoder
; *
; *  XviD is free software; you can redistribute it and/or modify it
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
; * $Id: deintl_sse.asm,v 1.4.2.2 2009/09/16 17:11:39 Isibaar Exp $
; *
; *************************************************************************/

;/**************************************************************************
; *
; *	History:
; *
; * Oct 13 2006:  initial version
; *
; *************************************************************************/

%include "nasm.inc"

;//////////////////////////////////////////////////////////////////////

cglobal xvid_deinterlace_sse

;//////////////////////////////////////////////////////////////////////

DATA

align SECTION_ALIGN
Mask_6b  times 16 db 0x3f
Rnd_3b:  times 16 db 3

TEXT

;//////////////////////////////////////////////////////////////////////
;// sse version

align SECTION_ALIGN
xvid_deinterlace_sse:

  mov _EAX, prm1  ; Pix
  mov TMP0, prm3  ; Height
  mov TMP1, prm4  ; BpS

  push _EBX
%ifdef ARCH_IS_X86_64
  mov _EBX,  prm2  ; Width
%else
  mov _EBX, [esp+4+ 8] ; Width
%endif

  add _EBX, 7
  shr TMP0, 1
  shr _EBX, 3        ; Width /= 8
  dec TMP0

  movq mm6, [Mask_6b]

.Loop_x:
  push _EAX
  movq mm1,  [_EAX      ]
  movq mm2,  [_EAX+ TMP1]
  lea  _EAX, [_EAX+ TMP1]
  movq mm0, mm2

  push TMP0

.Loop:
  movq    mm3, [_EAX+  TMP1]
  movq    mm4, [_EAX+2*TMP1]
  movq    mm5, mm2
  pavgb   mm0, mm4
  pavgb   mm1, mm3
  movq    mm7, mm2
  psubusb mm2, mm0
  psubusb mm0, mm7
  paddusb mm0, [Rnd_3b]
  psrlw   mm2, 2
  psrlw   mm0, 2
  pand    mm2, mm6
  pand    mm0, mm6
  paddusb mm1, mm2
  psubusb mm1, mm0
  movq   [_EAX], mm1
  lea  _EAX, [_EAX+2*TMP1]
  movq mm0, mm5
  movq mm1, mm3
  movq mm2, mm4
  dec TMP0
  jg .Loop

  pavgb mm0, mm2     ; p0 += p2
  pavgb mm1, mm1     ; p1 += p1
  movq    mm7, mm2
  psubusb mm2, mm0
  psubusb mm0, mm7
  paddusb mm0, [Rnd_3b]
  psrlw   mm2, 2
  psrlw   mm0, 2
  pand    mm2, mm6
  pand    mm0, mm6
  paddusb mm1, mm2
  psubusb mm1, mm0
  movq   [_EAX], mm1

  pop TMP0
  pop _EAX
  add _EAX, 8

  dec _EBX
  jg .Loop_x

  pop _EBX
  ret
ENDFUNC

;//////////////////////////////////////////////////////////////////////
NON_EXEC_STACK
