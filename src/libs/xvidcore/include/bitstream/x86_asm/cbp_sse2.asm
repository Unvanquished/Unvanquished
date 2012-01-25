;/****************************************************************************
; *
; *  XVID MPEG-4 VIDEO CODEC
; *  - SSE2 CBP computation -
; *
; *  Copyright (C) 2002 Daniel Smith <danielsmith@astroboymail.com>
; *                2002 Pascal Massimino <skal@planet-d.net>
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
; * $Id: cbp_sse2.asm,v 1.10.2.3 2009/09/16 17:11:39 Isibaar Exp $
; *
; ***************************************************************************/

;=============================================================================
; Macros
;=============================================================================

%include "nasm.inc"

%macro LOOP_SSE2 2
  movdqa xmm0, [%2+(%1)*128]
  pand xmm0, xmm7
  movdqa xmm1, [%2+(%1)*128+16]

  por xmm0, [%2+(%1)*128+32]
  por xmm1, [%2+(%1)*128+48]
  por xmm0, [%2+(%1)*128+64]
  por xmm1, [%2+(%1)*128+80]
  por xmm0, [%2+(%1)*128+96]
  por xmm1, [%2+(%1)*128+112]

  por xmm0, xmm1        ; xmm0 = xmm1 = 128 bits worth of info
  psadbw xmm0, xmm6     ; contains 2 dwords with sums
  movhlps xmm1, xmm0    ; move high dword from xmm0 to low xmm1
  por xmm0, xmm1        ; combine
  movd ecx, xmm0        ; if ecx set, values were found
  test _ECX, _ECX
%endmacro

;=============================================================================
; Data (Read Only)
;=============================================================================

DATA

ALIGN SECTION_ALIGN
ignore_dc:
  dw 0, -1, -1, -1, -1, -1, -1, -1

;=============================================================================
; Code
;=============================================================================

TEXT

;-----------------------------------------------------------------------------
; uint32_t calc_cbp_sse2(const int16_t coeff[6*64]);
;-----------------------------------------------------------------------------

ALIGN SECTION_ALIGN
cglobal calc_cbp_sse2
calc_cbp_sse2:
  mov _EDX, prm1           ; coeff[]
  xor _EAX, _EAX           ; cbp = 0

  PUSH_XMM6_XMM7
  
  movdqu xmm7, [ignore_dc] ; mask to ignore dc value
  pxor xmm6, xmm6          ; zero

  LOOP_SSE2 0, _EDX
  jz .blk2
  or _EAX, (1<<5)

.blk2:
  LOOP_SSE2 1, _EDX
  jz .blk3
  or _EAX, (1<<4)

.blk3:
  LOOP_SSE2 2, _EDX
  jz .blk4
  or _EAX, (1<<3)

.blk4:
  LOOP_SSE2 3, _EDX
  jz .blk5
  or _EAX, (1<<2)

.blk5:
  LOOP_SSE2 4, _EDX
  jz .blk6
  or _EAX, (1<<1)

.blk6:
  LOOP_SSE2 5, _EDX
  jz .finished
  or _EAX, (1<<0)

.finished:
 
  POP_XMM6_XMM7
  ret
ENDFUNC

NON_EXEC_STACK
