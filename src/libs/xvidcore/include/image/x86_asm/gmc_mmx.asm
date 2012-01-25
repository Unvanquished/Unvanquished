;/*****************************************************************************
; *
; *  XVID MPEG-4 VIDEO CODEC
; *  - GMC core functions -
; *  Copyright(C) 2006 Pascal Massimino <skal@planet-d.net>
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
; * $Id: gmc_mmx.asm,v 1.7.2.4 2009/09/16 17:11:39 Isibaar Exp $
; *
; *************************************************************************/

;/**************************************************************************
; *
; *	History:
; *
; * Jun 14 2006:  initial version (during Germany/Poland match;)
; *
; *************************************************************************/

%include "nasm.inc"

;//////////////////////////////////////////////////////////////////////

cglobal xvid_GMC_Core_Lin_8_mmx
cglobal xvid_GMC_Core_Lin_8_sse2
cglobal xvid_GMC_Core_Lin_8_sse41

;//////////////////////////////////////////////////////////////////////

DATA

align SECTION_ALIGN
Cst16:
times 8 dw 16

TEXT

;//////////////////////////////////////////////////////////////////////
;// mmx version

%macro GMC_4_SSE 2  ; %1: i   %2: out reg (mm5 or mm6)

  pcmpeqw   mm0, mm0
  movq      mm1, [_EAX+2*(%1)     ]  ; u0 | u1 | u2 | u3
  psrlw     mm0, 12                 ; mask 0x000f
  movq      mm2, [_EAX+2*(%1)+2*16]  ; v0 | v1 | v2 | v3

  pand      mm1, mm0  ; u0
  pand      mm2, mm0  ; v0

  movq      mm0, [Cst16]
  movq      mm3, mm1    ; u     | ...
  movq      mm4, mm0
  pmullw    mm3, mm2    ; u.v
  psubw     mm0, mm1    ; 16-u
  psubw     mm4, mm2    ; 16-v
  pmullw    mm2, mm0    ; (16-u).v
  pmullw    mm0, mm4    ; (16-u).(16-v)
  pmullw    mm1, mm4    ;     u .(16-v)

  movd      mm4, [TMP0+TMP1  +%1]  ; src2
  movd       %2, [TMP0+TMP1+1+%1]  ; src3
  punpcklbw mm4, mm7
  punpcklbw  %2, mm7
  pmullw    mm2, mm4
  pmullw    mm3,  %2

  movd      mm4, [TMP0       +%1]  ; src0
  movd       %2, [TMP0     +1+%1]  ; src1
  punpcklbw mm4, mm7
  punpcklbw  %2, mm7
  pmullw    mm4, mm0
  pmullw     %2, mm1

  paddw     mm2, mm3
  paddw      %2, mm4

  paddw      %2, mm2
%endmacro

align SECTION_ALIGN
xvid_GMC_Core_Lin_8_mmx:
  mov  _EAX, prm2  ; Offsets
  mov  TMP0, prm3  ; Src0
  mov  TMP1, prm4  ; BpS

  pxor      mm7, mm7

  GMC_4_SSE 0, mm5
  GMC_4_SSE 4, mm6

;  pshufw   mm4, prm5d, 01010101b  ; Rounder (bits [16..31])
  movd      mm4, prm5d   ; Rounder (bits [16..31])
  mov       _EAX, prm1  ; Dst
  punpcklwd mm4, mm4
  punpckhdq mm4, mm4

  paddw    mm5, mm4
  paddw    mm6, mm4
  psrlw    mm5, 8
  psrlw    mm6, 8
  packuswb mm5, mm6
  movq [_EAX], mm5

  ret
ENDFUNC

;//////////////////////////////////////////////////////////////////////
;// SSE2 version

%macro GMC_8_SSE2 1
  
  pcmpeqw   xmm0, xmm0
  movdqa    xmm1, [_EAX     ]  ; u...
  psrlw     xmm0, 12          ; mask = 0x000f
  movdqa    xmm2, [_EAX+2*16]  ; v...
  pand      xmm1, xmm0
  pand      xmm2, xmm0

  movdqa    xmm0, [Cst16]
  movdqa    xmm3, xmm1    ; u     | ...
  movdqa    xmm4, xmm0
  pmullw    xmm3, xmm2    ; u.v
  psubw     xmm0, xmm1    ; 16-u
  psubw     xmm4, xmm2    ; 16-v
  pmullw    xmm2, xmm0    ; (16-u).v
  pmullw    xmm0, xmm4    ; (16-u).(16-v)
  pmullw    xmm1, xmm4    ;     u .(16-v)

%if (%1!=0) ; SSE41
  pmovzxbw  xmm4, [TMP0+TMP1  ]  ; src2
  pmovzxbw  xmm5, [TMP0+TMP1+1]  ; src3
%else
  movq      xmm4, [TMP0+TMP1  ]  ; src2
  movq      xmm5, [TMP0+TMP1+1]  ; src3
  punpcklbw xmm4, xmm7
  punpcklbw xmm5, xmm7
%endif
  pmullw    xmm2, xmm4
  pmullw    xmm3, xmm5

%if (%1!=0) ; SSE41
  pmovzxbw  xmm4, [TMP0     ]  ; src0
  pmovzxbw  xmm5, [TMP0   +1]  ; src1
%else
  movq      xmm4, [TMP0     ]  ; src0
  movq      xmm5, [TMP0   +1]  ; src1
  punpcklbw xmm4, xmm7
  punpcklbw xmm5, xmm7
%endif
  pmullw    xmm4, xmm0
  pmullw    xmm5, xmm1

  paddw     xmm2, xmm3
  paddw     xmm5, xmm4

  paddw     xmm5, xmm2
%endmacro

align SECTION_ALIGN
xvid_GMC_Core_Lin_8_sse2:
  PUSH_XMM6_XMM7
  
  mov  _EAX, prm2  ; Offsets
  mov  TMP0, prm3  ; Src0
  mov  TMP1, prm4  ; BpS

  pxor     xmm7, xmm7

  GMC_8_SSE2 0

  movd      xmm4, prm5d
  pshuflw   xmm4, xmm4, 01010101b  ; Rounder (bits [16..31])
  punpckldq xmm4, xmm4
  mov  _EAX, prm1  ; Dst

  paddw    xmm5, xmm4
  psrlw    xmm5, 8
  packuswb xmm5, xmm5
  movq [_EAX], xmm5

  POP_XMM6_XMM7
  ret
ENDFUNC

align SECTION_ALIGN
xvid_GMC_Core_Lin_8_sse41:
  mov  _EAX, prm2  ; Offsets
  mov  TMP0, prm3  ; Src0
  mov  TMP1, prm4  ; BpS

  GMC_8_SSE2 1

  movd      xmm4, prm5d
  pshuflw   xmm4, xmm4, 01010101b  ; Rounder (bits [16..31])
  punpckldq xmm4, xmm4
  mov  _EAX, prm1  ; Dst

  paddw    xmm5, xmm4
  psrlw    xmm5, 8
  packuswb xmm5, xmm5
  movq [_EAX], xmm5

  ret
ENDFUNC

;//////////////////////////////////////////////////////////////////////
NON_EXEC_STACK
