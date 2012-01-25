;/*****************************************************************************
; *
; *  XVID MPEG-4 VIDEO CODEC
; *  - mmx post processing -
; *
; *  Copyright(C) 2004 Peter Ross <pross@xvid.org>
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
; * $Id: postprocessing_mmx.asm,v 1.9.2.3 2009/09/16 17:11:39 Isibaar Exp $
; *
; *************************************************************************/

%include "nasm.inc"

;===========================================================================
; read only data
;===========================================================================

DATA

mmx_0x80:
	times 8 db 0x80

mmx_offset:
%assign i -128
%rep 256
	times 8 db i
	%assign i i+1
%endrep


;=============================================================================
; Code
;=============================================================================

TEXT

cglobal image_brightness_mmx


;//////////////////////////////////////////////////////////////////////
;// image_brightness_mmx
;//////////////////////////////////////////////////////////////////////

align SECTION_ALIGN
image_brightness_mmx:

	movq mm6, [mmx_0x80]

%ifdef ARCH_IS_X86_64
        XVID_MOVSXD _EAX, prm5d
        lea TMP0, [mmx_offset]
	movq mm7, [TMP0 + (_EAX + 128)*8]   ; being lazy
%else
        mov eax, prm5d ; offset
        movq mm7, [mmx_offset + (_EAX + 128)*8]   ; being lazy
%endif

	mov TMP1, prm1  ; Dst
	mov TMP0, prm2  ; stride

	push _ESI
	push _EDI
%ifdef ARCH_IS_X86_64
        mov _ESI, prm3
	mov _EDI, prm4
%else
	mov _ESI, [_ESP+8+12] ; width
	mov _EDI, [_ESP+8+16] ; height
%endif

.yloop:
	xor	_EAX, _EAX

.xloop:
	movq mm0, [TMP1 + _EAX]
	movq mm1, [TMP1 + _EAX + 8]	; mm0 = [dst]

	paddb mm0, mm6				; unsigned -> signed domain
	paddb mm1, mm6
	paddsb mm0, mm7
	paddsb mm1, mm7				; mm0 += offset
	psubb mm0, mm6
	psubb mm1, mm6				; signed -> unsigned domain

	movq [TMP1 + _EAX], mm0
	movq [TMP1 + _EAX + 8], mm1	; [dst] = mm0

	add	_EAX,16
	cmp	_EAX,_ESI	
	jl .xloop

	add TMP1, TMP0				; dst += stride
	sub _EDI, 1
	jg .yloop

	pop _EDI
	pop _ESI

	ret
ENDFUNC
;//////////////////////////////////////////////////////////////////////

NON_EXEC_STACK
