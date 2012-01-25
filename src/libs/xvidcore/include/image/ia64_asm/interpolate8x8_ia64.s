// ****************************************************************************
// *
// *  XVID MPEG-4 VIDEO CODEC
// *  - IA64 halfpel interpolation -
// *
// *  Copyright(C) 2002 Kai Kühn, Alexander Viehl
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
// * $Id: interpolate8x8_ia64.s,v 1.5.14.1 2009/05/25 09:03:47 Isibaar Exp $
// *
// ***************************************************************************/
//
// ****************************************************************************
// *                                                                            
// *  interpolate8x8_ia64.s, IA-64 halfpel interpolation                                  
// *                                                                            
// *  This version was implemented during an IA-64 practical training at 	
// *  the University of Karlsruhe (http://i44w3.info.uni-karlsruhe.de/)		
// *                                                                            
// ****************************************************************************

	.file	"interpolate8x8_ia64.s"
	.pred.safe_across_calls p1-p5,p16-p63
	.text
        .align 16
        .global interpolate8x8_halfpel_h_ia64#
        .proc interpolate8x8_halfpel_h_ia64#
interpolate8x8_halfpel_h_ia64:
	LL=3
	SL=1
	SL2=1
	OL=1
	OL2=1
	AVL=1
	AL=1
      STL=3

	alloc r9=ar.pfs,4, 60,0,64

	mov r20 = ar.lc
	mov r21 = pr

	dep.z r22 = r33,3,3		// rshift of src

	and r14 = -8,r33			// align src
	mov r15 = r32			// get dest
	mov r16 = r34			// stride
//	sub r17 = 0,r0			// 1-rounding
		
	;;

	add r18 = 8,r14			
//	mux1 r17 = r17, @brcst		// broadcast 1-rounding

	sub r24 = 64,r22			// lshift of src
	add r26 = 8,r22			// rshift of src+1
	sub r27 = 56,r22			// lshift of src+1

	mov ar.lc = 7						// loopcounter
	mov ar.ec = LL + SL +OL + AVL  + STL		// sum of latencies
	mov pr.rot = 1 << 16					// init pr regs for sw-pipeling

	;;
	.rotr ald1[LL+1],ald2[LL+1],shru1[SL+1],shl1[SL+1],shru2[SL+1],shl2[SL+1],or1[OL+1],or2[OL+1+AL],avg[AVL+1]
	.rotp aldp[LL], sh1p[SL], or1p[OL], pavg1p[AVL],stp[STL]


.Lloop_interpolate:
	(aldp[0]) ld8 ald1[0] = [r14],r16		// load aligned src
	(aldp[0]) ld8 ald2[0] = [r18],r16		// and aligned src+8

	(sh1p[0]) shr.u shru1[0] = ald1[LL],r22	// get src
	(sh1p[0]) shl shl1[0] = ald2[LL],r27
      (sh1p[0]) shr.u shru2[0] = ald1[LL],r26	// get src+1
      (sh1p[0]) shl shl2[0] = ald2[LL],r24

	(or1p[0]) or or1[0] = shru1[SL],shl2[SL]		// merge things
	(or1p[0]) or or2[0] = shru2[SL],shl1[SL]

//	(addp[0]) padd1.uus add1[0] = or1[OL],r17		// add 1-rounding

	(pavg1p[0]) pavg1 avg[0] = or1[OL],or2[OL]	// parallel average

	 (stp[0]) st8 [r15] = avg[AVL]			// store results
	 (stp[0]) add r15 = r15,r16




	br.ctop.sptk.few .Lloop_interpolate
	;;
	mov ar.lc = r20
	mov pr = r21,-1
	br.ret.sptk.many b0
	.endp interpolate8x8_halfpel_h_ia64#

        .align 16
        .global interpolate8x8_halfpel_v_ia64#
        .proc interpolate8x8_halfpel_v_ia64#
interpolate8x8_halfpel_v_ia64:
	LL=3
	SL=1
	SL2=1
	OL=1
	OL2=1
	AVL=1
	AL=1
      STL=3

	alloc r9=ar.pfs,4, 60,0,64

	mov r20 = ar.lc
	mov r21 = pr

	dep.z r22 = r33,3,3

	and r14 = -8,r33
	mov r15 = r32
	mov r16 = r34
//	sub r17 = 0,r0
	;;

	add r18 = 8,r14
	add r19 = r14,r16			// src + stride 
//	mux1 r17 = r17, @brcst

	sub r24 = 64,r22
	;;	
	add r26 = 8,r19			// src + stride + 8

	mov ar.lc = 7
	mov ar.ec = LL + SL +OL + AVL + STL
	mov pr.rot = 1 << 16

	;;
	.rotr ald1[LL+1],ald2[LL+1],ald3[LL+1],ald4[LL+1],shru1[SL+1],shl1[SL+1],shru2[SL+1],shl2[SL+1],or1[OL+1],or2[OL+1+AL],avg[AVL+1]
	.rotp aldp[LL], sh1p[SL], or1p[OL], pavg1p[AVL],stp[STL]


.Lloop_interpolate2:
	(aldp[0]) ld8 ald1[0] = [r14],r16
	(aldp[0]) ld8 ald2[0] = [r18],r16
	(aldp[0]) ld8 ald3[0] = [r19],r16
	(aldp[0]) ld8 ald4[0] = [r26],r16

	(sh1p[0]) shr.u shru1[0] = ald1[LL],r22
	(sh1p[0]) shl shl1[0] = ald2[LL],r24
      (sh1p[0]) shr.u shru2[0] = ald3[LL],r22
      (sh1p[0]) shl shl2[0] = ald4[LL],r24

	(or1p[0]) or or1[0] = shru1[SL],shl1[SL]
	(or1p[0]) or or2[0] = shru2[SL],shl2[SL]

//	(addp[0]) padd1.uus add1[0] = or1[OL],r17

	(pavg1p[0]) pavg1 avg[0] = or1[OL],or2[OL]

	 (stp[0]) st8 [r15] = avg[AVL]
	 (stp[0]) add r15 = r15,r16




	br.ctop.sptk.few .Lloop_interpolate2
	;;
	mov ar.lc = r20
	mov pr = r21,-1
	br.ret.sptk.many b0
	.endp interpolate8x8_halfpel_v_ia64#

        .align 16
        .global interpolate8x8_halfpel_hv_ia64#
        .proc interpolate8x8_halfpel_hv_ia64#
interpolate8x8_halfpel_hv_ia64:
	LL=3
	SL=1
	SL2=1
	OL=1
	OL2=1
	AVL=1
	AL=1
      STL=3

	alloc r9=ar.pfs,4, 60,0,64

	mov r20 = ar.lc
	mov r21 = pr

	dep.z r22 = r33,3,3

	and r14 = -8,r33
	mov r15 = r32
	mov r16 = r34
//	sub r17 = 0,r0
	;;

	add r18 = 8,r14
	add r19 = r14,r16
//	mux1 r17 = r17, @brcst

	add r27 = 8,r22
	sub r28 = 56,r22
	sub r24 = 64,r22
	;;
	add r26 = 8,r19

	mov ar.lc = 7
	mov ar.ec = LL + SL +OL + 2*AVL  + STL
	mov pr.rot = 1 << 16

	;;
	.rotr ald1[LL+1],ald2[LL+1],ald3[LL+1],ald4[LL+1],shru1[SL+1],shl1[SL+1],shru2[SL+1],shl2[SL+1],shl3[SL+1],shru3[SL+1],shl4[SL+1],shru4[SL+1],or1[OL+1],or2[OL+1+AL],or3[OL+AL+1],or4[OL+AL+1],avg[AVL+1],avg1[AVL+1],avg2[AVL+1]
	.rotp aldp[LL], sh1p[SL], or1p[OL],pavg1p[AVL],pavg2p[AVL],stp[STL]


.Lloop_interpolate3:
	(aldp[0]) ld8 ald1[0] = [r14],r16
	(aldp[0]) ld8 ald2[0] = [r18],r16
	(aldp[0]) ld8 ald3[0] = [r19],r16
	(aldp[0]) ld8 ald4[0] = [r26],r16

	(sh1p[0]) shr.u shru1[0] = ald1[LL],r22
	(sh1p[0]) shl shl1[0] = ald2[LL],r24
      (sh1p[0]) shr.u shru2[0] = ald3[LL],r22
      (sh1p[0]) shl shl2[0] = ald4[LL],r24
	(sh1p[0]) shr.u shru3[0] = ald1[LL],r27
	(sh1p[0]) shl shl3[0] = ald2[LL],r28
	(sh1p[0]) shr.u shru4[0] = ald3[LL],r27
	(sh1p[0]) shl shl4[0] = ald4[LL],r28
	

	(or1p[0]) or or1[0] = shru1[SL],shl1[SL]
	(or1p[0]) or or2[0] = shru2[SL],shl2[SL]
	(or1p[0]) or or3[0] = shru3[SL],shl3[SL]
	(or1p[0]) or or4[0] = shru4[SL],shl4[SL]

//	(addp[0]) padd1.uus add1[0] = or1[OL],r17

	(pavg1p[0]) pavg1 avg[0] = or1[OL],or2[OL]
	(pavg1p[0]) pavg1 avg1[0] = or3[OL],or4[OL]
	
	(pavg2p[0]) pavg1 avg2[0] = avg[AVL],avg1[AVL]

	 (stp[0]) st8 [r15] = avg2[AVL]
	 (stp[0]) add r15 = r15,r16




	br.ctop.sptk.few .Lloop_interpolate3
	;;
	mov ar.lc = r20
	mov pr = r21,-1
	br.ret.sptk.many b0
	.endp interpolate8x8_halfpel_hv_ia64#


