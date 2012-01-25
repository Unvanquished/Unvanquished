/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - emms related header -
 *
 *  Copyright(C) 2002 Michael Militzer <isibaar@xvid.org>
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
 * $Id: emms.h,v 1.16.4.1 2008/12/02 14:00:09 Isibaar Exp $
 *
 ****************************************************************************/

#ifndef _EMMS_H_
#define _EMMS_H_

/*****************************************************************************
 * emms API
 ****************************************************************************/

typedef void (emmsFunc) ();

typedef emmsFunc *emmsFuncPtr;

/* Our global function pointer - defined in emms.c */
extern emmsFuncPtr emms;

/* Implemented functions */

emmsFunc emms_c;

#if defined(ARCH_IS_IA32) || defined(ARCH_IS_X86_64)
emmsFunc emms_mmx;
emmsFunc emms_3dn;
#endif

/*****************************************************************************
 * Prototypes
 ****************************************************************************/

#if defined(ARCH_IS_IA32) || defined(ARCH_IS_X86_64)
/* cpu_flag detection helper functions */
extern int check_cpu_features(void);
extern void sse_os_trigger(void);
extern void sse2_os_trigger(void);
#endif

#if defined(ARCH_IS_X86_64) && defined(WIN32)
extern void prime_xmm(void *);
extern void get_xmm(void *);
#endif

#ifdef ARCH_IS_PPC
extern void altivec_trigger(void);
#endif

#endif
