/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - Motion module header -
 *
 *  Copyright(C) 2002-2003 Radoslaw Czyz <xvid@syskin.cjb.net>
 *               2002 Michael Militzer <michael@xvid.org>
 *               
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
 *  $Id: motion.h,v 1.24.2.1 2009/05/25 09:03:47 Isibaar Exp $
 *
 ***************************************************************************/

#ifndef _MOTION_H_
#define _MOTION_H_

#include "../portab.h"
#include "../global.h"

/*****************************************************************************
 * Modified rounding tables -- defined in estimation_common.c
 * Original tables see ISO spec tables 7-6 -> 7-9
 ****************************************************************************/

extern const uint32_t roundtab[16];
/* K = 4 */
extern const uint32_t roundtab_76[16];
/* K = 2 */
extern const uint32_t roundtab_78[8];
/* K = 1 */
extern const uint32_t roundtab_79[4];


/** MotionEstimation **/

void MotionEstimation(MBParam * const pParam,
					FRAMEINFO * const current,
					FRAMEINFO * const reference,
					const IMAGE * const pRefH,
					const IMAGE * const pRefV,
					const IMAGE * const pRefHV,
					const IMAGE * const pGMC,
					const uint32_t iLimit);

void
MotionEstimationBVOP(MBParam * const pParam,
						FRAMEINFO * const frame,
						const int32_t time_bp,
						const int32_t time_pp,
						const MACROBLOCK * const f_mbs,
						const IMAGE * const f_ref,
						const IMAGE * const f_refH,
						const IMAGE * const f_refV,
						const IMAGE * const f_refHV,
						const FRAMEINFO * const b_reference,
						const IMAGE * const b_ref,
						const IMAGE * const b_refH,
						const IMAGE * const b_refV,
						const IMAGE * const b_refHV);

void
GMEanalysis(const MBParam * const pParam,
			const FRAMEINFO * const current,
			const FRAMEINFO * const reference,
			const IMAGE * const pRefH,
			const IMAGE * const pRefV,
			const IMAGE * const pRefHV);

WARPPOINTS
GlobalMotionEst(MACROBLOCK * const pMBs,
				const MBParam * const pParam,
				const FRAMEINFO * const current,
				const FRAMEINFO * const reference,
				const IMAGE * const pRefH,
				const IMAGE * const pRefV,
				const IMAGE * const pRefHV);

int
GlobalMotionEstRefine(
				WARPPOINTS *const startwp,
				MACROBLOCK * const pMBs,
				const MBParam * const pParam,
				const FRAMEINFO * const current,
				const FRAMEINFO * const reference,
				const IMAGE * const pCurr,
				const IMAGE * const pRef,
				const IMAGE * const pRefH,
				const IMAGE * const pRefV,
				const IMAGE * const pRefHV);

int
globalSAD(const WARPPOINTS *const wp,
		  const MBParam * const pParam,
		  const MACROBLOCK * const pMBs,
		  const FRAMEINFO * const current,
		  const IMAGE * const pRef,
		  const IMAGE * const pCurr,
		  uint8_t *const GMCblock);


int
MEanalysis(	const IMAGE * const pRef,
			const FRAMEINFO * const Current,
			const MBParam * const pParam,
			const int maxIntra,
			const int intraCount,
			const int bCount,
			const int b_thresh,
			const MACROBLOCK * const prev_mbs);

/** MotionCompensation **/

void
MBMotionCompensation(MACROBLOCK * const mb,
					const uint32_t i,
					const uint32_t j,
					const IMAGE * const ref,
					const IMAGE * const refh,
					const IMAGE * const refv,
					const IMAGE * const refhv,
					const IMAGE * const refGMC,
					IMAGE * const cur,
					int16_t * dct_codes,
					const uint32_t width,
					const uint32_t height,
					const uint32_t edged_width,
					const int32_t quarterpel,
					const int32_t rounding);

void
MBMotionCompensationBVOP(MBParam * pParam,
							MACROBLOCK * const mb,
							const uint32_t i,
							const uint32_t j,
							IMAGE * const cur,
							const IMAGE * const f_ref,
							const IMAGE * const f_refh,
							const IMAGE * const f_refv,
							const IMAGE * const f_refhv,
							const IMAGE * const b_ref,
							const IMAGE * const b_refh,
							const IMAGE * const b_refv,
							const IMAGE * const b_refhv,
							int16_t * dct_codes);

#endif							/* _MOTION_H_ */
