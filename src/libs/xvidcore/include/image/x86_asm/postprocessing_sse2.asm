;/*****************************************************************************
; *
; *  XVID MPEG-4 VIDEO CODEC
; *  - sse2 post processing -
; *
; *  Copyright(C) 2004 Peter Ross <pross@xvid.org>
; *               2004 Dcoder <dcoder@alexandria.cc>
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
; *************************************************************************/

%include "nasm.inc"

;===========================================================================
; read only data
;===========================================================================

DATA

xmm_0x80:
	times 16 db 0x80

;=============================================================================
; Code
;=============================================================================

TEXT

cglobal image_brightness_sse2

;//////////////////////////////////////////////////////////////////////
;// image_brightness_sse2
;//////////////////////////////////////////////////////////////////////

%macro CREATE_OFFSET_VECTOR 2
  mov [%1 +  0], %2
  mov [%1 +  1], %2
  mov [%1 +  2], %2
  mov [%1 +  3], %2
  mov [%1 +  4], %2
  mov [%1 +  5], %2
  mov [%1 +  6], %2
  mov [%1 +  7], %2
  mov [%1 +  8], %2
  mov [%1 +  9], %2
  mov [%1 + 10], %2
  mov [%1 + 11], %2
  mov [%1 + 12], %2
  mov [%1 + 13], %2
  mov [%1 + 14], %2
  mov [%1 + 15], %2
%endmacro

ALIGN SECTION_ALIGN
image_brightness_sse2:
  PUSH_XMM6_XMM7
%ifdef ARCH_IS_X86_64
  XVID_MOVSXD _EAX, prm5d
%else
  mov eax, prm5   ; brightness offset value	
%endif
  mov TMP1, prm1  ; Dst
  mov TMP0, prm2  ; stride

  push _ESI
  push _EDI    ; 8 bytes offset for push
  sub _ESP, 32 ; 32 bytes for local data (16bytes will be used, 16bytes more to align correctly mod 16)

  movdqa xmm6, [xmm_0x80]

  ; Create a offset...offset vector
  mov _ESI, _ESP          ; TMP1 will be esp aligned mod 16
  add _ESI, 15            ; TMP1 = esp + 15
  and _ESI, ~15           ; TMP1 = (esp + 15)&(~15)
  CREATE_OFFSET_VECTOR _ESI, al
  movdqa xmm7, [_ESI]

%ifdef ARCH_IS_X86_64
  mov _ESI, prm3
  mov _EDI, prm4
%else
  mov _ESI, [_ESP+8+32+12] ; width
  mov _EDI, [_ESP+8+32+16] ; height
%endif

.yloop:
  xor _EAX, _EAX

.xloop:
  movdqa xmm0, [TMP1 + _EAX]
  movdqa xmm1, [TMP1 + _EAX + 16] ; xmm0 = [dst]

  paddb xmm0, xmm6              ; unsigned -> signed domain
  paddb xmm1, xmm6
  paddsb xmm0, xmm7
  paddsb xmm1, xmm7             ; xmm0 += offset
  psubb xmm0, xmm6
  psubb xmm1, xmm6              ; signed -> unsigned domain

  movdqa [TMP1 + _EAX], xmm0
  movdqa [TMP1 + _EAX + 16], xmm1 ; [dst] = xmm0

  add _EAX,32
  cmp _EAX,_ESI
  jl .xloop

  add TMP1, TMP0                  ; dst += stride
  sub _EDI, 1
  jg .yloop

  add _ESP, 32
  pop _EDI
  pop _ESI

  POP_XMM6_XMM7
  ret
ENDFUNC
;//////////////////////////////////////////////////////////////////////

NON_EXEC_STACK
