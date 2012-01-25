;/****************************************************************************
; *
; *  XVID MPEG-4 VIDEO CODEC
; *  - MMX and XMM YUYV<->YV12 conversion -
; *
; *  Copyright(C) 2002 Peter Ross <pross@xvid.org>
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
; * $Id: colorspace_yuyv_mmx.asm,v 1.10.2.2 2009/09/16 17:11:39 Isibaar Exp $
; *
; ***************************************************************************/

%include "nasm.inc"

;=============================================================================
; Read only data
;=============================================================================

DATA

;-----------------------------------------------------------------------------
; yuyv/uyvy mask for extracting yuv components
;-----------------------------------------------------------------------------
;				y     u     y     v     y     u     y     v

ALIGN SECTION_ALIGN
yuyv_mask:	db 0xff,  0,  0xff,   0,   0xff,  0,   0xff,  0
mmx_one:    dw 1, 1, 1, 1

;=============================================================================
; helper macros used with colorspace_mmx.inc
;=============================================================================

;-----------------------------------------------------------------------------
; YUYV_TO_YV12( TYPE, PAVG )
;
; TYPE	0=yuyv, 1=uyvy
; PAVG  0=mmx, pavgusb=3dnow, pavgb=xmm
;
; bytes=2, pixels = 8, vpixels=2
;-----------------------------------------------------------------------------

%macro YUYV_TO_YV12_INIT		2
  movq mm7, [yuyv_mask]
%endmacro


%macro YUYV_TO_YV12             2
  movq mm0, [x_ptr]               ; x_ptr[0]
  movq mm1, [x_ptr + 8]           ; x_ptr[8]
  movq mm2, [x_ptr + x_stride]         ; x_ptr[x_stride + 0]
  movq mm3, [x_ptr + x_stride + 8]     ; x_ptr[x_stride + 8]

    ; average uv-components
;---[ plain mmx ]----------------------------------------------------
%ifidn %2,0     ; if (%2 eq "0")
  movq mm4, mm0
  movq mm5, mm2
%if %1 == 0         ; yuyv
  psrlw mm4, 8
  psrlw mm5, 8
%endif
  pand mm4, mm7
  pand mm5, mm7
  paddw mm4, mm5

  movq mm5, mm1
  movq mm6, mm3
%if %1 == 0         ; yuyv
  psrlw mm5, 8
  psrlw mm6, 8
%endif
  pand mm5, mm7
  pand mm6, mm7
  paddw mm5, mm6
  paddw mm4, [mmx_one]      ; +1 rounding
  paddw mm5, [mmx_one]      ;
  psrlw mm4, 1
  psrlw mm5, 1
;---[ 3dnow/xmm ]----------------------------------------------------
%else
  movq mm4, mm0
  movq mm5, mm1
  %2 mm4, mm2           ;pavgb/pavgusb mm4, mm2
  %2 mm5, mm3           ;pavgb/pavgusb mm5, mm3

  ;;movq mm6, mm0       ; 0 rounding
  ;;pxor mm6, mm2       ;
  ;;psubb mm4, mm6      ;
  ;;movq mm6, mm1       ;
  ;;pxor mm6, mm3       ;
  ;;psubb mm5, mm5      ;

%if %1 == 0             ; yuyv
  psrlw mm4, 8
  psrlw mm5, 8
%endif
  pand mm4, mm7
  pand mm5, mm7
%endif
;--------------------------------------------------------------------

    ; write y-component
%if %1 == 1         ; uyvy
  psrlw mm0, 8
  psrlw mm1, 8
  psrlw mm2, 8
  psrlw mm3, 8
%endif
  pand mm0, mm7
  pand mm1, mm7
  pand mm2, mm7
  pand mm3, mm7
  packuswb mm0, mm1
  packuswb mm2, mm3

%ifidn %2,pavgb         ; xmm
  movntq [y_ptr], mm0
  movntq [y_ptr+y_stride], mm2
%else                   ; plain mmx,3dnow
  movq [y_ptr], mm0
  movq [y_ptr+y_stride], mm2
%endif

    ; write uv-components
  packuswb mm4, mm5
  movq mm5, mm4
  psrlq mm4, 8
  pand mm5, mm7
  pand mm4, mm7
  packuswb mm5,mm5
  packuswb mm4,mm4
  movd [u_ptr],mm5
  movd [v_ptr],mm4
%endmacro

;-----------------------------------------------------------------------------
; YV12_TO_YUYV( TYPE )
;
; bytes=2, pixels = 16, vpixels=2
;-----------------------------------------------------------------------------

%macro YV12_TO_YUYV_INIT        2
%endmacro


%macro YV12_TO_YUYV             2
  movq mm6, [u_ptr]               ; [    |uuuu]
  movq mm2, [v_ptr]               ; [    |vvvv]
  movq mm0, [y_ptr    ]           ; [yyyy|yyyy] ; y row 0
  movq mm1, [y_ptr+y_stride]           ; [yyyy|yyyy] ; y row 1
  movq      mm7, mm6
  punpcklbw mm6, mm2            ; [vuvu|vuvu] ; uv[0..3]
  punpckhbw mm7, mm2            ; [vuvu|vuvu] ; uv[4..7]

%if %1 == 0     ; YUYV
  movq mm2, mm0
  movq mm3, mm1
  movq mm4, [y_ptr    +8]         ; [yyyy|yyyy] ; y[8..15] row 0
  movq mm5, [y_ptr+y_stride+8]         ; [yyyy|yyyy] ; y[8..15] row 1
  punpcklbw mm0, mm6            ; [vyuy|vyuy] ; y row 0 + 0
  punpckhbw mm2, mm6            ; [vyuy|vyuy] ; y row 0 + 8
  punpcklbw mm1, mm6            ; [vyuy|vyuy] ; y row 1 + 0
  punpckhbw mm3, mm6            ; [vyuy|vyuy] ; y row 1 + 8
  movq [x_ptr      ], mm0
  movq [x_ptr+8    ], mm2
  movq [x_ptr+x_stride  ], mm1
  movq [x_ptr+x_stride+8], mm3
  movq mm0, mm4
  movq mm2, mm5
  punpcklbw mm0, mm7            ; [vyuy|vyuy] ; y row 0 + 16
  punpckhbw mm4, mm7            ; [vyuy|vyuy] ; y row 0 + 24
  punpcklbw mm2, mm7            ; [vyuy|vyuy] ; y row 1 + 16
  punpckhbw mm5, mm7            ; [vyuy|vyuy] ; y row 1 + 24
  movq [x_ptr    +16], mm0
  movq [x_ptr    +24], mm4
  movq [x_ptr+x_stride+16], mm2
  movq [x_ptr+x_stride+24], mm5
%else           ; UYVY
  movq mm2, mm6
  movq mm3, mm6
  movq mm4, mm6
  punpcklbw mm2, mm0            ; [yvyu|yvyu]   ; y row 0 + 0
  punpckhbw mm3, mm0            ; [yvyu|yvyu]   ; y row 0 + 8
  movq mm0, [y_ptr    +8]         ; [yyyy|yyyy] ; y[8..15] row 0
  movq mm5, [y_ptr+y_stride+8]         ; [yyyy|yyyy] ; y[8..15] row 1
  punpcklbw mm4, mm1            ; [yvyu|yvyu]   ; y row 1 + 0
  punpckhbw mm6, mm1            ; [yvyu|yvyu]   ; y row 1 + 8
  movq [x_ptr      ], mm2
  movq [x_ptr    +8], mm3
  movq [x_ptr+x_stride  ], mm4
  movq [x_ptr+x_stride+8], mm6
  movq mm2, mm7
  movq mm3, mm7
  movq mm6, mm7
  punpcklbw mm2, mm0            ; [yvyu|yvyu]   ; y row 0 + 0
  punpckhbw mm3, mm0            ; [yvyu|yvyu]   ; y row 0 + 8
  punpcklbw mm6, mm5            ; [yvyu|yvyu]   ; y row 1 + 0
  punpckhbw mm7, mm5            ; [yvyu|yvyu]   ; y row 1 + 8
  movq [x_ptr    +16], mm2
  movq [x_ptr    +24], mm3
  movq [x_ptr+x_stride+16], mm6
  movq [x_ptr+x_stride+24], mm7
%endif
%endmacro

;------------------------------------------------------------------------------
; YV12_TO_YUYVI( TYPE )
;
; TYPE  0=yuyv, 1=uyvy
;
; bytes=2, pixels = 8, vpixels=4
;------------------------------------------------------------------------------

%macro YV12_TO_YUYVI_INIT       2
%endmacro

%macro YV12_TO_YUYVI                2
%ifdef ARCH_IS_X86_64
  mov TMP1d, prm_uv_stride
  movd mm0, [u_ptr]               ; [    |uuuu]
  movd mm1, [u_ptr+TMP1]          ; [    |uuuu]
  punpcklbw mm0, [v_ptr]          ; [vuvu|vuvu] ; uv row 0
  punpcklbw mm1, [v_ptr+TMP1]     ; [vuvu|vuvu] ; uv row 1
%else
  xchg width, prm_uv_stride
  movd mm0, [u_ptr]               ; [    |uuuu]
  movd mm1, [u_ptr+width]         ; [    |uuuu]
  punpcklbw mm0, [v_ptr]          ; [vuvu|vuvu] ; uv row 0
  punpcklbw mm1, [v_ptr+width]    ; [vuvu|vuvu] ; uv row 1
  xchg width, prm_uv_stride
%endif

%if %1 == 0     ; YUYV
  movq mm4, [y_ptr]               ; [yyyy|yyyy] ; y row 0
  movq mm6, [y_ptr+y_stride]           ; [yyyy|yyyy] ; y row 1
  movq mm5, mm4
  movq mm7, mm6
  punpcklbw mm4, mm0            ; [yuyv|yuyv] ; y row 0 + 0
  punpckhbw mm5, mm0            ; [yuyv|yuyv] ; y row 0 + 8
  punpcklbw mm6, mm1            ; [yuyv|yuyv] ; y row 1 + 0
  punpckhbw mm7, mm1            ; [yuyv|yuyv] ; y row 1 + 8
  movq [x_ptr], mm4
  movq [x_ptr+8], mm5
  movq [x_ptr+x_stride], mm6
  movq [x_ptr+x_stride+8], mm7

  push y_ptr
  push x_ptr
  add y_ptr, y_stride
  add x_ptr, x_stride
  movq mm4, [y_ptr+y_stride]           ; [yyyy|yyyy] ; y row 2
  movq mm6, [y_ptr+2*y_stride]         ; [yyyy|yyyy] ; y row 3
  movq mm5, mm4
  movq mm7, mm6
  punpcklbw mm4, mm0            ; [yuyv|yuyv] ; y row 2 + 0
  punpckhbw mm5, mm0            ; [yuyv|yuyv] ; y row 2 + 8
  punpcklbw mm6, mm1            ; [yuyv|yuyv] ; y row 3 + 0
  punpckhbw mm7, mm1            ; [yuyv|yuyv] ; y row 3 + 8
  movq [x_ptr+x_stride], mm4
  movq [x_ptr+x_stride+8], mm5
  movq [x_ptr+2*x_stride], mm6
  movq [x_ptr+2*x_stride+8], mm7
  pop x_ptr
  pop y_ptr
%else           ; UYVY
  movq mm2, [y_ptr]               ; [yyyy|yyyy] ; y row 0
  movq mm3, [y_ptr+y_stride]           ; [yyyy|yyyy] ; y row 1
  movq mm4, mm0
  movq mm5, mm0
  movq mm6, mm1
  movq mm7, mm1
  punpcklbw mm4, mm2            ; [uyvy|uyvy] ; y row 0 + 0
  punpckhbw mm5, mm2            ; [uyvy|uyvy] ; y row 0 + 8
  punpcklbw mm6, mm3            ; [uyvy|uyvy] ; y row 1 + 0
  punpckhbw mm7, mm3            ; [uyvy|uyvy] ; y row 1 + 8
  movq [x_ptr], mm4
  movq [x_ptr+8], mm5
  movq [x_ptr+x_stride], mm6
  movq [x_ptr+x_stride+8], mm7

  push y_ptr
  push x_ptr
  add y_ptr, y_stride
  add x_ptr, x_stride
  movq mm2, [y_ptr+y_stride]           ; [yyyy|yyyy] ; y row 2
  movq mm3, [y_ptr+2*y_stride]         ; [yyyy|yyyy] ; y row 3
  movq mm4, mm0
  movq mm5, mm0
  movq mm6, mm1
  movq mm7, mm1
  punpcklbw mm4, mm2            ; [uyvy|uyvy] ; y row 2 + 0
  punpckhbw mm5, mm2            ; [uyvy|uyvy] ; y row 2 + 8
  punpcklbw mm6, mm3            ; [uyvy|uyvy] ; y row 3 + 0
  punpckhbw mm7, mm3            ; [uyvy|uyvy] ; y row 3 + 8
  movq [x_ptr+x_stride], mm4
  movq [x_ptr+x_stride+8], mm5
  movq [x_ptr+2*x_stride], mm6
  movq [x_ptr+2*x_stride+8], mm7
  pop x_ptr
  pop y_ptr
%endif
%endmacro

;=============================================================================
; Code
;=============================================================================

TEXT

%include "colorspace_mmx.inc"

; input

MAKE_COLORSPACE	 yuyv_to_yv12_mmx,0,    2,8,2,  YUYV_TO_YV12, 0, 0
MAKE_COLORSPACE	 yuyv_to_yv12_3dn,0,    2,8,2,  YUYV_TO_YV12, 0, pavgusb
MAKE_COLORSPACE	 yuyv_to_yv12_xmm,0,    2,8,2,  YUYV_TO_YV12, 0, pavgb

MAKE_COLORSPACE  uyvy_to_yv12_mmx,0,    2,8,2,  YUYV_TO_YV12, 1, 0
MAKE_COLORSPACE  uyvy_to_yv12_3dn,0,    2,8,2,  YUYV_TO_YV12, 1, pavgusb
MAKE_COLORSPACE  uyvy_to_yv12_xmm,0,    2,8,2,  YUYV_TO_YV12, 1, pavgb

; output

MAKE_COLORSPACE  yv12_to_yuyv_mmx,0,    2,16,2,  YV12_TO_YUYV, 0, -1
MAKE_COLORSPACE  yv12_to_uyvy_mmx,0,    2,16,2,  YV12_TO_YUYV, 1, -1

MAKE_COLORSPACE  yv12_to_yuyvi_mmx,0,   2,8,4,  YV12_TO_YUYVI, 0, -1
MAKE_COLORSPACE  yv12_to_uyvyi_mmx,0,   2,8,4,  YV12_TO_YUYVI, 1, -1

NON_EXEC_STACK
