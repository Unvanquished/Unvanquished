/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - 8<->16 bit buffer transfer header -
 *
 *  Copyright(C) 2001-2003 Peter Ross <pross@xvid.org>
 *
 *  This program is free software ; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation ; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY ; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program ; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 * $Id: mem_transfer.h,v 1.18 2008/11/26 01:04:34 Isibaar Exp $
 *
 ****************************************************************************/

#ifndef _MEM_TRANSFER_H
#define _MEM_TRANSFER_H

/*****************************************************************************
 * transfer8to16 API
 ****************************************************************************/

typedef void (TRANSFER_8TO16COPY) (int16_t * const dst,
								   const uint8_t * const src,
								   uint32_t stride);

typedef TRANSFER_8TO16COPY *TRANSFER_8TO16COPY_PTR;

/* Our global function pointer - Initialized in xvid.c */
extern TRANSFER_8TO16COPY_PTR transfer_8to16copy;

/* Implemented functions */
extern TRANSFER_8TO16COPY transfer_8to16copy_c;

#if defined(ARCH_IS_IA32) || defined(ARCH_IS_X86_64)
extern TRANSFER_8TO16COPY transfer_8to16copy_mmx;
extern TRANSFER_8TO16COPY transfer_8to16copy_3dne;
#endif

#ifdef ARCH_IS_IA64
extern TRANSFER_8TO16COPY transfer_8to16copy_ia64;
#endif

#ifdef ARCH_IS_PPC
extern TRANSFER_8TO16COPY transfer_8to16copy_altivec_c;
#endif

/*****************************************************************************
 * transfer16to8 API
 ****************************************************************************/

typedef void (TRANSFER_16TO8COPY) (uint8_t * const dst,
								   const int16_t * const src,
								   uint32_t stride);
typedef TRANSFER_16TO8COPY *TRANSFER_16TO8COPY_PTR;

/* Our global function pointer - Initialized in xvid.c */
extern TRANSFER_16TO8COPY_PTR transfer_16to8copy;

/* Implemented functions */
extern TRANSFER_16TO8COPY transfer_16to8copy_c;

#if defined(ARCH_IS_IA32) || defined(ARCH_IS_X86_64)
extern TRANSFER_16TO8COPY transfer_16to8copy_mmx;
extern TRANSFER_16TO8COPY transfer_16to8copy_3dne;
#endif

#ifdef ARCH_IS_IA64
extern TRANSFER_16TO8COPY transfer_16to8copy_ia64;
#endif

#ifdef ARCH_IS_PPC
extern TRANSFER_16TO8COPY transfer_16to8copy_altivec_c;
#endif

#ifdef ARCH_IS_X86_64
extern TRANSFER_16TO8COPY transfer_16to8copy_x86_64;
#endif

/*****************************************************************************
 * transfer8to16 + substraction *writeback* op API
 ****************************************************************************/

typedef void (TRANSFER_8TO16SUB) (int16_t * const dct,
								  uint8_t * const cur,
								  const uint8_t * ref,
								  const uint32_t stride);

typedef TRANSFER_8TO16SUB *TRANSFER_8TO16SUB_PTR;

/* Our global function pointer - Initialized in xvid.c */
extern TRANSFER_8TO16SUB_PTR transfer_8to16sub;

/* Implemented functions */
extern TRANSFER_8TO16SUB transfer_8to16sub_c;

#if defined(ARCH_IS_IA32) || defined(ARCH_IS_X86_64)
extern TRANSFER_8TO16SUB transfer_8to16sub_mmx;
extern TRANSFER_8TO16SUB transfer_8to16sub_3dne;
#endif

#ifdef ARCH_IS_IA64
extern TRANSFER_8TO16SUB transfer_8to16sub_ia64;
#endif

#ifdef ARCH_IS_PPC
extern TRANSFER_8TO16SUB transfer_8to16sub_altivec_c;
#endif

#ifdef ARCH_IS_X86_64
extern TRANSFER_8TO16SUB transfer_8to16sub_x86_64;
#endif

/*****************************************************************************
 * transfer8to16 + substraction *readonly* op API
 ****************************************************************************/

typedef void (TRANSFER_8TO16SUBRO) (int16_t * const dct,
								  const uint8_t * const cur,
								  const uint8_t * ref,
								  const uint32_t stride);

typedef TRANSFER_8TO16SUBRO *TRANSFER_8TO16SUBRO_PTR;

/* Our global function pointer - Initialized in xvid.c */
extern TRANSFER_8TO16SUBRO_PTR transfer_8to16subro;

/* Implemented functions */
extern TRANSFER_8TO16SUBRO transfer_8to16subro_c;

#if defined(ARCH_IS_IA32) || defined(ARCH_IS_X86_64)
extern TRANSFER_8TO16SUBRO transfer_8to16subro_mmx;
extern TRANSFER_8TO16SUBRO transfer_8to16subro_3dne;
#endif

#ifdef ARCH_IS_PPC
extern TRANSFER_8TO16SUBRO transfer_8to16subro_altivec_c;
#endif

#ifdef ARCH_IS_X86_64
extern TRANSFER_8TO16SUBRO transfer_8to16subro_x86_64;
#endif

/*****************************************************************************
 * transfer8to16 + substraction op API - Bidirectionnal Version
 ****************************************************************************/

typedef void (TRANSFER_8TO16SUB2) (int16_t * const dct,
								   uint8_t * const cur,
								   const uint8_t * ref1,
								   const uint8_t * ref2,
								   const uint32_t stride);

typedef TRANSFER_8TO16SUB2 *TRANSFER_8TO16SUB2_PTR;

/* Our global function pointer - Initialized in xvid.c */
extern TRANSFER_8TO16SUB2_PTR transfer_8to16sub2;

/* Implemented functions */
TRANSFER_8TO16SUB2 transfer_8to16sub2_c;

#if defined(ARCH_IS_IA32) || defined(ARCH_IS_X86_64)
extern TRANSFER_8TO16SUB2 transfer_8to16sub2_mmx;
extern TRANSFER_8TO16SUB2 transfer_8to16sub2_xmm;
extern TRANSFER_8TO16SUB2 transfer_8to16sub2_3dne;
#endif

#ifdef ARCH_IS_IA64
extern TRANSFER_8TO16SUB2 transfer_8to16sub2_ia64;
#endif

#ifdef ARCH_IS_PPC
extern TRANSFER_8TO16SUB2 transfer_8to16sub2_altivec_c;
#endif

#ifdef ARCH_IS_X86_64
extern TRANSFER_8TO16SUB2 transfer_8to16sub2_x86_64;
#endif

/*****************************************************************************
 * transfer8to16 + substraction op API - Bidirectionnal Version *readonly*
 ****************************************************************************/

typedef void (TRANSFER_8TO16SUB2RO) (int16_t * const dct,
								   const uint8_t * const cur,
								   const uint8_t * ref1,
								   const uint8_t * ref2,
								   const uint32_t stride);

typedef TRANSFER_8TO16SUB2RO *TRANSFER_8TO16SUB2RO_PTR;

/* Our global function pointer - Initialized in xvid.c */
extern TRANSFER_8TO16SUB2RO_PTR transfer_8to16sub2ro;

/* Implemented functions */
TRANSFER_8TO16SUB2RO transfer_8to16sub2ro_c;

#if defined(ARCH_IS_IA32) || defined(ARCH_IS_X86_64)
extern TRANSFER_8TO16SUB2RO transfer_8to16sub2ro_xmm;
#endif

#ifdef ARCH_IS_X86_64
extern TRANSFER_8TO16SUB2RO transfer_8to16sub2ro_x86_64;
#endif

/*****************************************************************************
 * transfer16to8 + addition op API
 ****************************************************************************/

typedef void (TRANSFER_16TO8ADD) (uint8_t * const dst,
								  const int16_t * const src,
								  uint32_t stride);

typedef TRANSFER_16TO8ADD *TRANSFER_16TO8ADD_PTR;

/* Our global function pointer - Initialized in xvid.c */
extern TRANSFER_16TO8ADD_PTR transfer_16to8add;

/* Implemented functions */
extern TRANSFER_16TO8ADD transfer_16to8add_c;

#if defined(ARCH_IS_IA32) || defined(ARCH_IS_X86_64)
extern TRANSFER_16TO8ADD transfer_16to8add_mmx;
extern TRANSFER_16TO8ADD transfer_16to8add_3dne;
#endif

#ifdef ARCH_IS_IA64
extern TRANSFER_16TO8ADD transfer_16to8add_ia64;
#endif

#ifdef ARCH_IS_PPC
extern TRANSFER_16TO8ADD transfer_16to8add_altivec_c;
#endif

#ifdef ARCH_IS_X86_64
extern TRANSFER_16TO8ADD transfer_16to8add_x86_64;
#endif

/*****************************************************************************
 * transfer8to8 + no op
 ****************************************************************************/

typedef void (TRANSFER8X8_COPY) (uint8_t * const dst,
								 const uint8_t * const src,
								 const uint32_t stride);

typedef TRANSFER8X8_COPY *TRANSFER8X8_COPY_PTR;

/* Our global function pointer - Initialized in xvid.c */
extern TRANSFER8X8_COPY_PTR transfer8x8_copy;

/* Implemented functions */
extern TRANSFER8X8_COPY transfer8x8_copy_c;

#if defined(ARCH_IS_IA32) || defined(ARCH_IS_X86_64)
extern TRANSFER8X8_COPY transfer8x8_copy_mmx;
extern TRANSFER8X8_COPY transfer8x8_copy_3dne;
#endif

#ifdef ARCH_IS_IA64
extern TRANSFER8X8_COPY transfer8x8_copy_ia64;
#endif

#ifdef ARCH_IS_PPC
extern TRANSFER8X8_COPY transfer8x8_copy_altivec_c;
#endif

#ifdef ARCH_IS_X86_64
extern TRANSFER8X8_COPY transfer8x8_copy_x86_64;
#endif

/*****************************************************************************
 * transfer8to4 + no op
 ****************************************************************************/

typedef void (TRANSFER8X4_COPY) (uint8_t * const dst,
								 const uint8_t * const src,
								 const uint32_t stride);

typedef TRANSFER8X4_COPY *TRANSFER8X4_COPY_PTR;

/* Our global function pointer - Initialized in xvid.c */
extern TRANSFER8X4_COPY_PTR transfer8x4_copy;

/* Implemented functions */
extern TRANSFER8X4_COPY transfer8x4_copy_c;

#if defined(ARCH_IS_IA32) || defined(ARCH_IS_X86_64)
extern TRANSFER8X4_COPY transfer8x4_copy_mmx;
extern TRANSFER8X4_COPY transfer8x4_copy_3dne;
#endif


static __inline void
transfer16x16_copy(uint8_t * const dst,
					const uint8_t * const src,
					const uint32_t stride)
{
	transfer8x8_copy(dst, src, stride);
	transfer8x8_copy(dst + 8, src + 8, stride);
	transfer8x8_copy(dst + 8*stride, src + 8*stride, stride);
	transfer8x8_copy(dst + 8*stride + 8, src + 8*stride + 8, stride);
}

static __inline void
transfer32x32_copy(uint8_t * const dst,
					const uint8_t * const src,
					const uint32_t stride)
{
	transfer16x16_copy(dst, src, stride);
	transfer16x16_copy(dst + 16, src + 16, stride);
	transfer16x16_copy(dst + 16*stride, src + 16*stride, stride);
	transfer16x16_copy(dst + 16*stride + 16, src + 16*stride + 16, stride);
}


#endif
