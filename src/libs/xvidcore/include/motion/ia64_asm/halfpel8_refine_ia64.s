// ****************************************************************************
// *
// *  XVID MPEG-4 VIDEO CODEC
// *  - IA64 halfpel refinement -
// *
// *  Copyright(C) 2002 Johannes Singler, Daniel Winkler
// *
// *  This program is free software; you can redistribute it and/or modify it
// *  under the terms of the GNU General Public License as published by
// *  the Free Software Foundation; either version 2 of the License, or
// *  (at your option) any later version.
// *
// *  This program is distributed in the hope that it will be useful,
// *  but WITHOUT ANY WARRANTY; without even the implied warranty of
// *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// *  GNU General Public License for more details.
// *
// *  You should have received a copy of the GNU General Public License
// *  along with this program; if not, write to the Free Software
// *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
// *
// * $Id: halfpel8_refine_ia64.s,v 1.3.14.1 2009/05/25 09:03:47 Isibaar Exp $
// *
// ***************************************************************************/
//
// ****************************************************************************
// *                                                                            
// *  halfpel8_refine_ia64.s, IA-64 halfpel refinement                                  
// *                                                                            
// *  This version was implemented during an IA-64 practical training at 	
// *  the University of Karlsruhe (http://i44w3.info.uni-karlsruhe.de/)		
// *                                                                            
// ****************************************************************************

//   ------------------------------------------------------------------------------
//   * Programmed by
//   * Johannes Singler (email@jsingler.de), Daniel Winkler (infostudent@uni.de)
//   *
//   * Programmed for the IA64 laboratory held at University Karlsruhe 2002
//   * http://www.info.uni-karlsruhe.de/~rubino/ia64p/
//   *
//   ------------------------------------------------------------------------------
//   *
//   * This is the optimized assembler version of Halfpel8_Refine. This function 
//   * is worth it to be optimized for the IA-64 architecture because of the huge 
//   * register set. We can hold all necessary data in general use registers
//   * and reuse it.
//   *	
//   * Our approach uses:
//   *   - The Itanium command psad1, which solves the problem in hardware. 
//   *   - Alignment resolving to avoid memory faults
//   *   - Massive lopp unrolling
//   *
//   ------------------------------------------------------------------------------
//   *
//   *    -------	Half-pixel steps around the center (*) and corresponding 
//   *    |0|1|0|       register set parts.
//   *    -------
//   *    |2|*|2|
//   *    -------
//   *    |0|1|0|
//   *    -------
//   *
//   ------------------------------------------------------------------------------
//   * calc_delta is split up in three parts wich are included from
//   *
//   * calc_delta_1.s
//   * calc_delta_2.s
//   * calc_delta_3.s
//   *
//   ------------------------------------------------------------------------------
//   * We assume    min_dx <= currX <= max_dx     &&     min_dy <= currY <= max_dy

	
.sdata
	.align 4
	.type	 lambda_vec8#,@object
	.size	 lambda_vec8#,128
lambda_vec8:
	data4	0
	data4	1
	data4	1
	data4	1
	data4	1
	data4	2
	data4	2
	data4	2
	data4	2
	data4	3
	data4	3
	data4	3
	data4	4
	data4	4
	data4	4
	data4	5
	data4	5
	data4	6
	data4	7
	data4	7
	data4	8
	data4	9
	data4	10
	data4	11
	data4	13
	data4	14
	data4	16
	data4	18
	data4	21
	data4	25
	data4	30
	data4	36


	.type	 mvtab#,@object
	.size	 mvtab#,132
mvtab:
	data4	1
	data4	2
	data4	3
	data4	4
	data4	6
	data4	7
	data4	7
	data4	7
	data4	9
	data4	9
	data4	9
	data4	10
	data4	10
	data4	10
	data4	10
	data4	10
	data4	10
	data4	10
	data4	10
	data4	10
	data4	10
	data4	10
	data4	10
	data4	10
	data4	10
	data4	11
	data4	11
	data4	11
	data4	11
	data4	11
	data4	11
	data4	12
	data4	12
.text
	.align 16
	.global Halfpel8_Refine_ia64#
	.proc Halfpel8_Refine_ia64#

Halfpel8_Refine_ia64:

	pfs = r14
	prsave = r15

	// Save important registers
	
	alloc pfs = ar.pfs, 18, 74, 4, 96
	mov prsave = pr

	// Naming registers for better readability
	
	pRef = in0
	pRefH = in1
	pRefV = in2
	pRefHV = in3
	cura = in4
	x = in5
	y = in6
	currMV = in7
	iMinSAD = in8
	dx = in9
	dy = in10
	min_dx = in11
	max_dx = in12
	min_dy = in13
	max_dy = in14
	iFcode = in15
	iQuant = in16
	iEdgedWidth = in17

	iSAD = r17
	backupX = r18
	backupY = r19
	currX = r20
	currY = r21
	currYAddress = r22
	bitX0 = r23
	bitY0 = r24
	dxd2 = r25
	dyd2 = r26
	offset = r27
	block = r28 
	nob02 = r29
	nob1 = r30
	nob64m02 = r31
	nob64m1 = r127
	const7 = r126
	nob56m02 = r125
	oldX = r124
	oldY = r123

	.rotr	inregisters[18], refaa[3], refab[3], cur[8], ref0a[9], ref0b[9], ref1a[9], mpr[9], ref2a[8], ref2b[8], component[2], sc[2], tabaddress[2]

	fx = f8
	fy = f9
	fblock = f10
	fiEdgedWidth = f11
	fdxd2 = f12
	fdyd2 = f13
	foffset = f14
	fydiEdgedWidth = f15
	fQuant = f32
	fmv = f33

	n = p16
	h = p17
	v = p18
	hv = p19
	l = p20
	r = p21
	t = p22
	b = p23
	lt = p24
	lb = p25
	rt = p26
	rb = p27
	fb = p28
	non0_0 = p30
	non0_1 = p31
	non0_2 = p32
	non0_3 = p33
	neg_0 = p34
	neg_1 = p35
	neg_2 = p36
	neg_3 = p37
	cg32_0 = p29
	cg32_1 = p38

	// Initialize input variables

	add sp = 16, sp
	;; 
	ld4 iMinSAD = [sp], 8
	;;
	sxt4 iMinSAD = iMinSAD

	
	ld4 dx = [sp], 8
	;; 
	sxt4 dx = dx
	
	ld4 dy = [sp], 8
	;;
	sxt4 dy = dy
	
	ld4 min_dx = [sp], 8
	;; 
	sxt4 min_dx = min_dx

	ld4 max_dx = [sp], 8
	;; 
	sxt4 max_dx = max_dx

	ld4 min_dy = [sp], 8
	;; 
	sxt4 min_dy = min_dy

	ld4 max_dy = [sp], 8
	;; 
	sxt4 max_dy = max_dy

	ld4 iFcode = [sp], 8
	;;
	sxt4 iFcode = iFcode

	ld4 iQuant = [sp], 8

	add tabaddress[0] = @gprel(lambda_vec8#), gp
	;;
	shladd tabaddress[0] = iQuant, 2, tabaddress[0]
	;;
	ld4 iQuant = [tabaddress[0]]
	;;
	sxt4 iQuant = iQuant
	;;
	add iFcode = -1, iFcode		//only used in decreased version
	shl iQuant = iQuant, 1
	;; 
	setf.sig fQuant = iQuant
	
	ld4 iEdgedWidth = [sp]
	add sp = -88, sp
	 


	
	// Initialize local variables

	
	ld4 currX = [currMV]
	add currYAddress = 4, currMV
	;;
	sxt4 currX = currX
	ld4 currY = [currYAddress]
	;; 
	sxt4 currY = currY
	;; 
	// Calculate references
	
	cmp.gt l, p0 = currX, min_dx
	cmp.lt r, p0 = currX, max_dx
	cmp.gt t, p0 = currY, min_dy
	cmp.lt b, p0 = currY, max_dy
	add backupX = -1, currX			//move to left upper corner of quadrate
	add backupY = -1, currY

	;; 
(b)	cmp.gt.unc lb, p0 = currX, min_dx
(t)	cmp.lt.unc rt, p0 = currX, max_dx
(l)	cmp.gt.unc lt, p0 = currY, min_dy
(r)	cmp.lt.unc rb, p0 = currY, max_dy
	 
	and bitX0 = 1, backupX
	and bitY0 = 1, backupY
	;;
	cmp.eq n, p0 = 0, bitX0
	cmp.eq h, p0 = 1, bitX0
	cmp.eq v, p0 = 0, bitX0
	cmp.eq hv, p0 = 1, bitX0
	;; 
	cmp.eq.and n, p0 = 0, bitY0
	cmp.eq.and h, p0 = 0, bitY0
	cmp.eq.and v, p0 = 1, bitY0
	cmp.eq.and hv, p0 = 1, bitY0
	;;

	.pred.rel "mutex", p16, p17, p18, p19	//n, h, v, hv 
(n)	mov refaa[0] = pRef
(h)	mov refaa[0] = pRefH
(v)	mov refaa[0] = pRefV
(hv)	mov refaa[0] = pRefHV

(n)	mov refaa[1] = pRefH
(h)	mov refaa[1] = pRef
(v)	mov refaa[1] = pRefHV
(hv)	mov refaa[1] = pRefV

(n)	mov refaa[2] = pRefV
(h)	mov refaa[2] = pRefHV
(v)	mov refaa[2] = pRef
(hv)	mov refaa[2] = pRefH
	

	// Calculate offset (integer multiplication on IA-64 sucks!)

	mov block = 8
	 
	shr dxd2 = backupX, 1
	shr dyd2 = backupY, 1

	setf.sig fx = x
	setf.sig fy = y
	;; 
	setf.sig fblock = block
	setf.sig fiEdgedWidth = iEdgedWidth
	;; 
	setf.sig fdxd2 = dxd2
	setf.sig fdyd2 = dyd2
	;; 
	xma.l foffset = fx, fblock, fdxd2
	xma.l fydiEdgedWidth = fy, fblock, fdyd2
	;; 
	xma.l foffset = fydiEdgedWidth, fiEdgedWidth, foffset
	;; 
	getf.sig offset = foffset
	;;
	add refaa[0] = refaa[0], offset
	add refaa[1] = refaa[1], offset
	add refaa[2] = refaa[2], offset
	;; 
(h)	add refaa[1] = 1, refaa[1] 
(hv)	add refaa[1] = 1, refaa[1]
(v)	add refaa[2] = iEdgedWidth, refaa[2]
(hv)	add refaa[2] = iEdgedWidth, refaa[2]
	
	// Load respecting misalignment of refx...

	mov const7 = 7
	;; 
	dep.z nob02 = refaa[0], 3, 3
	dep.z nob1 = refaa[1], 3, 3
	;; 
	andcm refaa[0] = refaa[0], const7	// set last 3 bits = 0
	andcm refaa[1] = refaa[1], const7
	andcm refaa[2] = refaa[2], const7
	;;
	add refab[0] = 8, refaa[0]
	add refab[1] = 8, refaa[1]
	add refab[2] = 8, refaa[2]
	;;
	ld8 cur[0] = [cura], iEdgedWidth
	ld8 ref0a[0] = [refaa[0]], iEdgedWidth
	sub nob64m02 = 64, nob02		// 64 - nob

	ld8 ref0b[0] = [refab[0]], iEdgedWidth
	ld8 ref1a[0] = [refaa[1]], iEdgedWidth
	sub nob56m02 = 56, nob02		// 56 - nob

	ld8 mpr[0] = [refab[1]], iEdgedWidth
	ld8 ref2a[0] = [refaa[2]], iEdgedWidth
	sub nob64m1 = 64, nob1
	
	ld8 ref2b[0] = [refab[2]], iEdgedWidth
	;;  
	ld8 cur[1] = [cura], iEdgedWidth
	ld8 ref0a[1] = [refaa[0]], iEdgedWidth
	ld8 ref0b[1] = [refab[0]], iEdgedWidth
	ld8 ref1a[1] = [refaa[1]], iEdgedWidth
	ld8 mpr[1] = [refab[1]], iEdgedWidth
	ld8 ref2a[1] = [refaa[2]], iEdgedWidth
	ld8 ref2b[1] = [refab[2]], iEdgedWidth
	;; 
	ld8 cur[2] = [cura], iEdgedWidth
	ld8 ref0a[2] = [refaa[0]], iEdgedWidth
	ld8 ref0b[2] = [refab[0]], iEdgedWidth
	ld8 ref1a[2] = [refaa[1]], iEdgedWidth
	ld8 mpr[2] = [refab[1]], iEdgedWidth
	ld8 ref2a[2] = [refaa[2]], iEdgedWidth
	ld8 ref2b[2] = [refab[2]], iEdgedWidth
	;; 
	ld8 cur[3] = [cura], iEdgedWidth
	ld8 ref0a[3] = [refaa[0]], iEdgedWidth
	ld8 ref0b[3] = [refab[0]], iEdgedWidth
	ld8 ref1a[3] = [refaa[1]], iEdgedWidth
	ld8 mpr[3] = [refab[1]], iEdgedWidth
	ld8 ref2a[3] = [refaa[2]], iEdgedWidth
	ld8 ref2b[3] = [refab[2]], iEdgedWidth
	;; 
	ld8 cur[4] = [cura], iEdgedWidth
	ld8 ref0a[4] = [refaa[0]], iEdgedWidth
	ld8 ref0b[4] = [refab[0]], iEdgedWidth
	ld8 ref1a[4] = [refaa[1]], iEdgedWidth
	ld8 mpr[4] = [refab[1]], iEdgedWidth
	ld8 ref2a[4] = [refaa[2]], iEdgedWidth
	ld8 ref2b[4] = [refab[2]], iEdgedWidth
	;; 
	ld8 cur[5] = [cura], iEdgedWidth
	ld8 ref0a[5] = [refaa[0]], iEdgedWidth
	ld8 ref0b[5] = [refab[0]], iEdgedWidth
	ld8 ref1a[5] = [refaa[1]], iEdgedWidth
	ld8 mpr[5] = [refab[1]], iEdgedWidth
	ld8 ref2a[5] = [refaa[2]], iEdgedWidth
	ld8 ref2b[5] = [refab[2]], iEdgedWidth
	;; 
	ld8 cur[6] = [cura], iEdgedWidth
	ld8 ref0a[6] = [refaa[0]], iEdgedWidth
	ld8 ref0b[6] = [refab[0]], iEdgedWidth
	ld8 ref1a[6] = [refaa[1]], iEdgedWidth
	ld8 mpr[6] = [refab[1]], iEdgedWidth
	ld8 ref2a[6] = [refaa[2]], iEdgedWidth
	ld8 ref2b[6] = [refab[2]], iEdgedWidth
	;; 
	ld8 cur[7] = [cura]
	ld8 ref0a[7] = [refaa[0]], iEdgedWidth
	ld8 ref0b[7] = [refab[0]], iEdgedWidth
	ld8 ref1a[7] = [refaa[1]], iEdgedWidth
	ld8 mpr[7] = [refab[1]], iEdgedWidth
	ld8 ref2a[7] = [refaa[2]]
	ld8 ref2b[7] = [refab[2]]
	;; 
	ld8 ref0a[8] = [refaa[0]]
	ld8 ref0b[8] = [refab[0]]
	ld8 ref1a[8] = [refaa[1]]
	ld8 mpr[8] = [refab[1]]
	;;


	// Align ref1
	
     	shr.u ref1a[0] = ref1a[0], nob1
     	shr.u ref1a[1] = ref1a[1], nob1
     	shr.u ref1a[2] = ref1a[2], nob1
     	shr.u ref1a[3] = ref1a[3], nob1
     	shr.u ref1a[4] = ref1a[4], nob1
     	shr.u ref1a[5] = ref1a[5], nob1
     	shr.u ref1a[6] = ref1a[6], nob1
     	shr.u ref1a[7] = ref1a[7], nob1
     	shr.u ref1a[8] = ref1a[8], nob1

	shl mpr[0] = mpr[0], nob64m1
	shl mpr[1] = mpr[1], nob64m1
	shl mpr[2] = mpr[2], nob64m1
	shl mpr[3] = mpr[3], nob64m1
	shl mpr[4] = mpr[4], nob64m1
	shl mpr[5] = mpr[5], nob64m1
	shl mpr[6] = mpr[6], nob64m1
	shl mpr[7] = mpr[7], nob64m1
	shl mpr[8] = mpr[8], nob64m1
	;; 
.explicit
{.mii
	or ref1a[0] = ref1a[0], mpr[0]
     	shr.u ref0a[0] = ref0a[0], nob02
     	shr.u ref0a[1] = ref0a[1], nob02
}
{.mmi
	or ref1a[1] = ref1a[1], mpr[1]
	or ref1a[2] = ref1a[2], mpr[2]
     	shr.u ref0a[2] = ref0a[2], nob02
}
{.mii
	or ref1a[3] = ref1a[3], mpr[3]
     	shr.u ref0a[3] = ref0a[3], nob02
     	shr.u ref0a[4] = ref0a[4], nob02
}
{.mmi
	or ref1a[4] = ref1a[4], mpr[4]
	or ref1a[5] = ref1a[5], mpr[5]
	shr.u ref0a[5] = ref0a[5], nob02
}
{.mii
	or ref1a[6] = ref1a[6], mpr[6]
     	shr.u ref0a[6] = ref0a[6], nob02
     	shr.u ref0a[7] = ref0a[7], nob02
}
{.mii
	or ref1a[7] = ref1a[7], mpr[7]
	or ref1a[8] = ref1a[8], mpr[8]
     	shr.u ref0a[8] = ref0a[8], nob02
}
.default
	// ref1a[] now contains center position values
	// mpr[] not used any more
	
	// Align ref0 left
	
	;; 
	shl mpr[0] = ref0b[0], nob56m02
	shl mpr[1] = ref0b[1], nob56m02
	shl mpr[2] = ref0b[2], nob56m02
	shl mpr[3] = ref0b[3], nob56m02
	shl mpr[4] = ref0b[4], nob56m02
	shl mpr[5] = ref0b[5], nob56m02
	shl mpr[6] = ref0b[6], nob56m02
	shl mpr[7] = ref0b[7], nob56m02
	shl mpr[8] = ref0b[8], nob56m02

	shl ref0b[0] = ref0b[0], nob64m02
	shl ref0b[1] = ref0b[1], nob64m02
	shl ref0b[2] = ref0b[2], nob64m02
	shl ref0b[3] = ref0b[3], nob64m02
	shl ref0b[4] = ref0b[4], nob64m02
	shl ref0b[5] = ref0b[5], nob64m02
	shl ref0b[6] = ref0b[6], nob64m02
	shl ref0b[7] = ref0b[7], nob64m02
	shl ref0b[8] = ref0b[8], nob64m02
	;; 
	or ref0a[0] = ref0a[0], ref0b[0]
	or ref0a[1] = ref0a[1], ref0b[1]
	or ref0a[2] = ref0a[2], ref0b[2]
	or ref0a[3] = ref0a[3], ref0b[3]
	or ref0a[4] = ref0a[4], ref0b[4]
	or ref0a[5] = ref0a[5], ref0b[5]
	or ref0a[6] = ref0a[6], ref0b[6]
	or ref0a[7] = ref0a[7], ref0b[7]
	or ref0a[8] = ref0a[8], ref0b[8]
	;;

	// ref0a[] now contains left position values
	// mpr[] contains intermediate result for right position values (former ref0a << 56 - nob02)
	
	// Align ref0 right

	// Shift one byte more to the right (seen als big-endian)
	shr.u ref0b[0] = ref0a[0], 8
	shr.u ref0b[1] = ref0a[1], 8
	shr.u ref0b[2] = ref0a[2], 8
	shr.u ref0b[3] = ref0a[3], 8
	shr.u ref0b[4] = ref0a[4], 8
	shr.u ref0b[5] = ref0a[5], 8
	shr.u ref0b[6] = ref0a[6], 8
	shr.u ref0b[7] = ref0a[7], 8
	shr.u ref0b[8] = ref0a[8], 8
	;;
.explicit
{.mii
	or  ref0b[0] = ref0b[0], mpr[0]
     	shr.u ref2a[0] = ref2a[0], nob02
     	shr.u ref2a[1] = ref2a[1], nob02
}
{.mmi
	or  ref0b[1] = ref0b[1], mpr[1]
	or  ref0b[2] = ref0b[2], mpr[2]
     	shr.u ref2a[2] = ref2a[2], nob02
}
{.mii
	or  ref0b[3] = ref0b[3], mpr[3]
     	shr.u ref2a[3] = ref2a[3], nob02
     	shr.u ref2a[4] = ref2a[4], nob02
}
{.mmi
	or  ref0b[4] = ref0b[4], mpr[4]
	or  ref0b[5] = ref0b[5], mpr[5]
     	shr.u ref2a[5] = ref2a[5], nob02
}
{.mii
	or  ref0b[6] = ref0b[6], mpr[6]
     	shr.u ref2a[6] = ref2a[6], nob02
     	shr.u ref2a[7] = ref2a[7], nob02
}
.default
	or  ref0b[7] = ref0b[7], mpr[7]
	or  ref0b[8] = ref0b[8], mpr[8]
	
	// ref0b[] now contains right position values
	// mpr[] not needed any more

	
	// Align ref2 left
	
	;; 
	shl mpr[0] = ref2b[0], nob56m02
	shl mpr[1] = ref2b[1], nob56m02
	shl mpr[2] = ref2b[2], nob56m02
	shl mpr[3] = ref2b[3], nob56m02
	shl mpr[4] = ref2b[4], nob56m02
	shl mpr[5] = ref2b[5], nob56m02
	shl mpr[6] = ref2b[6], nob56m02
	shl mpr[7] = ref2b[7], nob56m02

	shl ref2b[0] = ref2b[0], nob64m02
	shl ref2b[1] = ref2b[1], nob64m02
	shl ref2b[2] = ref2b[2], nob64m02
	shl ref2b[3] = ref2b[3], nob64m02
	shl ref2b[4] = ref2b[4], nob64m02
	shl ref2b[5] = ref2b[5], nob64m02
	shl ref2b[6] = ref2b[6], nob64m02
	shl ref2b[7] = ref2b[7], nob64m02
	;; 
	or ref2a[0] = ref2a[0], ref2b[0]
	or ref2a[1] = ref2a[1], ref2b[1]
	or ref2a[2] = ref2a[2], ref2b[2]
	or ref2a[3] = ref2a[3], ref2b[3]
	or ref2a[4] = ref2a[4], ref2b[4]
	or ref2a[5] = ref2a[5], ref2b[5]
	or ref2a[6] = ref2a[6], ref2b[6]
	or ref2a[7] = ref2a[7], ref2b[7]
	;;

	// ref2a[] now contains left position values
	// mpr[] contains intermediate result for right position values (former ref2a << 56 - nob02)
	
	// Align ref2 right

	// Shift one byte more to the right (seen als big-endian)
	shr.u ref2b[0] = ref2a[0], 8
	shr.u ref2b[1] = ref2a[1], 8
	shr.u ref2b[2] = ref2a[2], 8
	shr.u ref2b[3] = ref2a[3], 8
	shr.u ref2b[4] = ref2a[4], 8
	shr.u ref2b[5] = ref2a[5], 8
	shr.u ref2b[6] = ref2a[6], 8
	shr.u ref2b[7] = ref2a[7], 8
	;; 
	or  ref2b[0] = ref2b[0], mpr[0]
	or  ref2b[1] = ref2b[1], mpr[1]
	or  ref2b[2] = ref2b[2], mpr[2]
	or  ref2b[3] = ref2b[3], mpr[3]
	or  ref2b[4] = ref2b[4], mpr[4]
	or  ref2b[5] = ref2b[5], mpr[5]
	or  ref2b[6] = ref2b[6], mpr[6]
	or  ref2b[7] = ref2b[7], mpr[7]
	

	// ref2b[] now contains right position values
	// mpr[] not needed any more


		
	// Let's SAD

	// Left top corner
	

	sub dx = backupX, dx
	psad1 mpr[0] = cur[0], ref0a[0]
	psad1 mpr[1] = cur[1], ref0a[1]

	sub dy = backupY, dy
	psad1 mpr[2] = cur[2], ref0a[2]
	psad1 mpr[3] = cur[3], ref0a[3]
	psad1 mpr[4] = cur[4], ref0a[4]
	psad1 mpr[5] = cur[5], ref0a[5]
	psad1 mpr[6] = cur[6], ref0a[6]
	psad1 mpr[7] = cur[7], ref0a[7]
	;; 
.include "../../src/motion/ia64_asm/calc_delta_1.s"
	
	// Top edge

	psad1 mpr[0] = cur[0], ref1a[0]
	psad1 mpr[1] = cur[1], ref1a[1]
	psad1 mpr[2] = cur[2], ref1a[2]
	psad1 mpr[3] = cur[3], ref1a[3]
	psad1 mpr[4] = cur[4], ref1a[4]

	add dx = 1, dx
	psad1 mpr[5] = cur[5], ref1a[5]
	psad1 mpr[6] = cur[6], ref1a[6]

	psad1 mpr[7] = cur[7], ref1a[7]
	;;

.include "../../src/motion/ia64_asm/calc_delta_2.s"
(lt)	cmp.lt.unc fb, p0 = mpr[8], iMinSAD
.include "../../src/motion/ia64_asm/calc_delta_3.s"
	
	// Right top corner


	psad1 mpr[0] = cur[0], ref0b[0]
	psad1 mpr[1] = cur[1], ref0b[1]
	psad1 mpr[2] = cur[2], ref0b[2]
	psad1 mpr[3] = cur[3], ref0b[3]
	psad1 mpr[4] = cur[4], ref0b[4]
	
	add backupX = 1, backupX
	psad1 mpr[5] = cur[5], ref0b[5]
	psad1 mpr[6] = cur[6], ref0b[6]

	add dx = 1, dx
	psad1 mpr[7] = cur[7], ref0b[7]
	;;
	
.include "../../src/motion/ia64_asm/calc_delta_1.s"
(t)	cmp.lt.unc fb, p0 = iSAD, iMinSAD
	;; 
	
	// Left edge

(fb)	mov iMinSAD = iSAD
	psad1 mpr[0] = cur[0], ref2a[0]

(fb)	mov currX = backupX
	psad1 mpr[1] = cur[1], ref2a[1]
	psad1 mpr[2] = cur[2], ref2a[2]

(fb)	mov currY = backupY
	psad1 mpr[3] = cur[3], ref2a[3]
	psad1 mpr[4] = cur[4], ref2a[4]

	add backupX = 1, backupX
	psad1 mpr[5] = cur[5], ref2a[5]
	psad1 mpr[6] = cur[6], ref2a[6]

	psad1 mpr[7] = cur[7], ref2a[7]

	add dx = -2, dx
	add dy = 1, dy
	;;
	
.include "../../src/motion/ia64_asm/calc_delta_2.s"
(rt)	cmp.lt.unc fb, p0 = mpr[8], iMinSAD
.include "../../src/motion/ia64_asm/calc_delta_3.s"
	
	// Right edge

	
	psad1 mpr[0] = cur[0], ref2b[0]
	psad1 mpr[1] = cur[1], ref2b[1]
	psad1 mpr[2] = cur[2], ref2b[2]
	psad1 mpr[3] = cur[3], ref2b[3]
	psad1 mpr[4] = cur[4], ref2b[4]

	add backupX = -2, backupX
	psad1 mpr[5] = cur[5], ref2b[5]
	psad1 mpr[6] = cur[6], ref2b[6]

	add backupY = 1, backupY
	add dx = 2, dx
	psad1 mpr[7] = cur[7], ref2b[7]
	;;
	
.include "../../src/motion/ia64_asm/calc_delta_1.s"
(l)	cmp.lt.unc fb, p0 = iSAD, iMinSAD
	;;
	
	// Left bottom corner

(fb)	mov iMinSAD = iSAD
	psad1 mpr[0] = cur[0], ref0a[1]

(fb)	mov currX = backupX
	psad1 mpr[1] = cur[1], ref0a[2]
	psad1 mpr[2] = cur[2], ref0a[3]

(fb)	mov currY = backupY
	psad1 mpr[3] = cur[3], ref0a[4]
	psad1 mpr[4] = cur[4], ref0a[5]

	add backupX = 2, backupX
	psad1 mpr[5] = cur[5], ref0a[6]
	psad1 mpr[6] = cur[6], ref0a[7]

	psad1 mpr[7] = cur[7], ref0a[8]

	add dx = -2, dx
	add dy = 1, dy
	;;
	
.include "../../src/motion/ia64_asm/calc_delta_2.s"
(r)	cmp.lt.unc fb, p0 = mpr[8], iMinSAD
.include "../../src/motion/ia64_asm/calc_delta_3.s"
	
	// Bottom edge
	
	psad1 mpr[0] = cur[0], ref1a[1]
	psad1 mpr[1] = cur[1], ref1a[2]
	psad1 mpr[2] = cur[2], ref1a[3]
	psad1 mpr[3] = cur[3], ref1a[4]
	psad1 mpr[4] = cur[4], ref1a[5]

	add backupX = -2, backupX
	psad1 mpr[5] = cur[5], ref1a[6]
	psad1 mpr[6] = cur[6], ref1a[7]

	add backupY = 1, backupY
	add dx = 1, dx
	psad1 mpr[7] = cur[7], ref1a[8]
	;;
	
.include "../../src/motion/ia64_asm/calc_delta_1.s"
(lb)	cmp.lt.unc fb, p0 = iSAD, iMinSAD
	;; 
	// Right bottom corner


(fb)	mov iMinSAD = iSAD
	psad1 mpr[0] = cur[0], ref0b[1]

(fb)	mov currX = backupX
	psad1 mpr[1] = cur[1], ref0b[2]
	psad1 mpr[2] = cur[2], ref0b[3]

(fb)	mov currY = backupY
	psad1 mpr[3] = cur[3], ref0b[4]
	psad1 mpr[4] = cur[4], ref0b[5]
	
	add backupX = 1, backupX
	psad1 mpr[5] = cur[5], ref0b[6]
	psad1 mpr[6] = cur[6], ref0b[7]

	add dx = 1, dx
	psad1 mpr[7] = cur[7], ref0b[8]
	;;
	
.include "../../src/motion/ia64_asm/calc_delta_2.s"
(b)	cmp.lt.unc fb, p0 = mpr[8], iMinSAD
.include "../../src/motion/ia64_asm/calc_delta_3.s"

(rb)	getf.sig ret0 = fmv
	add backupX = 1, backupX
	;; 
(rb)	add iSAD = iSAD, ret0
	;; 
(rb)	cmp.lt.unc fb, p0 = iSAD, iMinSAD
	;; 
(fb)	mov iMinSAD = iSAD
(fb)	mov currX = backupX
(fb)	mov currY = backupY
	;; 
	
	// Write back result
 
	st4 [currMV] = currX
	st4 [currYAddress] = currY
	mov ret0 = iMinSAD

	// Restore important registers

	;;
	mov pr = prsave, -1
	mov ar.pfs = pfs	
	br.ret.sptk.many b0
	
	.endp Halfpel8_Refine_ia64#
