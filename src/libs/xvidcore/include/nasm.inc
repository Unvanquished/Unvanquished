;/****************************************************************************
; *
; *  XVID MPEG-4 VIDEO CODEC
; *  - NASM common header -
; *
; *  Copyright (C) 2008 Michael Militzer
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
; * $Id: nasm.inc,v 1.1.2.5 2009/09/16 17:11:39 Isibaar Exp $
; *
; ***************************************************************************/

%ifdef ARCH_IS_X86_64

BITS 64
DEFAULT REL

%define SECTION_ALIGN 32

%ifdef WINDOWS

%define prm1 rcx
%define prm2 rdx
%define prm3 r8
%define prm4 r9
%define prm5 [rsp+40]
%define prm6 [rsp+48]
%define prm7 [rsp+56]
%define prm8 [rsp+64]

%define prm1d ecx
%define prm2d edx
%define prm3d r8d
%define prm4d r9d
%define prm5d dword prm5
%define prm6d dword prm6
%define prm7d dword prm7
%define prm8d dword prm8

%macro PUSH_XMM6_XMM7 0
      movdqa [_ESP+PTR_SIZE], xmm6
      movdqa [_ESP+PTR_SIZE+16], xmm7
%endmacro

%macro POP_XMM6_XMM7 0
      movdqa xmm6, [_ESP+PTR_SIZE]
      movdqa xmm7, [_ESP+PTR_SIZE+16]
%endmacro

%else ; Linux

%define prm1 rdi
%define prm2 rsi
%define prm3 rdx
%define prm4 rcx
%define prm5 r8
%define prm6 r9
%define prm7 [rsp+8]
%define prm8 [rsp+16]

%define prm1d edi
%define prm2d esi
%define prm3d edx
%define prm4d ecx
%define prm5d r8d
%define prm6d r9d
%define prm7d dword prm7
%define prm8d dword prm8

%define PUSH_XMM6_XMM7
%define POP_XMM6_XMM7

%endif

%define _EAX rax
%define _EBX rbx
%define _ECX rcx
%define _EDX rdx
%define _ESI rsi
%define _EDI rdi
%define _EBP rbp
%define _ESP rsp

%define TMP0 r10
%define TMP1 r11

%define TMP0d r10d
%define TMP1d r11d

%define PTR_SIZE 8
%define PTR_TYPE qword

%ifdef __YASM_VERSION_ID__
%define XVID_MOVSXD movsxd
%else
%define XVID_MOVSXD movsx
%endif

%else

%define SECTION_ALIGN 16

BITS 32

%define prm1 [esp +  4]
%define prm2 [esp +  8]
%define prm3 [esp + 12]
%define prm4 [esp + 16]
%define prm5 [esp + 20]
%define prm6 [esp + 24]
%define prm7 [esp + 28]
%define prm8 [esp + 32]

%define prm1d dword prm1
%define prm2d dword prm2
%define prm3d dword prm3
%define prm4d dword prm4
%define prm5d dword prm5
%define prm6d dword prm6
%define prm7d dword prm7
%define prm8d dword prm8

%define _EAX eax
%define _EBX ebx
%define _ECX ecx
%define _EDX edx
%define _ESI esi
%define _EDI edi
%define _EBP ebp
%define _ESP esp

%define TMP0 ecx
%define TMP1 edx

%define TMP0d ecx
%define TMP1d edx

%define PTR_SIZE 4
%define PTR_TYPE dword

%define PUSH_XMM6_XMM7
%define POP_XMM6_XMM7

%define XVID_MOVSXD movsx
%endif


%ifdef WINDOWS
      %define PREFIX
%endif

%ifdef NO_PREFIX
      %undef PREFIX
%endif

%macro DATA 0
      %ifdef FORMAT_COFF
            SECTION .rodata
      %else
            SECTION .rodata align=SECTION_ALIGN
      %endif
%endmacro

%macro TEXT 0
%ifidn __OUTPUT_FORMAT__,macho
      SECTION .text align=SECTION_ALIGN
%else
      SECTION .rotext align=SECTION_ALIGN
%endif
%endmacro

%macro cglobal 1
	%ifdef PREFIX
		%ifdef MARK_FUNCS
			global _%1:function %1.endfunc-%1
			%define %1 _%1:function %1.endfunc-%1
			%define ENDFUNC .endfunc:
		%else
			global _%1
			%define %1 _%1
			%define ENDFUNC
		%endif
	%else
		%ifdef MARK_FUNCS
			global %1:function %1.endfunc-%1
			%define ENDFUNC .endfunc:
		%else
			global %1
			%define ENDFUNC
		%endif
	%endif
%endmacro

%macro NON_EXEC_STACK 0
%ifidn __OUTPUT_FORMAT__,elf
section .note.GNU-stack noalloc noexec nowrite progbits
%endif
%ifidn __OUTPUT_FORMAT__,elf64
section .note.GNU-stack noalloc noexec nowrite progbits
%endif
%endmacro
