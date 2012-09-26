; ===========================================================================
; Copyright (C) 2011 Thilo Schulz <thilo@tjps.eu>
; 
; This file is part of Daemon source code.
; 
; Daemon source code is free software; you can redistribute it
; and/or modify it under the terms of the GNU General Public License as
; published by the Free Software Foundation; either version 2 of the License,
; or (at your option) any later version.
; 
; Daemon source code is distributed in the hope that it will be
; useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
; 
; You should have received a copy of the GNU General Public License
; along with Daemon source code; if not, write to the Free Software
; Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
; ===========================================================================

; Call wrapper for vm_x86 when built with MSVC in 64-bit mode,
; since MSVC does not support inline x64 assembly anymore.
;
; assumes the fastcall calling convention

.code

; Call to compiled code after setting up the register environment for the VM
; prototype:
; uint8_t qvmcall64(int *programStack, int *opStack, intptr_t *instructionPointers, byte *dataBase);

qvmcall64 PROC
  push rsi							; push non-volatile registers to stack
  push rdi
  push rbx
  ; need to save the pointer in rcx, so we can write back the programStack value to the caller
  push rcx

  ; registers r8 and r9 already have the correct values, thanks to fastcall
  xor rbx, rbx						; opStackOfs starts out as 0
  mov rdi, rdx						; opStack
  mov esi, dword ptr [rcx]			; programStack

  call qword ptr [r8]				; instructionPointers[0] is also the entry point

  pop rcx

  mov dword ptr [rcx], esi			; write back the programStack value
  mov al, bl						; return the opStack offset

  pop rbx
  pop rdi
  pop rsi

  ret
qvmcall64 ENDP

end
