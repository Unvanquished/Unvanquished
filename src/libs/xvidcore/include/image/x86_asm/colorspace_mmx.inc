;/*****************************************************************************
; *
; *  XVID MPEG-4 VIDEO CODEC
; *  - colorspace conversions -
; *
; *  Copyright(C) 2002-2003 Peter Ross <pross@xvid.org>
; *               2008      Michael Militzer <michael@xvid.org>
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
; ****************************************************************************/

;------------------------------------------------------------------------------
;
; MAKE_COLORSPACE(NAME,STACK, BYTES,PIXELS,ROWS, FUNC, ARG1)
;
; This macro provides a assembler width/height scroll loop
; NAME		function name
; STACK		additional stack bytes required by FUNC
; BYTES		bytes-per-pixel for the given colorspace
; PIXELS	pixels (columns) operated on per FUNC call
; VPIXELS	vpixels (rows) operated on per FUNC call
; FUNC		conversion macro name; we expect to find FUNC_INIT and FUNC macros
; ARG1		argument passed to FUNC
; 
; throughout the FUNC the registers mean:
;------------------------------------------------------------------------------

%define y_stride       _EAX
%define u_ptr          _EBX
%define v_ptr          _ECX
%define x_stride       _EDX
%define x_stride_d      edx
%define y_ptr          _ESI
%define x_ptr          _EDI
%define width          _EBP

%macro		MAKE_COLORSPACE			8
%define NAME		%1
%define STACK		%2
%define BYTES		%3
%define PIXELS		%4
%define VPIXELS		%5
%define FUNC		%6
%define ARG1		%7
%define ARG2		%8
	; --- define function global/symbol
ALIGN SECTION_ALIGN
cglobal NAME
NAME:
	; --- init stack ---

  push _EBX    ;   esp + localsize + 16

%ifdef ARCH_IS_X86_64

%define localsize	2*PTR_SIZE + STACK
%ifndef WINDOWS
%define pushsize	2*PTR_SIZE
%define shadow          0
%else
%define pushsize	4*PTR_SIZE
%define shadow          32 + 2*PTR_SIZE
%endif

%define prm_vflip           dword [_ESP + localsize + pushsize + shadow + 4*PTR_SIZE]
%define prm_height          dword [_ESP + localsize + pushsize + shadow + 3*PTR_SIZE]
%define prm_width           dword [_ESP + localsize + pushsize + shadow + 2*PTR_SIZE]
%define prm_uv_stride       dword [_ESP + localsize + pushsize + shadow + 1*PTR_SIZE]

%ifdef WINDOWS
%define prm_y_stride        dword [_ESP + localsize + pushsize + shadow + 0*PTR_SIZE]
%define prm_v_ptr           [_ESP + localsize + pushsize + shadow - 1*PTR_SIZE]

  push _ESI    ;   esp + localsize + 8
  push _EDI    ;   esp + localsize + 4

%else
%define prm_y_stride        prm6d
%define prm_v_ptr           prm5
%endif

%define prm_u_ptr           prm4
%define prm_y_ptr           prm3
%define prm_x_stride        prm2d
%define prm_x_ptr           prm1
%define _ip                 _ESP + localsize + pushsize + 0

%define x_dif               TMP0

%else

%define localsize	5*PTR_SIZE + STACK
%define pushsize	4*PTR_SIZE

%define prm_vflip           [_ESP + localsize + pushsize + 10*PTR_SIZE]
%define prm_height          [_ESP + localsize + pushsize +  9*PTR_SIZE]
%define prm_width           [_ESP + localsize + pushsize +  8*PTR_SIZE]
%define prm_uv_stride       [_ESP + localsize + pushsize +  7*PTR_SIZE]
%define prm_y_stride        [_ESP + localsize + pushsize +  6*PTR_SIZE]
%define prm_v_ptr           [_ESP + localsize + pushsize +  5*PTR_SIZE]
%define prm_u_ptr           [_ESP + localsize + pushsize +  4*PTR_SIZE]
%define prm_y_ptr           [_ESP + localsize + pushsize +  3*PTR_SIZE]
%define prm_x_stride        [_ESP + localsize + pushsize +  2*PTR_SIZE]
%define prm_x_ptr           [_ESP + localsize + pushsize +  1*PTR_SIZE]
%define _ip                 _ESP + localsize + pushsize + 0

%define x_dif               dword [_ESP + localsize -  5*4]

  push _ESI    ;   esp + localsize + 8
  push _EDI    ;   esp + localsize + 4

%endif

  push _EBP    ;   esp + localsize + 0

%define y_dif           dword [_ESP + localsize -  1*4]
%define uv_dif          dword [_ESP + localsize -  2*4]
%define fixed_width     dword [_ESP + localsize -  3*4]
%define tmp_height      dword [_ESP + localsize -  4*4]

  sub _ESP, localsize

    ; --- init varibles ---
    
  mov eax, prm_width         ; fixed width
  add eax, 15                ;
  and eax, ~15               ;
  mov fixed_width, eax       ;

  mov ebx, prm_x_stride      ;
%rep BYTES
  sub _EBX, _EAX             ;
%endrep
  mov x_dif, _EBX            ; x_dif = x_stride - BYTES*fixed_width

  mov ebx, prm_y_stride      ;
  sub ebx, eax               ;
  mov y_dif, ebx             ; y_dif = y_stride - fixed_width

  mov ebx, prm_uv_stride     ;
  mov TMP1, _EAX             ;
  shr TMP1, 1                ;
  sub _EBX, TMP1             ;
  mov uv_dif, ebx            ; uv_dif = uv_stride - fixed_width/2

%ifdef ARCH_IS_X86_64
%ifndef WINDOWS
  mov TMP1d, prm_x_stride
  mov _ESI, prm_y_ptr
  mov _EDX, TMP1
%else
  mov _ESI, prm_y_ptr
  mov _EDI, prm_x_ptr
%endif
%else
  mov _ESI, prm_y_ptr        ; $esi$ = y_ptr
  mov _EDI, prm_x_ptr        ; $edi$ = x_ptr
  mov  edx, prm_x_stride     ; $edx$ = x_stride
%endif

  mov ebp, prm_height       ; $ebp$ = height

  mov ebx, prm_vflip
  or _EBX, _EBX
  jz .dont_flip

    ; --- do flipping ---

  xor _EBX,_EBX
%rep BYTES
  sub _EBX, _EAX
%endrep
  sub _EBX, _EDX
  mov x_dif, _EBX            ; x_dif = -BYTES*fixed_width - x_stride

  mov _EAX, _EBP
  sub _EAX, 1
%ifdef ARCH_IS_X86_64
  mov TMP1, _EDX
  mul edx 
  mov _EDX, TMP1
%else
  push _EDX                
  mul edx 
  pop _EDX
%endif
  add _EDI, _EAX             ; $edi$ += (height-1) * x_stride

  neg _EDX                   ; x_stride = -x_stride

.dont_flip:

    ; --- begin loop ---

  mov  eax, prm_y_stride     ; $eax$ = y_stride
  mov _EBX, prm_u_ptr        ; $ebx$ = u_ptr
  mov _ECX, prm_v_ptr        ; $ecx$ = v_ptr

  FUNC %+ _INIT ARG1, ARG2   ; call FUNC_INIT

.y_loop:
  mov tmp_height, ebp
  mov ebp, fixed_width

.x_loop:
  FUNC ARG1, ARG2            ; call FUNC

  add _EDI, BYTES*PIXELS     ; x_ptr += BYTES*PIXELS
  add _ESI, PIXELS           ; y_ptr += PIXELS
  add _EBX, PIXELS/2         ; u_ptr += PIXELS/2
  add _ECX, PIXELS/2         ; v_ptr += PIXELS/2
        
  sub _EBP, PIXELS           ; $ebp$ -= PIXELS
  jg .x_loop                 ; if ($ebp$ > 0) goto .x_loop

  mov ebp, tmp_height
  add _EDI, x_dif            ; x_ptr += x_dif + (VPIXELS-1)*x_stride
%ifdef ARCH_IS_X86_64
  mov TMP1d, y_dif
  add _ESI, TMP1             ; y_ptr += y_dif + (VPIXELS-1)*y_stride
%else
  add _ESI, y_dif            ; y_ptr += y_dif + (VPIXELS-1)*y_stride
%endif

%rep VPIXELS-1
  add _EDI, _EDX            
  add _ESI, _EAX          
%endrep

%ifdef ARCH_IS_X86_64
  mov TMP1d, uv_dif
  add _EBX, TMP1             ; u_ptr += uv_dif + ((VPIXELS/2)-1)*uv_stride
  add _ECX, TMP1             ; v_ptr += uv_dif + ((VPIXELS/2)-1)*uv_stride
%else
  add _EBX, uv_dif           ; u_ptr += uv_dif + ((VPIXELS/2)-1)*uv_stride
  add _ECX, uv_dif           ; v_ptr += uv_dif + ((VPIXELS/2)-1)*uv_stride
%endif

%rep (VPIXELS/2)-1
%ifdef ARCH_IS_X86_64
  mov TMP1d, prm_uv_stride
  add _EBX, TMP1
  add _ECX, TMP1
%else
  add _EBX, prm_uv_stride
  add _ECX, prm_uv_stride
%endif
%endrep

  sub _EBP, VPIXELS          ; $ebp$ -= VPIXELS
  jg .y_loop                 ; if ($ebp$ > 0) goto .y_loop

  ; cleanup stack & undef everything

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

%undef prm_vflip
%undef prm_height
%undef prm_width
%undef prm_uv_stride
%undef prm_y_stride
%undef prm_v_ptr
%undef prm_u_ptr
%undef prm_y_ptr
%undef prm_x_stride
%undef prm_x_ptr
%undef _ip
%undef x_dif
%undef y_dif
%undef uv_dif
%undef fixed_width
%undef tmp_height
        ret
ENDFUNC
%undef NAME
%undef STACK
%undef BYTES
%undef PIXELS
%undef VPIXELS
%undef FUNC
%undef ARG1
%endmacro
;------------------------------------------------------------------------------
