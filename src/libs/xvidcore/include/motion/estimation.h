/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - Motion Estimation related header -
 *
 *  Copyright(C) 2002 Christoph Lampert <gruel@web.de>
 *               2002 Michael Militzer <michael@xvid.org>
 *               2002-2003 Radoslaw Czyz <xvid@syskin.cjb.net>
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
 * $Id: estimation.h,v 1.13 2005/12/09 04:39:49 syskin Exp $
 *
 ****************************************************************************/

#ifndef _ESTIMATION_H_
#define _ESTIMATION_H_

#include "../portab.h"
#include "../global.h"

/* hard coded motion search parameters */

/* very large value */
#define MV_MAX_ERROR	(4096 * 256)

/* INTER bias for INTER/INTRA decision; mpeg4 spec suggests 2*nb */
#define MV16_INTER_BIAS	450

/* vector map (vlc delta size) smoother parameters ! float !*/
#define NEIGH_TEND_16X16		0.6
#define NEIGH_TEND_8X8			1.5

#define NEIGH_8X8_BIAS			40

#define BITS_MULT				16

#define INITIAL_SKIP_THRESH		6
#define FINAL_SKIP_THRESH		50
#define MAX_SAD00_FOR_SKIP		20
#define MAX_CHROMA_SAD_FOR_SKIP	22

/* Parameters which control inter/inter4v decision */
#define IMV16X16				2

extern const int xvid_me_lambda_vec16[32];

#define CHECK_CANDIDATE(X,Y,D) { \
	CheckCandidate((X),(Y), data, (D) ); }

/* fast ((A)/2)*2 */
#define EVEN(A)		(((A)<0?(A)+1:(A)) & ~1)

#define MVequal(A,B) ( ((A).x)==((B).x) && ((A).y)==((B).y) )

static const VECTOR zeroMV = { 0, 0 };

typedef struct
{
	int max_dx, min_dx, max_dy, min_dy; /* maximum search range */

	/* data modified by CheckCandidates */
	int32_t iMinSAD[5];			/* smallest SADs found so far */
	VECTOR currentMV[5];		/* best vectors found so far */
	VECTOR currentQMV[5];		/* as above, but used during qpel search */
	int temp[4];				/* temporary space */
	unsigned int dir;			/* 'direction', set when better vector is found */
	int chromaX, chromaY, chromaSAD; /* info to make ChromaSAD faster */

	/* general fields */
	uint32_t rounding;			/* rounding type in use */
	VECTOR predMV;				/* vector which predicts current vector */
	const uint8_t * RefP[6];	/* reference pictures - N, V, H, HV, cU, cV */
	const uint8_t * Cur;		/* current picture */
	const uint8_t *CurU, *CurV;	/* current picture - chroma planes */
	
	uint8_t * RefQ;				/* temporary space for interpolations */
	uint32_t lambda16;			/* how much vector bits weight */
	uint32_t lambda8;			/* as above - for inter4v mode */
	uint32_t iEdgedWidth;		/* picture's stride */
	uint32_t iFcode;			/* current fcode */

	int qpel;					/* if we're coding in qpel mode */
	int qpel_precision;			/* if X and Y are in qpel precision (refinement probably) */
	int chroma;					/* should we include chroma SAD? */

	/* fields for interpolate and direct modes */
	const uint8_t * b_RefP[6];	/* backward reference pictures - N, V, H, HV, cU, cV */
	VECTOR bpredMV;				/* backward prediction - used in Interpolate-mode search only */
	uint32_t bFcode;			/* backward fcode - used in Interpolate-mode search only */
	int b_chromaX, b_chromaY;

	/* fields for direct mode */
	VECTOR directmvF[4];		/* scaled reference vectors */
	VECTOR directmvB[4];
	const VECTOR * referencemv; /* pointer to not-scaled reference vectors */

	/* BITS/R-D stuff */
	int16_t * dctSpace;			/* temporary space for dct */
	uint32_t iQuant;			/* current quant */
	uint32_t quant_type;		/* current quant type */
	unsigned int cbp[2];		/* CBP of the best vector found so far + cbp for inter4v search */
	const uint16_t * scan_table; /* current scan table */
	const uint16_t * mpeg_quant_matrices;			/* current MPEG quantization matrices */
	int lambda[6];				/* R-D lambdas for all 6 blocks */
	unsigned int quant_sq;		/* quant squared - saves many multiplications in VHQ */

} SearchData;

typedef void(CheckFunc)(const int x, const int y,
						SearchData * const Data,
						const unsigned int Direction);

CheckFunc CheckCandidate16no4v; /* shared between p-vop and b-vop search */

uint8_t *
xvid_me_interpolate8x8qpel(const int x, const int y, const uint32_t block,
							const uint32_t dir, const SearchData * const data);

uint8_t *
xvid_me_interpolate16x16qpel(const int x, const int y, const uint32_t dir,
							const SearchData * const data);

int32_t
xvid_me_ChromaSAD(const int dx, const int dy, SearchData * const data);

int
xvid_me_SkipDecisionP(const IMAGE * current, const IMAGE * reference,
					const int x, const int y,
					const uint32_t stride, const uint32_t iQuant);

#define iDiamondSize 2
typedef void
MainSearchFunc(int x, int y, SearchData * const Data,
			   int bDirection, CheckFunc * const CheckCandidate);

MainSearchFunc xvid_me_DiamondSearch, xvid_me_AdvDiamondSearch, xvid_me_SquareSearch;

void
xvid_me_SubpelRefine(VECTOR centerMV, SearchData * const data, CheckFunc * const CheckCandidate, int dir);

void 
FullRefine_Fast(SearchData * data, CheckFunc * CheckCandidate, int direction);

void
xvid_me_ModeDecision_RD(SearchData * const Data,
		MACROBLOCK * const pMB,
		const MACROBLOCK * const pMBs,
		const int x, const int y,
		const MBParam * const pParam,
		const uint32_t MotionFlags,
		const uint32_t VopFlags,
		const uint32_t VolFlags,
		const IMAGE * const pCurrent,
		const IMAGE * const pRef,
		const IMAGE * const vGMC,
		const int coding_type);

void
xvid_me_ModeDecision_Fast(SearchData * const Data,
		MACROBLOCK * const pMB,
		const MACROBLOCK * const pMBs,
		const int x, const int y,
		const MBParam * const pParam,
		const uint32_t MotionFlags,
		const uint32_t VopFlags,
		const uint32_t VolFlags,
		const IMAGE * const pCurrent,
		const IMAGE * const pRef,
		const IMAGE * const vGMC,
		const int coding_type);

void 
ModeDecision_BVOP_RD(SearchData * const Data_d,
					 SearchData * const Data_b,
					 SearchData * const Data_f,
					 SearchData * const Data_i,
					 MACROBLOCK * const pMB,
					 const MACROBLOCK * const b_mb,
					 VECTOR * f_predMV,
					 VECTOR * b_predMV,
					 const uint32_t MotionFlags,
					 const MBParam * const pParam,
					 int x, int y,
					 int best_sad);

unsigned int
getMinFcode(const int MVmax);

#endif							/* _ESTIMATION_H_ */
