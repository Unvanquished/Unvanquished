;/****************************************************************************
; *
; *  XVID MPEG-4 VIDEO CODEC
; *  - MMX and XMM YV12->YV12 conversion -
; *
; *  Copyright(C) 2001-2008 Michael Militzer <michael@xvid.org>
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
; * $Id: colorspace_yuv_mmx.asm,v 1.10.2.3 2009/09/16 17:11:39 Isibaar Exp $
; *
; ***************************************************************************/

%include "nasm.inc"

;=============================================================================
; Helper macros
;=============================================================================

;------------------------------------------------------------------------------
; PLANE_COPY ( DST, DST_STRIDE, SRC, SRC_STRIDE, WIDTH, HEIGHT, OPT )
; DST		dst buffer
; DST_STRIDE	dst stride
; SRC		src destination buffer
; SRC_STRIDE	src stride
; WIDTH		width
; HEIGHT	height
; OPT		0=plain mmx, 1=xmm
;
;
; Trashes: DST, SRC, WIDTH, HEIGHT, _EBX, _ECX, _EDX
;------------------------------------------------------------------------------

%macro	PLANE_COPY	7
%define DST		        %1
%define DST_STRIDE      	%2
%define SRC		        %3
%define SRC_STRIDE      	%4
%define WIDTH		    	%5
%define HEIGHT                  %6
%define OPT		        %7

  mov _EBX, WIDTH 
  shr WIDTH, 6              ; $_EAX$ = width / 64
  and _EBX, 63              ; remainder = width % 64
  mov _EDX, _EBX 
  shr _EBX, 4               ; $_EBX$ = remainder / 16
  and _EDX, 15              ; $_EDX$ = remainder % 16

%%loop64_start_pc:
  push DST
  push SRC

  mov  _ECX, WIDTH          ; width64
  test WIDTH, WIDTH 
  jz %%loop16_start_pc
  
%%loop64_pc:
%if OPT == 1                ; xmm
  prefetchnta [SRC + 64]    ; non temporal prefetch
  prefetchnta [SRC + 96]
%endif
  movq mm1, [SRC     ]      ; read from src
  movq mm2, [SRC +  8]
  movq mm3, [SRC + 16]
  movq mm4, [SRC + 24]
  movq mm5, [SRC + 32]
  movq mm6, [SRC + 40]
  movq mm7, [SRC + 48]
  movq mm0, [SRC + 56]

%if OPT == 0                ; plain mmx
  movq [DST     ], mm1      ; write to y_out
  movq [DST +  8], mm2
  movq [DST + 16], mm3
  movq [DST + 24], mm4
  movq [DST + 32], mm5
  movq [DST + 40], mm6
  movq [DST + 48], mm7
  movq [DST + 56], mm0
%else
  movntq [DST     ], mm1    ; write to y_out
  movntq [DST +  8], mm2
  movntq [DST + 16], mm3
  movntq [DST + 24], mm4
  movntq [DST + 32], mm5
  movntq [DST + 40], mm6
  movntq [DST + 48], mm7
  movntq [DST + 56], mm0
%endif

  add SRC, 64
  add DST, 64
  loop %%loop64_pc

%%loop16_start_pc:
  mov  _ECX, _EBX           ; width16
  test _EBX, _EBX 
  jz %%loop1_start_pc

%%loop16_pc:
  movq mm1, [SRC]
  movq mm2, [SRC + 8]
%if OPT == 0                ; plain mmx
  movq [DST], mm1
  movq [DST + 8], mm2
%else
  movntq [DST], mm1
  movntq [DST + 8], mm2
%endif

  add SRC, 16
  add DST, 16
  loop %%loop16_pc

%%loop1_start_pc:
  mov _ECX, _EDX 
  rep movsb

  pop SRC
  pop DST

%ifdef ARCH_IS_X86_64
  XVID_MOVSXD _ECX, SRC_STRIDE
  add SRC, _ECX
  mov ecx, DST_STRIDE
  add DST, _ECX
%else
  add SRC, SRC_STRIDE
  add DST, DST_STRIDE
%endif

  dec HEIGHT 
  jg near %%loop64_start_pc

%undef DST
%undef DST_STRIDE
%undef SRC		
%undef SRC_STRIDE
%undef WIDTH	
%undef HEIGHT	
%undef OPT
%endmacro

;------------------------------------------------------------------------------
; PLANE_FILL ( DST, DST_STRIDE, WIDTH, HEIGHT, OPT )
; DST		dst buffer
; DST_STRIDE	dst stride
; WIDTH		width
; HEIGHT	height
; OPT		0=plain mmx, 1=xmm
;
; Trashes: DST, WIDTH, HEIGHT, _EBX, _ECX, _EDX, _EAX
;------------------------------------------------------------------------------

%macro	PLANE_FILL	5
%define DST		%1
%define DST_STRIDE      %2
%define WIDTH		%3
%define HEIGHT		%4
%define OPT		%5

  mov _EAX, 0x80808080
  mov _EBX, WIDTH
  shr WIDTH, 6               ; $_ESI$ = width / 64
  and _EBX, 63               ; _EBX = remainder = width % 64
  movd mm0, eax 
  mov _EDX, _EBX
  shr _EBX, 4                ; $_EBX$ = remainder / 16
  and _EDX, 15               ; $_EDX$ = remainder % 16
  punpckldq mm0, mm0

%%loop64_start_pf:
  push DST
  mov  _ECX, WIDTH           ; width64
  test WIDTH, WIDTH
  jz %%loop16_start_pf

%%loop64_pf:

%if OPT == 0                 ; plain mmx
  movq [DST     ], mm0       ; write to y_out
  movq [DST +  8], mm0
  movq [DST + 16], mm0
  movq [DST + 24], mm0
  movq [DST + 32], mm0
  movq [DST + 40], mm0
  movq [DST + 48], mm0
  movq [DST + 56], mm0
%else
  movntq [DST     ], mm0     ; write to y_out
  movntq [DST +  8], mm0
  movntq [DST + 16], mm0
  movntq [DST + 24], mm0
  movntq [DST + 32], mm0
  movntq [DST + 40], mm0
  movntq [DST + 48], mm0
  movntq [DST + 56], mm0
%endif

  add DST, 64
  loop %%loop64_pf

%%loop16_start_pf:
  mov  _ECX, _EBX            ; width16
  test _EBX, _EBX
  jz %%loop1_start_pf

%%loop16_pf:
%if OPT == 0                 ; plain mmx
  movq [DST    ], mm0
  movq [DST + 8], mm0
%else
  movntq [DST    ], mm0
  movntq [DST + 8], mm0
%endif

  add DST, 16
  loop %%loop16_pf

%%loop1_start_pf:
  mov _ECX, _EDX
  rep stosb

  pop DST

%ifdef ARCH_IS_X86_64
  mov ecx, DST_STRIDE
  add DST, _ECX
%else
  add DST, DST_STRIDE
%endif

  dec HEIGHT
  jg near %%loop64_start_pf

%undef DST
%undef DST_STRIDE
%undef WIDTH	
%undef HEIGHT	
%undef OPT
%endmacro

;------------------------------------------------------------------------------
; MAKE_YV12_TO_YV12( NAME, OPT )
; NAME	function name
; OPT	0=plain mmx, 1=xmm
;
; yv12_to_yv12_mmx(uint8_t * y_dst, uint8_t * u_dst, uint8_t * v_dst,
; 				int y_dst_stride, int uv_dst_stride,
; 				uint8_t * y_src, uint8_t * u_src, uint8_t * v_src,
; 				int y_src_stride, int uv_src_stride,
; 				int width, int height, int vflip)
;------------------------------------------------------------------------------
%macro	MAKE_YV12_TO_YV12	2
%define	NAME		%1
%define	XMM_OPT		%2
ALIGN SECTION_ALIGN
cglobal NAME
NAME:

  push _EBX	;	_ESP + localsize + 3*PTR_SIZE

%define localsize	2*4

%ifdef ARCH_IS_X86_64

%ifndef WINDOWS
%define pushsize        2*PTR_SIZE
%define shadow          0
%else
%define pushsize	4*PTR_SIZE
%define shadow          32 + 2*PTR_SIZE 
%endif

%define prm_vflip	        dword [_ESP + localsize + pushsize + shadow + 7*PTR_SIZE]
%define prm_height	        dword [_ESP + localsize + pushsize + shadow + 6*PTR_SIZE]
%define prm_width        	dword [_ESP + localsize + pushsize + shadow + 5*PTR_SIZE]
%define prm_uv_src_stride	dword [_ESP + localsize + pushsize + shadow + 4*PTR_SIZE]
%define prm_y_src_stride	dword [_ESP + localsize + pushsize + shadow + 3*PTR_SIZE]
%define prm_v_src	  	      [_ESP + localsize + pushsize + shadow + 2*PTR_SIZE]
%define prm_u_src   	              [_ESP + localsize + pushsize + shadow + 1*PTR_SIZE]

%ifdef WINDOWS
  push _ESI	;	_ESP + localsize + 2*PTR_SIZE
  push _EDI	;	_ESP + localsize + 1*PTR_SIZE
  push _EBP	;	_ESP + localsize + 0*PTR_SIZE

  sub _ESP, localsize

%define prm_y_src		_ESI
%define prm_uv_dst_stride	TMP0d
%define prm_y_dst_stride	prm4d
%define prm_v_dst		prm3
%define prm_u_dst   	        TMP1
%define prm_y_dst	        _EDI

  mov _EDI, prm1
  mov TMP1, prm2

  mov _ESI, [_ESP + localsize + pushsize + shadow + 0*PTR_SIZE]
  mov TMP0d, dword [_ESP + localsize + pushsize + shadow - 1*PTR_SIZE]

%else
  push _EBP	;	_ESP + localsize + 0*PTR_SIZE

  sub _ESP, localsize

%define prm_y_src		_ESI
%define prm_uv_dst_stride	prm5d
%define prm_y_dst_stride	TMP1d
%define prm_v_dst		prm6
%define prm_u_dst   	        TMP0
%define prm_y_dst		_EDI

  mov TMP0, prm2
  mov _ESI, prm6

  mov prm6, prm3
  mov TMP1d, prm4d
%endif

%define _ip		_ESP + localsize + pushsize + 0

%else

%define pushsize	4*PTR_SIZE

%define prm_vflip		[_ESP + localsize + pushsize + 13*PTR_SIZE]
%define prm_height		[_ESP + localsize + pushsize + 12*PTR_SIZE]
%define prm_width        	[_ESP + localsize + pushsize + 11*PTR_SIZE]
%define prm_uv_src_stride	[_ESP + localsize + pushsize + 10*PTR_SIZE]
%define prm_y_src_stride	[_ESP + localsize + pushsize + 9*PTR_SIZE]
%define prm_v_src		[_ESP + localsize + pushsize + 8*PTR_SIZE]
%define prm_u_src   	        [_ESP + localsize + pushsize + 7*PTR_SIZE]

%define prm_y_src		_ESI
%define prm_uv_dst_stride	[_ESP + localsize + pushsize + 5*PTR_SIZE]
%define prm_y_dst_stride	[_ESP + localsize + pushsize + 4*PTR_SIZE]
%define prm_v_dst		[_ESP + localsize + pushsize + 3*PTR_SIZE]
%define prm_u_dst   	        [_ESP + localsize + pushsize + 2*PTR_SIZE]
%define prm_y_dst		_EDI

%define _ip		_ESP + localsize + pushsize + 0

  push _ESI	;	_ESP + localsize + 2*PTR_SIZE
  push _EDI	;	_ESP + localsize + 1*PTR_SIZE
  push _EBP	;	_ESP + localsize + 0*PTR_SIZE

  sub _ESP, localsize

  mov _ESI, [_ESP + localsize + pushsize + 6*PTR_SIZE]
  mov _EDI, [_ESP + localsize + pushsize + 1*PTR_SIZE]

%endif

%define width2			dword [_ESP + localsize - 1*4]
%define height2			dword [_ESP + localsize - 2*4]

  mov eax, prm_width
  mov ebx, prm_height
  shr eax, 1                    ; calculate widht/2, heigh/2
  shr ebx, 1
  mov width2, eax 
  mov height2, ebx 

  mov eax, prm_vflip
  test eax, eax 
  jz near .go

        ; flipping support
  mov eax, prm_height
  mov ecx, prm_y_src_stride
  sub eax, 1
  imul eax, ecx 
  add _ESI, _EAX                ; y_src += (height-1) * y_src_stride
  neg ecx 
  mov prm_y_src_stride, ecx     ; y_src_stride = -y_src_stride

  mov eax, height2
  mov _EDX, prm_u_src
  mov _EBP, prm_v_src
  mov ecx, prm_uv_src_stride
  test _EDX, _EDX 
  jz .go
  test _EBP, _EBP 
  jz .go
  sub eax, 1                     ; _EAX = height2 - 1
  imul eax, ecx 
  add _EDX, _EAX                 ; u_src += (height2-1) * uv_src_stride
  add _EBP, _EAX                 ; v_src += (height2-1) * uv_src_stride
  neg ecx 
  mov prm_u_src, _EDX 
  mov prm_v_src, _EBP 
  mov prm_uv_src_stride, ecx     ; uv_src_stride = -uv_src_stride

.go:
  mov eax, prm_width
  mov ebp, prm_height
  PLANE_COPY _EDI, prm_y_dst_stride,  _ESI, prm_y_src_stride,  _EAX,  _EBP, XMM_OPT

  mov _EAX, prm_u_src
  or  _EAX, prm_v_src
  jz near .UVFill_0x80

  mov eax, width2
  mov ebp, height2
  mov _ESI, prm_u_src
  mov _EDI, prm_u_dst
  PLANE_COPY _EDI, prm_uv_dst_stride, _ESI, prm_uv_src_stride, _EAX, _EBP, XMM_OPT

  mov eax, width2
  mov ebp, height2
  mov _ESI, prm_v_src
  mov _EDI, prm_v_dst
  PLANE_COPY _EDI, prm_uv_dst_stride, _ESI, prm_uv_src_stride, _EAX, _EBP, XMM_OPT

.Done_UVPlane:
  add _ESP, localsize

  pop _EBP
%ifndef ARCH_IS_X86_64
  pop _EDI
  pop _ESI
%else
%ifdef WINDOWS
  pop _EDI
  pop _ESI
%endif
%endif
  pop _EBX

  ret

.UVFill_0x80:

  mov esi, width2
  mov ebp, height2
  mov _EDI, prm_u_dst
  PLANE_FILL _EDI, prm_uv_dst_stride, _ESI, _EBP, XMM_OPT

  mov esi, width2
  mov ebp, height2
  mov _EDI, prm_v_dst
  PLANE_FILL _EDI, prm_uv_dst_stride, _ESI, _EBP, XMM_OPT

  jmp near .Done_UVPlane

ENDFUNC

%undef NAME
%undef XMM_OPT
%endmacro

;=============================================================================
; Code
;=============================================================================

TEXT

MAKE_YV12_TO_YV12	yv12_to_yv12_mmx, 0

MAKE_YV12_TO_YV12	yv12_to_yv12_xmm, 1

NON_EXEC_STACK
