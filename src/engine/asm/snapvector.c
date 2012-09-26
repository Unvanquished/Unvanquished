/*
===========================================================================
Copyright (C) 2011 Thilo Schulz <thilo@tjps.eu>

This file is part of Daemon source code.

Daemon source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Daemon source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "qasm-inline.h"
#include "../qcommon/q_shared.h"

/*
 * The GNU inline asm version of qsnapvector
 * See snapvector.asm (the MASM version) for commentary
 */

static unsigned char ssemask[16] ALIGNED(16) =
{
	"\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x00\x00\x00\x00"
};

void qsnapvectorsse(vec3_t vec)
{
	__asm__ volatile
	(
		"movaps (%0), %%xmm1\n"
		"movups (%1), %%xmm0\n"
		"movaps %%xmm0, %%xmm2\n"
		"andps %%xmm1, %%xmm0\n"
		"andnps %%xmm2, %%xmm1\n"
		"cvtps2dq %%xmm0, %%xmm0\n"
		"cvtdq2ps %%xmm0, %%xmm0\n"
		"orps %%xmm1, %%xmm0\n"
		"movups %%xmm0, (%1)\n"
		:
	// there's a Clang/LLVM warning for an uninitialized use of the oldcw variable.
	// i wasn't able to come up with anything better than the use of compiler
	// pragmas to silence the warning. TODO: come up with a better solution.
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wuninitialized"
#endif
		: "r" (ssemask), "r" (vec)
#ifdef __clang__
#pragma clang diagnostic pop
#endif
		: "memory", "%xmm0", "%xmm1", "%xmm2"
	);
}

#define QROUNDX87(src) \
	"flds " src "\n" \
	"fistps " src "\n" \
	"filds " src "\n" \
	"fstps " src "\n"

void qsnapvectorx87(vec3_t vec)
{
	__asm__ volatile
	(
		QROUNDX87("(%0)")
		QROUNDX87("4(%0)")
		QROUNDX87("8(%0)")
		:
		: "r" (vec)
		: "memory"
	);
}
