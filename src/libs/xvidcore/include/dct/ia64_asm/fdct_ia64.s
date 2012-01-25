// ****************************************************************************
// *
// *  XVID MPEG-4 VIDEO CODEC
// *  - IA64 forward discrete cosine transform -
// *
// *  Copyright(C) 2002 Stephan Krause, Ingo-Marc Weber, Daniel Kallfass
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
// * $Id: fdct_ia64.s,v 1.5.10.1 2009/05/25 09:03:47 Isibaar Exp $
// *
// ***************************************************************************/
//
// ****************************************************************************
// *                                                                            
// *  fdct_ia64.s, IA-64 optimized forward DCT                                  
// *                                                                            
// *  Completed version provided by Intel at AppNote AP-922    			        
// *  http://developer.intel.com/software/products/college/ia32/strmsimd/	
// *  Copyright (C) 1999 Intel Corporation,                                     
// *                                                                            
// *  This version was implemented during an IA-64 practical training at 	
// *  the University of Karlsruhe (http://i44w3.info.uni-karlsruhe.de/)		
// *                                                                            
// *****************************************************************************
//
// *****************************************************************************
// *                                                                            
// *  Revision history:                                                         
// *                                                                            
// *  24.07.2002 Initial Version						
// *                                                                            
// *****************************************************************************


// This is a fast precise implementation of 8x8 Discrete Cosine Transform
// published in Intel Application Note 922 from 1999 and optimized for IA-64.
//
// An unoptimized "straight forward" version can be found at the end of this file.


	.pred.safe_across_calls p1-p5,p16-p63
.text
	.align 16
	.global fdct_ia64#
	.proc fdct_ia64#
fdct_ia64:
	.prologue
	alloc r14 = ar.pfs, 1, 56, 0, 0
			// Save constants
	mov r31 = 0x32ec		// c0 = tan(1pi/16)
	mov r30 = 0x6a0a		// c1 = tan(2pi/16)
	mov r29 = 0xab0e		// c2 = tan(3pi/16)
	mov r28 = 0xb505		// g4 = cos(4pi/16)
	mov r27 = 0xd4db		// g3 = cos(3pi/16)
	mov r26 = 0xec83		// g2 = cos(2pi/16)
	mov r25 = 0xfb15		// g1 = cos(1pi/16)
	mov r24 = 0x0002		// correction bit for descaling
	mov r23 = 0x0004		// correction bit for descaling

		// Load Matrix into registers
	
	add loc0 = r0, r32
	add loc2 = 16, r32
	add loc4 = 32, r32
	add loc6 = 48, r32
	add loc8 = 64, r32
	add loc10 = 80, r32
	add loc12 = 96, r32
	add loc14 = 112, r32
	add loc1 = 8, r32
	add loc3 = 24, r32
	add loc5 = 40, r32
	add loc7 = 56, r32
	add loc9 = 72, r32
	add loc11 = 88, r32
	add loc13 = 104, r32
	add loc15 = 120, r32
	;;
	ld8 loc16 = [loc0]
	ld8 loc17 = [loc2]
	ld8 loc18 = [loc4]
	ld8 loc19 = [loc6]
	ld8 loc20 = [loc8]
	ld8 loc21 = [loc10]
	ld8 loc22 = [loc12]
	ld8 loc23 = [loc14]
	ld8 loc24 = [loc1]
	ld8 loc25 = [loc3]
	ld8 loc26 = [loc5]
	ld8 loc27 = [loc7]
	mux2 r26 = r26, 0x00
	ld8 loc28 = [loc9]
	mux2 r31 = r31, 0x00
	mux2 r25 = r25, 0x00
	ld8 loc29 = [loc11]
	mux2 r30 = r30, 0x00
	mux2 r29 = r29, 0x00
	ld8 loc30 = [loc13]
	mux2 r28 = r28, 0x00
	mux2 r27 = r27, 0x00
	ld8 loc31 = [loc15]
	mux2 r24 = r24, 0x00
	mux2 r23 = r23, 0x00
	;;
	pshl2 loc16 = loc16, 3
	pshl2 loc17 = loc17, 3
	pshl2 loc18 = loc18, 3
	pshl2 loc19 = loc19, 3
	pshl2 loc20 = loc20, 3
	pshl2 loc21 = loc21, 3
	pshl2 loc22 = loc22, 3
	pshl2 loc23 = loc23, 3
	;;
	pshl2 loc24 = loc24, 3
		
		// *******************
		// column-DTC 1st half
		// *******************
		
	psub2 loc37 = loc17, loc22	// t5 = x1 - x6
	pshl2 loc25 = loc25, 3
	pshl2 loc26 = loc26, 3
	psub2 loc38 = loc18, loc21	// t6 = x2 - x5
	pshl2 loc27 = loc27, 3
	pshl2 loc28 = loc28, 3
	;;
	padd2 loc32 = loc16, loc23	// t0 = x0 + x7
	pshl2 loc29 = loc29, 3
	pshl2 loc30 = loc30, 3
	padd2 loc33 = loc17, loc22	// t1 = x1 + x6
	padd2 loc40 = loc37, loc38	// buf0 = t5 + t6
	psub2 loc41 = loc37, loc38	// buf1 = t5 - t6
	;;
	padd2 loc34 = loc18, loc21	// t2 = x2 + x5
	pshl2 loc31 = loc31, 3
	padd2 loc35 = loc19, loc20	// t3 = x3 + x4
	psub2 loc36 = loc16, loc23	// t4 = x0 - x7
	pmpyshr2 loc37 = loc40, r28, 16	// t5 = buf0 * g4
	pmpyshr2 loc38 = loc41, r28, 16	// t6 = buf1 * g4
	;;
	psub2 loc39 = loc19, loc20	// t7 = x3 - x4
	padd2 loc37 = loc37, loc40	// t5 = t5 + buf1
	padd2 loc38 = loc38, loc41	// t6 = t6 + buf2

	padd2 loc16 = loc32, loc35	// x0 = t0 + t3
	padd2 loc17 = loc33, loc34	// x1 = t1 + t2
	psub2 loc18 = loc32, loc35	// x2 = t0 - t3
	;;
	psub2 loc19 = loc33, loc34	// x3 = t1 - t2
	padd2 loc20 = loc36, loc37	// x4 = t4 + t5
	padd2 loc21 = loc38, loc39	// x5 = t6 + t7
	psub2 loc22 = loc36, loc37	// x6 = t4 - t5
	psub2 loc23 = loc38, loc39	// x7 = t6 - t7
	;;
	pmpyshr2 loc42 = loc18, r30, 16	// buf2 = x2 * c1
	padd2 loc32 = loc16, loc17	// t0 = x0 + x1
	pmpyshr2 loc43 = loc19, r30, 16	// buf3 = x3 * c1
	pmpyshr2 loc44 = loc20, r31, 16	// buf4 = x4 * c0
	psub2 loc33 = loc16, loc17	// t1 = x0 - x1
	pmpyshr2 loc46 = loc22, r29, 16	// buf6 = x6 * c2
	pmpyshr2 loc47 = loc23, r29, 16	// buf7 = x7 * c2
	;;
	padd2 loc46 = loc46, loc22	// buf6 = buf6 + x6
	pmpyshr2 loc45 = loc21, r31, 16	// buf5 = x5 * c0
	padd2 loc47 = loc47, loc23	// buf7 = buf7 + x7
	padd2 loc34 = loc18, loc43	// t2 = x2 + (x3 * c1)
	psub2 loc35 = loc42, loc19	// t3 = (c1 * x2) - x3
	psub2 loc37 = loc44, loc21	// t5 = (c1 * x4) - x5
	;;
	padd2 loc36 = loc20, loc45	// t4 = x4 + (x5 * c1)
	padd2 loc38 = loc22, loc47	// t6 = x6 + (x7 * c1)
	pmpyshr2 loc16 = loc32, r28, 16	// x0 = t0 * g4
	pmpyshr2 loc17 = loc33, r28, 16	// x1 = t1 * g4
	psub2 loc39 = loc46, loc23	// t7 = (c1 * x6) - x7
	;;
	padd2 loc48 = loc16, loc32	// y0 = x0 + t0
	pmpyshr2 loc18 = loc34, r26, 16	// x2 = t2 * g2
	pmpyshr2 loc19 = loc35, r26, 16	// x3 = t3 * g2
	padd2 loc52 = loc17, loc33	// y4 = x1 + t1
	pmpyshr2 loc20 = loc36, r25, 16	// x4 = t4 * g1
	pmpyshr2 loc21 = loc37, r25, 16	// x5 = t5 * g1
	;;
	padd2 loc50 = loc18, loc34	// y2 = x2 + t2
	pmpyshr2 loc22 = loc38, r27, 16	// x6 = t6 * g3
	pmpyshr2 loc23 = loc39, r27, 16	// x7 = t7 * g3
	padd2 loc55 = loc21, loc37	// y7 = x5 + t5
	padd2 loc49 = loc20, loc36	// y1 = x4 + t4
	padd2 loc54 = loc19, loc35	// y6 = x3 + t3
	;;
	padd2 loc51 = loc22, loc38	// y3 = x6 + t6
	padd2 loc53 = loc23, loc39	// y5 = x7 + t7
	
		//divide by 4
		
	padd2 loc48 = loc48, r24
	padd2 loc49 = loc49, r24
	padd2 loc50 = loc50, r24
	padd2 loc52 = loc52, r24
	;;
	padd2 loc51 = loc51, r24
	pshr2 loc48 = loc48, 2
	padd2 loc53 = loc53, r24
	pshr2 loc49 = loc49, 2
	padd2 loc54 = loc54, r24
	pshr2 loc50 = loc50, 2
	padd2 loc55 = loc55, r24
	pshr2 loc52 = loc52, 2
	;;
	pshr2 loc51 = loc51, 2
	pshr2 loc53 = loc53, 2
	pshr2 loc54 = loc54, 2
	pshr2 loc55 = loc55, 2


		// *******************
		// column-DTC 2nd half
		// *******************

	psub2 loc37 = loc25, loc30	// t5 = x1.2 - x6.2
	psub2 loc38 = loc26, loc29	// t6 = x2.2 - x5.2
	padd2 loc32 = loc24, loc31	// t0 = x0.2 + x7.2
	padd2 loc33 = loc25, loc30	// t1 = x1.2 + x6.2
	;;
	padd2 loc34 = loc26, loc29	// t2 = x2.2 + x5.2
	psub2 loc41 = loc37, loc38	// buf1 = t5 - t6
	padd2 loc40 = loc37, loc38	// buf0 = t5 + t6
	padd2 loc35 = loc27, loc28	// t3 = x3.2 + x4.2
	;;
	psub2 loc36 = loc24, loc31	// t4 = x0.2 - x7.2
	pmpyshr2 loc37 = loc40, r28, 16	// t5 = buf0 * g4
	pmpyshr2 loc38 = loc41, r28, 16	// t6 = buf1 * g4
	;;
	psub2 loc39 = loc27, loc28	// t7 = x3.2 - x4.2
	padd2 loc37 = loc37, loc40	// t5 = t5 + buf1
	padd2 loc38 = loc38, loc41	// t6 = t6 + buf2

	padd2 loc16 = loc32, loc35	// x0 = t0 + t3
	padd2 loc17 = loc33, loc34	// x1 = t1 + t2
	psub2 loc18 = loc32, loc35	// x2 = t0 - t3
	;;
	psub2 loc19 = loc33, loc34	// x3 = t1 - t2
	padd2 loc20 = loc36, loc37	// x4 = t4 + t5
	padd2 loc21 = loc38, loc39	// x5 = t6 + t7
	psub2 loc22 = loc36, loc37	// x6 = t4 - t5
	psub2 loc23 = loc38, loc39	// x7 = t6 - t7
	;;
	pmpyshr2 loc42 = loc18, r30, 16	// buf2 = x2 * c1
	padd2 loc32 = loc16, loc17	// t0 = x0 + x1
	pmpyshr2 loc43 = loc19, r30, 16	// buf3 = x3 * c1
	pmpyshr2 loc44 = loc20, r31, 16	// buf4 = x4 * c0
	psub2 loc33 = loc16, loc17	// t1 = x0 - x1
	pmpyshr2 loc45 = loc21, r31, 16	// buf5 = x5 * c0
	pmpyshr2 loc46 = loc22, r29, 16	// buf6 = x6 * c2
	pmpyshr2 loc47 = loc23, r29, 16	// buf7 = x7 * c2
	;;
	padd2 loc34 = loc18, loc43	// t2 = x2 + buf3
	padd2 loc46 = loc46, loc22	// buf6 = buf6 + x6
	padd2 loc47 = loc47, loc23	// buf7 = buf7 + x7
	psub2 loc35 = loc42, loc19	// t3 = buf2 - x3
	padd2 loc36 = loc20, loc45	// t4 = x4 + buf5
	pmpyshr2 loc16 = loc32, r28, 16	// x0 = t0 * g4
	;;
	psub2 loc37 = loc44, loc21	// t5 = buf4 - x5
	padd2 loc38 = loc22, loc47	// t6 = x6 + buf7
	psub2 loc39 = loc46, loc23	// t7 = buf6 - x7
	pmpyshr2 loc17 = loc33, r28, 16	// x1 = t1 * g4
	;;
	padd2 loc40 = loc16, loc32	// y0.2 = x0 + t0
	pmpyshr2 loc18 = loc34, r26, 16	// x2 = t2 * g2
	pmpyshr2 loc19 = loc35, r26, 16	// x3 = t3 * g2
	padd2 loc44 = loc17, loc33	// y4.2 = x1 + t1
	pmpyshr2 loc20 = loc36, r25, 16	// x4 = t4 * g1
	pmpyshr2 loc21 = loc37, r25, 16	// x5 = t5 * g1
	;;
	padd2 loc42 = loc18, loc34	// y2.2 = x2 + t2
	pmpyshr2 loc22 = loc38, r27, 16	// x6 = t6 * g3
	pmpyshr2 loc23 = loc39, r27, 16	// x7 = t7 * g3
	padd2 loc47 = loc21, loc37	// y7.2 = x5 + t5
	padd2 loc41 = loc20, loc36	// y1.2 = x4 + t4
	padd2 loc46 = loc19, loc35	// y6.2 = x3 + t3
	;;
	padd2 loc43 = loc22, loc38	// y3.2 = x6 + t6

		// *******************
		//  transpose matrix
		// *******************	
	
	mix2.r loc32 = loc48, loc49	// tmp0 = mixr y0, y1
	mix2.l loc33 = loc48, loc49	// tmp1 = mixl y0, y1
	padd2 loc45 = loc23, loc39	// y5.2 = x7 + t7
	mix2.r loc34 = loc50, loc51	// tmp2 = mixr y2, y3
	mix2.l loc35 = loc50, loc51	// tmp3 = mixl y2, y3
	;;

		//divide by 4 	
		
	padd2 loc40 = loc40, r24
	padd2 loc41 = loc41, r24
	mix4.r loc16 = loc32, loc34	// x0 = mixr tmp0, tmp2
	padd2 loc42 = loc42, r24
	padd2 loc43 = loc43, r24
	mix4.r loc17 = loc33, loc35	// x1 = mixr tmp1, tmp3
	padd2 loc44 = loc44, r24
	padd2 loc45 = loc45, r24
	mix4.l loc18 = loc32, loc34	// x2 = mixl tmp0, tmp2
	padd2 loc46 = loc46, r24
	padd2 loc47 = loc47, r24
	mix4.l loc19 = loc33, loc35	// x3 = mixl tmp1, tmp3
	;;
	pshr2 loc40 = loc40, 2
	pshr2 loc41 = loc41, 2
	pshr2 loc42 = loc42, 2
	pshr2 loc43 = loc43, 2
	mix2.r loc32 = loc52, loc53	// tmp0 = mixr y4, y5
	mix2.l loc33 = loc52, loc53	// tmp1 = mixl y4, y5
	mix2.r loc34 = loc54, loc55	// tmp2 = mixr y6, y7
	mix2.l loc35 = loc54, loc55	// tmp3 = mixl y6, y7
	;;
	pshr2 loc44 = loc44, 2
	pshr2 loc45 = loc45, 2
	pshr2 loc46 = loc46, 2
	pshr2 loc47 = loc47, 2
	mix4.r loc24 = loc32, loc34	// x0.2 = mixr tmp0, tmp2
	mix4.r loc25 = loc33, loc35	// x1.2 = mixr tmp1, tmp3
	mix4.l loc26 = loc32, loc34	// x2.2 = mixl tmp0, tmp2
	mix4.l loc27 = loc33, loc35	// x3.2 = mixl tmp1, tmp3
	;;
	mix2.r loc32 = loc40, loc41	// tmp0 = mixr y0.2, y1.2
	mix2.l loc33 = loc40, loc41	// tmp1 = mixl y0.2, y1.2
	mix2.r loc34 = loc42, loc43	// tmp2 = mixr y2.2, y3.2
	mix2.l loc35 = loc42, loc43	// tmp3 = mixl y2.2, y3.2
	;;
	mix4.r loc20 = loc32, loc34	// x4 = mixr tmp0, tmp2
	mix4.r loc21 = loc33, loc35	// x5 = mixr tmp1, tmp3
	mix4.l loc22 = loc32, loc34	// x6 = mixl tmp0, tmp2
	mix4.l loc23 = loc33, loc35	// x7 = mixl tmp1, tmp3
	;;
	mix2.r loc32 = loc44, loc45	// tmp0 = mixr y4.2, y5.2
	mix2.l loc33 = loc44, loc45	// tmp1 = mixl y4.2, y5.2
	mix2.r loc34 = loc46, loc47	// tmp2 = mixr y6.2, y6.2
	mix2.l loc35 = loc46, loc47	// tmp3 = mixl y6.2, y6.2
	;;
	mix4.r loc28 = loc32, loc34	// x4.2 = mixr tmp0, tmp2
	mix4.r loc29 = loc33, loc35	// x5.2 = mixr tmp1, tmp3
	mix4.l loc30 = loc32, loc34	// x6.2 = mixl tmp0, tmp2
	mix4.l loc31 = loc33, loc35	// x7.2 = mixl tmp1, tmp3
		
		// *******************
		// row-DTC 1st half
		// *******************
	
	psub2 loc37 = loc17, loc22	// t5 = x1 - x6
	psub2 loc38 = loc18, loc21	// t6 = x2 - x5
	;;
	padd2 loc32 = loc16, loc23	// t0 = x0 + x7
	padd2 loc33 = loc17, loc22	// t1 = x1 + x6
	padd2 loc34 = loc18, loc21	// t2 = x2 + x5
	psub2 loc41 = loc37, loc38	// buf1 = t5 - t6
	padd2 loc40 = loc37, loc38	// buf0 = t5 + t6
	padd2 loc35 = loc19, loc20	// t3 = x3 + x4
	;;
	psub2 loc36 = loc16, loc23	// t4 = x0 - x7
	pmpyshr2 loc37 = loc40, r28, 16	// t5 = buf0 * g4
	pmpyshr2 loc38 = loc41, r28, 16	// t6 = buf1 * g4
	;;
	psub2 loc39 = loc19, loc20	// t7 = x3 - x4
	padd2 loc37 = loc37, loc40	// t5 = t5 + buf1
	padd2 loc38 = loc38, loc41	// t6 = t6 + buf2

	padd2 loc16 = loc32, loc35	// x0 = t0 + t3
	padd2 loc17 = loc33, loc34	// x1 = t1 + t2
	psub2 loc18 = loc32, loc35	// x2 = t0 - t3
	;;
	psub2 loc19 = loc33, loc34	// x3 = t1 - t2
	padd2 loc20 = loc36, loc37	// x4 = t4 + t5
	padd2 loc21 = loc38, loc39	// x5 = t6 + t7
	psub2 loc22 = loc36, loc37	// x6 = t4 - t5
	psub2 loc23 = loc38, loc39	// x7 = t6 - t7
	;;
	pmpyshr2 loc42 = loc18, r30, 16	// buf2 = x2 * c1
	padd2 loc32 = loc16, loc17	// t0 = x0 + x1
	pmpyshr2 loc43 = loc19, r30, 16	// buf3 = x3 * c1
	pmpyshr2 loc44 = loc20, r31, 16	// buf4 = x4 * c0
	psub2 loc33 = loc16, loc17	// t1 = x0 - x1
	pmpyshr2 loc45 = loc21, r31, 16	// buf5 = x5 * c0
	pmpyshr2 loc46 = loc22, r29, 16	// buf6 = x6 * c2
	pmpyshr2 loc47 = loc23, r29, 16	// buf7 = x7 * c2
	;;
	padd2 loc46 = loc46, loc22	// buf6 = buf6 + x6
	padd2 loc47 = loc47, loc23	// buf7 = buf7 + x7
	padd2 loc34 = loc18, loc43	// t2 = x2 + (x3 * c1)
	;;
	psub2 loc35 = loc42, loc19	// t3 = (c1 * x2) - x3
	padd2 loc36 = loc20, loc45	// t4 = x4 + (x5 * c1)
	psub2 loc37 = loc44, loc21	// t5 = (c1 * x4) - x5
	padd2 loc38 = loc22, loc47	// t6 = x6 + (buf7 * c1)
	psub2 loc39 = loc46, loc23	// t7 = (c1 * buf6) - x7
	pmpyshr2 loc16 = loc32, r28, 16	// x0 = t0 * g4
	pmpyshr2 loc17 = loc33, r28, 16	// x1 = t1 * g4
	;;
	padd2 loc48 = loc16, loc32	// y0 = x0 + t0
	pmpyshr2 loc18 = loc34, r26, 16	// x2 = t2 * g2
	pmpyshr2 loc19 = loc35, r26, 16	// x3 = t3 * g2
	padd2 loc52 = loc17, loc33	// y4 = x1 + t1
	pmpyshr2 loc20 = loc36, r25, 16	// x4 = t4 * g1
	pmpyshr2 loc21 = loc37, r25, 16	// x5 = t5 * g1
	;;
	padd2 loc50 = loc18, loc34	// y2 = x2 + t2
	pmpyshr2 loc22 = loc38, r27, 16	// x6 = t6 * g3
	pmpyshr2 loc23 = loc39, r27, 16	// x7 = t7 * g3
	padd2 loc55 = loc21, loc37	// y7 = x5 + t5
	padd2 loc49 = loc20, loc36	// y1 = x4 + t4
	padd2 loc54 = loc19, loc35	// y6 = x3 + t3
	;;
	padd2 loc51 = loc22, loc38	// y3 = x6 + t6
	padd2 loc53 = loc23, loc39	// y5 = x7 + t7
	
		// *******************
		// row-DTC 2nd half
		// *******************

	psub2 loc37 = loc25, loc30	// t5 = x1.2 - x6.2
	psub2 loc38 = loc26, loc29	// t6 = x2.2 - x5.2
	padd2 loc32 = loc24, loc31	// t0 = x0.2 + x7.2
	padd2 loc33 = loc25, loc30	// t1 = x1.2 + x6.2
	;;
	padd2 loc34 = loc26, loc29	// t2 = x2.2 + x5.2
	psub2 loc41 = loc37, loc38	// buf1 = t5 - t6
	padd2 loc40 = loc37, loc38	// buf0 = t5 + t6
	padd2 loc35 = loc27, loc28	// t3 = x3.2 + x4.2
	;;
	psub2 loc36 = loc24, loc31	// t4 = x0.2 - x7.2
	pmpyshr2 loc37 = loc40, r28, 16	// t5 = buf0 * g4
	pmpyshr2 loc38 = loc41, r28, 16	// t6 = buf1 * g4
	;;
	psub2 loc39 = loc27, loc28	// t7 = x3.2 - x4.2
	padd2 loc37 = loc37, loc40	// t5 = t5 + buf1
	padd2 loc38 = loc38, loc41	// t6 = t6 + buf2

	padd2 loc16 = loc32, loc35	// x0 = t0 + t3
	padd2 loc17 = loc33, loc34	// x1 = t1 + t2
	psub2 loc18 = loc32, loc35	// x2 = t0 - t3
	;;
	psub2 loc19 = loc33, loc34	// x3 = t1 - t2
	padd2 loc20 = loc36, loc37	// x4 = t4 + t5
	padd2 loc21 = loc38, loc39	// x5 = t6 + t7
	psub2 loc22 = loc36, loc37	// x6 = t4 - t5
	psub2 loc23 = loc38, loc39	// x7 = t6 - t7
	;;
	pmpyshr2 loc42 = loc18, r30, 16	// buf2 = x2 * c1
	padd2 loc32 = loc16, loc17	// t0 = x0 + x1
	pmpyshr2 loc43 = loc19, r30, 16	// buf3 = x3 * c1
	pmpyshr2 loc44 = loc20, r31, 16	// buf4 = x4 * c0
	psub2 loc33 = loc16, loc17	// t1 = x0 - x1
	pmpyshr2 loc45 = loc21, r31, 16	// buf5 = x5 * c0
	pmpyshr2 loc46 = loc22, r29, 16	// buf6 = x6 * c2
	pmpyshr2 loc47 = loc23, r29, 16	// buf7 = x7 * c2
	;;
	padd2 loc46 = loc46, loc22	// buf6 = buf6 + x6
	padd2 loc47 = loc47, loc23	// buf7 = buf7 + x7
	padd2 loc34 = loc18, loc43	// t2 = x2 + (x3 * c1)
	;;
	psub2 loc35 = loc42, loc19	// t3 = (c1 * x2) - x3
	padd2 loc36 = loc20, loc45	// t4 = x4 + (x5 * c1)
	psub2 loc37 = loc44, loc21	// t5 = (c1 * x4) - x5
	padd2 loc38 = loc22, loc47	// t6 = x6 + (buf7 * c1)
	psub2 loc39 = loc46, loc23	// t7 = (c1 * buf6) - x7
	pmpyshr2 loc16 = loc32, r28, 16	// x0 = t0 * g4
	pmpyshr2 loc17 = loc33, r28, 16	// x1 = t1 * g4
	;;
	padd2 loc40 = loc16, loc32	// y0.2 = x0 + t0
	pmpyshr2 loc18 = loc34, r26, 16	// x2 = t2 * g2
	pmpyshr2 loc19 = loc35, r26, 16	// x3 = t3 * g2
	padd2 loc44 = loc17, loc33	// y4.2 = x1 + t1
	pmpyshr2 loc20 = loc36, r25, 16	// x4 = t4 * g1
	pmpyshr2 loc21 = loc37, r25, 16	// x5 = t5 * g1
	;;
	padd2 loc42 = loc18, loc34	// y2.2 = x2 + t2
	pmpyshr2 loc22 = loc38, r27, 16	// x6 = t6 * g3
	pmpyshr2 loc23 = loc39, r27, 16	// x7 = t7 * g3
	padd2 loc46 = loc19, loc35	// y6.2 = x3 + t3
	nop.i 0x0
	nop.i 0x0
	;;
	
		// *******************
		// Transpose matrix
		// *******************
	padd2 loc41 = loc20, loc36	// y1.2 = x4 + t4
	mix2.l loc32 = loc49, loc48	// tmp0 = mixr y1, y0
	mix2.r loc33 = loc49, loc48	// tmp1 = mixl y1, y0
	padd2 loc47 = loc21, loc37	// y7.2 = x5 + t5
	mix2.l loc34 = loc51, loc50	// tmp2 = mixr y3, y2
	mix2.r loc35 = loc51, loc50	// tmp3 = mixl y3, y2
	;;
	padd2 loc43 = loc22, loc38	// y3.2 = x6 + t6
	mix4.l loc16 = loc34, loc32	// x0 = mixr tmp2, tmp0
	mix4.l loc17 = loc35, loc33	// x1 = mixr tmp3, tmp1
	padd2 loc45 = loc23, loc39	// y5.2 = x7 + t7
	mix4.r loc18 = loc34, loc32	// x2 = mixl tmp2, tmp0
	mix4.r loc19 = loc35, loc33	// x3 = mixl tmp3, tmp1
	;;
	padd2 loc16 = loc16, r23
	mix2.l loc32 = loc41, loc40	// tmp0 = mixr y0.2, y1.2
	mix2.r loc33 = loc41, loc40	// tmp1 = mixl y0.2, y1.2
	padd2 loc17 = loc17, r23
	mix2.l loc34 = loc43, loc42	// tmp2 = mixr y2.2, y3.2
	mix2.r loc35 = loc43, loc42	// tmp3 = mixl y2.2, y3.2
	;;
	padd2 loc18 = loc18, r23
	mix4.l loc20 = loc34, loc32	// x4 = mixr tmp2, tmp0
	mix4.l loc21 = loc35, loc33	// x5 = mixr tmp3, tmp1
	padd2 loc19 = loc19, r23
	mix4.r loc22 = loc34, loc32	// x6 = mixl tmp2, tmp0
	mix4.r loc23 = loc35, loc33	// x7 = mixl tmp3, tmp1
	;;
	padd2 loc20 = loc20, r23
	mix2.l loc32 = loc53, loc52	// tmp0 = mixr y5, y4
	mix2.r loc33 = loc53, loc52	// tmp1 = mixl y5, y4
	padd2 loc21 = loc21, r23
	mix2.l loc34 = loc55, loc54	// tmp2 = mixr y7, y6
	mix2.r loc35 = loc55, loc54	// tmp3 = mixl y7, y6
	;;
	padd2 loc22 = loc22, r23
	mix4.l loc24 = loc34, loc32	// x0.2 = mixr tmp2, tmp0
	mix4.l loc25 = loc35, loc33	// x1.2 = mixr tmp3, tmp1
	padd2 loc23 = loc23, r23
	mix4.r loc26 = loc34, loc32	// x2.2 = mixl tmp2, tmp0
	mix4.r loc27 = loc35, loc33	// x3.2 = mixl tmp3, tmp1
	;;
	padd2 loc24 = loc24, r23
	mix2.l loc32 = loc45, loc44	// tmp0 = mixr y4.2, y5.2
	mix2.r loc33 = loc45, loc44	// tmp1 = mixl y4.2, y5.2
	padd2 loc25 = loc25, r23
	mix2.l loc34 = loc47, loc46	// tmp2 = mixr y6.2, y6.2
	mix2.r loc35 = loc47, loc46	// tmp3 = mixl y6.2, y6.2
	;;
	padd2 loc26 = loc26, r23
	mix4.l loc28 = loc34, loc32	// x4.2 = mixr tmp2, tmp0
	mix4.l loc29 = loc35, loc33	// x5.2 = mixr tmp3, tmp1
	padd2 loc27 = loc27, r23
	mix4.r loc30 = loc34, loc32	// x6.2 = mixl tmp2, tmp0
	mix4.r loc31 = loc35, loc33	// x7.2 = mixl tmp3, tmp1
	;;
		// *******************
		// Descale 
		// *******************
	padd2 loc28 = loc28, r23
	pshr2 loc16 = loc16, 3
	pshr2 loc17 = loc17, 3
	padd2 loc29 = loc29, r23
	pshr2 loc18 = loc18, 3
	pshr2 loc19 = loc19, 3
	padd2 loc30 = loc30, r23
	pshr2 loc20 = loc20, 3
	pshr2 loc21 = loc21, 3
	padd2 loc31 = loc31, r23
	pshr2 loc22 = loc22, 3
	pshr2 loc23 = loc23, 3
	;;
	pshr2 loc24 = loc24, 3
	pshr2 loc25 = loc25, 3
	pshr2 loc26 = loc26, 3
	pshr2 loc27 = loc27, 3
	pshr2 loc28 = loc28, 3
	pshr2 loc29 = loc29, 3
	pshr2 loc30 = loc30, 3
	pshr2 loc31 = loc31, 3
	;;
		// *******************
		// Store matrix
		// *******************
	st8 [loc0] = loc16
	st8 [loc1] = loc24
	st8 [loc2] = loc17
	st8 [loc3] = loc25
	st8 [loc4] = loc18
	st8 [loc5] = loc26
	st8 [loc6] = loc19
	st8 [loc7] = loc27
	st8 [loc8] = loc20
	st8 [loc9] = loc28
	st8 [loc10] = loc21
	st8 [loc11] = loc29
	st8 [loc12] = loc22
	st8 [loc13] = loc30
	st8 [loc14] = loc23
	st8 [loc15] = loc31

	mov ar.pfs = r14
	br.ret.sptk.many b0
	.endp fdct_ia64#
	.common	fdct#,8,8








//***********************************************
//* Here is a version of the DCT implementation	*
//* unoptimized in terms of command ordering.	*
//* This version is about 30% slower but	*
//* easier understand.				*
//***********************************************
//
//	.pred.safe_across_calls p1-p5,p16-p63
//.text
//	.align 16
//	.global fdct_ia64#
//	.proc fdct_ia64#
//fdct_ia64:
//	.prologue
//	alloc r14 = ar.pfs, 1, 56, 0, 0
//
//		// *******************
//		// Save constants
//		// *******************
//	mov r31 = 0x32ec		// c0 = tan(1pi/16)
//	mov r30 = 0x6a0a		// c1 = tan(2pi/16)
//	mov r29 = 0xab0e		// c2 = tan(3pi/16)
//	mov r28 = 0xb505		// g4 = cos(4pi/16)
//	mov r27 = 0xd4db		// g3 = cos(3pi/16)
//	mov r26 = 0xec83		// g2 = cos(2pi/16)
//	mov r25 = 0xfb15		// g1 = cos(1pi/16)
//	mov r24 = 0x0002		// correction bit for descaling
//	mov r23 = 0x0004		// correction bit for descaling
//
//		// **************************
//		// Load Matrix into registers
//		// **************************
//	
//	add loc0 = r0, r32
//	;;
//	mux2 r31 = r31, 0x00
//	mux2 r30 = r30, 0x00
//	mux2 r29 = r29, 0x00
//	mux2 r28 = r28, 0x00
//	mux2 r27 = r27, 0x00
//	mux2 r26 = r26, 0x00
//	mux2 r25 = r25, 0x00
//	mux2 r24 = r24, 0x00
//	mux2 r23 = r23, 0x00
//	ld8 loc16 = [loc0]
//	add loc2 = 16, r32
//	add loc4 = 32, r32
//	add loc6 = 48, r32
//	add loc8 = 64, r32
//	add loc10 = 80, r32
//	;;
//	ld8 loc17 = [loc2]
//	ld8 loc18 = [loc4]
//	add loc12 = 96, r32
//	ld8 loc19 = [loc6]
//	ld8 loc20 = [loc8]
//	add loc14 = 112, r32
//	;;
//	ld8 loc21 = [loc10]
//	ld8 loc22 = [loc12]
//	add loc1 = 8, r32
//	ld8 loc23 = [loc14]
//	add loc3 = 24, r32
//	add loc5 = 40, r32
//	;;
//	ld8 loc24 = [loc1]
//	ld8 loc25 = [loc3]
//	add loc7 = 56, r32
//	ld8 loc26 = [loc5]
//	add loc9 = 72, r32
//	add loc11 = 88, r32
//	;;
//	ld8 loc27 = [loc7]
//	ld8 loc28 = [loc9]
//	add loc13 = 104, r32
//	ld8 loc29 = [loc11]
//	add loc15 = 120, r32
//	;;
//	ld8 loc30 = [loc13]
//	ld8 loc31 = [loc15]
//	;;
//		// ******
//		// Scale
//		// ******
//	pshl2 loc16 = loc16, 3
//	pshl2 loc17 = loc17, 3
//	pshl2 loc18 = loc18, 3
//	pshl2 loc19 = loc19, 3
//	pshl2 loc20 = loc20, 3
//	pshl2 loc21 = loc21, 3
//	pshl2 loc22 = loc22, 3
//	pshl2 loc23 = loc23, 3
//	pshl2 loc24 = loc24, 3
//	pshl2 loc25 = loc25, 3
//	pshl2 loc26 = loc26, 3
//	pshl2 loc27 = loc27, 3
//	pshl2 loc28 = loc28, 3
//	pshl2 loc29 = loc29, 3
//	pshl2 loc30 = loc30, 3
//	pshl2 loc31 = loc31, 3
//	;;
//	
//		// *******************
//		// column-DTC 1st half
//		// *******************
//	
//	padd2 loc32 = loc16, loc23	// t0 = x0 + x7
//	padd2 loc33 = loc17, loc22	// t1 = x1 + x6
//	padd2 loc34 = loc18, loc21	// t2 = x2 + x5
//	padd2 loc35 = loc19, loc20	// t3 = x3 + x4
//	psub2 loc36 = loc16, loc23	// t4 = x0 - x7
//	psub2 loc37 = loc17, loc22	// t5 = x1 - x6
//	psub2 loc38 = loc18, loc21	// t6 = x2 - x5
//	psub2 loc39 = loc19, loc20	// t7 = x3 - x4
//	;;
//	padd2 loc40 = loc37, loc38	// buf0 = t5 + t6
//	psub2 loc41 = loc37, loc38	// buf1 = t5 - t6
//	;;
//	pmpyshr2 loc37 = loc40, r28, 16	// t5 = buf0 * g4
//	pmpyshr2 loc38 = loc41, r28, 16	// t6 = buf1 * g4
//	;;
//	padd2 loc37 = loc37, loc40	// t5 = t5 + buf1
//	padd2 loc38 = loc38, loc41	// t6 = t6 + buf2
//	;;
//	padd2 loc16 = loc32, loc35	// x0 = t0 + t3
//	padd2 loc17 = loc33, loc34	// x1 = t1 + t2
//	psub2 loc18 = loc32, loc35	// x2 = t0 - t3
//	psub2 loc19 = loc33, loc34	// x3 = t1 - t2
//	padd2 loc20 = loc36, loc37	// x4 = t4 + t5
//	padd2 loc21 = loc38, loc39	// x5 = t6 + t7
//	psub2 loc22 = loc36, loc37	// x6 = t4 - t5
//	psub2 loc23 = loc38, loc39	// x7 = t6 - t7
//	;;
//
//	padd2 loc32 = loc16, loc17	// t0 = x0 + x1
//	psub2 loc33 = loc16, loc17	// t1 = x0 - x1
//	pmpyshr2 loc42 = loc18, r30, 16	// buf2 = x2 * c1
//	pmpyshr2 loc43 = loc19, r30, 16	// buf3 = x3 * c1
//	pmpyshr2 loc44 = loc20, r31, 16	// buf4 = x4 * c0
//	pmpyshr2 loc45 = loc21, r31, 16	// buf5 = x5 * c0
//	pmpyshr2 loc46 = loc22, r29, 16	// buf6 = x6 * c2
//	pmpyshr2 loc47 = loc23, r29, 16	// buf7 = x7 * c2
//	;;
//	padd2 loc46 = loc46, loc22	// buf6 = buf6 + x6
//	padd2 loc47 = loc47, loc23	// buf7 = buf7 + x7
//	;;
//	padd2 loc34 = loc18, loc43	// t2 = x2 + (x3 * c1)
//	psub2 loc35 = loc42, loc19	// t3 = (c1 * x2) - x3
//	padd2 loc36 = loc20, loc45	// t4 = x4 + (x5 * c1)
//	psub2 loc37 = loc44, loc21	// t5 = (c1 * x4) - x5
//	padd2 loc38 = loc22, loc47	// t6 = x6 + (x7 * c1)
//	psub2 loc39 = loc46, loc23	// t7 = (c1 * x6) - x7
//	;;
//	pmpyshr2 loc16 = loc32, r28, 16	// x0 = t0 * g4
//	pmpyshr2 loc17 = loc33, r28, 16	// x1 = t1 * g4
//	pmpyshr2 loc18 = loc34, r26, 16	// x2 = t2 * g2
//	pmpyshr2 loc19 = loc35, r26, 16	// x3 = t3 * g2
//	pmpyshr2 loc20 = loc36, r25, 16	// x4 = t4 * g1
//	pmpyshr2 loc21 = loc37, r25, 16	// x5 = t5 * g1
//	pmpyshr2 loc22 = loc38, r27, 16	// x6 = t6 * g3
//	pmpyshr2 loc23 = loc39, r27, 16	// x7 = t7 * g3
//	;;
//	padd2 loc48 = loc16, loc32	// y0 = x0 + t0
//	padd2 loc49 = loc20, loc36	// y1 = x4 + t4
//	padd2 loc50 = loc18, loc34	// y2 = x2 + t2
//	padd2 loc51 = loc22, loc38	// y3 = x6 + t6
//	padd2 loc52 = loc17, loc33	// y4 = x1 + t1
//	padd2 loc53 = loc23, loc39	// y5 = x7 + t7
//	padd2 loc54 = loc19, loc35	// y6 = x3 + t3
//	padd2 loc55 = loc21, loc37	// y7 = x5 + t5
//	;;
//	
//		// *******************
//		// column-DTC 2nd half
//		// *******************
//	
//	padd2 loc32 = loc24, loc31	// t0 = x0.2 + x7.2
//	padd2 loc33 = loc25, loc30	// t1 = x1.2 + x6.2
//	padd2 loc34 = loc26, loc29	// t2 = x2.2 + x5.2
//	padd2 loc35 = loc27, loc28	// t3 = x3.2 + x4.2
//	psub2 loc36 = loc24, loc31	// t4 = x0.2 - x7.2
//	psub2 loc37 = loc25, loc30	// t5 = x1.2 - x6.2
//	psub2 loc38 = loc26, loc29	// t6 = x2.2 - x5.2
//	psub2 loc39 = loc27, loc28	// t7 = x3.2 - x4.2
//	;;
//	padd2 loc40 = loc37, loc38	// buf0 = t5 + t6
//	psub2 loc41 = loc37, loc38	// buf1 = t5 - t6
//	;;
//	pmpyshr2 loc37 = loc40, r28, 16	// t5 = buf0 * g4
//	pmpyshr2 loc38 = loc41, r28, 16	// t6 = buf1 * g4
//	;;
//	padd2 loc37 = loc37, loc40	// t5 = t5 + buf1
//	padd2 loc38 = loc38, loc41	// t6 = t6 + buf2
//	;;
//	padd2 loc16 = loc32, loc35	// x0 = t0 + t3
//	padd2 loc17 = loc33, loc34	// x1 = t1 + t2
//	psub2 loc18 = loc32, loc35	// x2 = t0 - t3
//	psub2 loc19 = loc33, loc34	// x3 = t1 - t2
//	padd2 loc20 = loc36, loc37	// x4 = t4 + t5
//	padd2 loc21 = loc38, loc39	// x5 = t6 + t7
//	psub2 loc22 = loc36, loc37	// x6 = t4 - t5
//	psub2 loc23 = loc38, loc39	// x7 = t6 - t7
//	;;
//	padd2 loc32 = loc16, loc17	// t0 = x0 + x1
//	psub2 loc33 = loc16, loc17	// t1 = x0 - x1
//	pmpyshr2 loc42 = loc18, r30, 16	// buf2 = x2 * c1
//	pmpyshr2 loc43 = loc19, r30, 16	// buf3 = x3 * c1
//	pmpyshr2 loc44 = loc20, r31, 16	// buf4 = x4 * c0
//	pmpyshr2 loc45 = loc21, r31, 16	// buf5 = x5 * c0
//	pmpyshr2 loc46 = loc22, r29, 16	// buf6 = x6 * c2
//	pmpyshr2 loc47 = loc23, r29, 16	// buf7 = x7 * c2
//	;;
//	padd2 loc46 = loc46, loc22	// buf6 = buf6 + x6
//	padd2 loc47 = loc47, loc23	// buf7 = buf7 + x7
//	;;
//	padd2 loc34 = loc18, loc43	// t2 = x2 + (x3 * c1)
//	psub2 loc35 = loc42, loc19	// t3 = (c1 * x2) - x3
//	padd2 loc36 = loc20, loc45	// t4 = x4 + (x5 * c1)
//	psub2 loc37 = loc44, loc21	// t5 = (c1 * x4) - x5
//	padd2 loc38 = loc22, loc47	// t6 = x6 + (x7 * c1)
//	psub2 loc39 = loc46, loc23	// t7 = (c1 * x6) - x7
//	;;
//	pmpyshr2 loc16 = loc32, r28, 16	// x0 = t0 * g4
//	pmpyshr2 loc17 = loc33, r28, 16	// x1 = t1 * g4
//	pmpyshr2 loc18 = loc34, r26, 16	// x2 = t2 * g2
//	pmpyshr2 loc19 = loc35, r26, 16	// x3 = t3 * g2
//	pmpyshr2 loc20 = loc36, r25, 16	// x4 = t4 * g1
//	pmpyshr2 loc21 = loc37, r25, 16	// x5 = t5 * g1
//	pmpyshr2 loc22 = loc38, r27, 16	// x6 = t6 * g3
//	pmpyshr2 loc23 = loc39, r27, 16	// x7 = t7 * g3
//	;;
//	padd2 loc40 = loc16, loc32	// y0.2 = x0 + t0
//	padd2 loc41 = loc20, loc36	// y1.2 = x4 + t4
//	padd2 loc42 = loc18, loc34	// y2.2 = x2 + t2
//	padd2 loc43 = loc22, loc38	// y3.2 = x6 + t6
//	padd2 loc44 = loc17, loc33	// y4.2 = x1 + t1
//	padd2 loc45 = loc23, loc39	// y5.2 = x7 + t7
//	padd2 loc46 = loc19, loc35	// y6.2 = x3 + t3
//	padd2 loc47 = loc21, loc37	// y7.2 = x5 + t5
//	;;
//	padd2 loc40 = loc40, r24 // add r24 to correct rounding
//	padd2 loc41 = loc41, r24
//	padd2 loc42 = loc42, r24
//	padd2 loc43 = loc43, r24
//	padd2 loc44 = loc44, r24
//	padd2 loc45 = loc45, r24
//	padd2 loc46 = loc46, r24
//	padd2 loc47 = loc47, r24
//	padd2 loc48 = loc48, r24
//	padd2 loc49 = loc49, r24
//	padd2 loc50 = loc50, r24
//	padd2 loc51 = loc51, r24
//	padd2 loc52 = loc52, r24
//	padd2 loc53 = loc53, r24
//	padd2 loc54 = loc54, r24
//	padd2 loc55 = loc55, r24
//	;;
//	pshr2 loc40 = loc40, 2 // Divide all matrix elements through 4
//	pshr2 loc41 = loc41, 2
//	pshr2 loc42 = loc42, 2
//	pshr2 loc43 = loc43, 2
//	pshr2 loc44 = loc44, 2
//	pshr2 loc45 = loc45, 2
//	pshr2 loc46 = loc46, 2
//	pshr2 loc47 = loc47, 2
//	pshr2 loc48 = loc48, 2
//	pshr2 loc49 = loc49, 2
//	pshr2 loc50 = loc50, 2
//	pshr2 loc51 = loc51, 2
//	pshr2 loc52 = loc52, 2
//	pshr2 loc53 = loc53, 2
//	pshr2 loc54 = loc54, 2
//	pshr2 loc55 = loc55, 2
//	;;
//
//		// *****************
//		// Transpose matrix
//		// *****************
//
//	mix2.r loc32 = loc48, loc49	// tmp0 = mixr y0, y1
//	mix2.l loc33 = loc48, loc49	// tmp1 = mixl y0, y1
//	mix2.r loc34 = loc50, loc51	// tmp2 = mixr y2, y3
//	mix2.l loc35 = loc50, loc51	// tmp3 = mixl y2, y3
//	;;
//	mix4.r loc16 = loc32, loc34	// x0 = mixr tmp0, tmp2
//	mix4.r loc17 = loc33, loc35	// x1 = mixr tmp1, tmp3
//	mix4.l loc18 = loc32, loc34	// x2 = mixl tmp0, tmp2
//	mix4.l loc19 = loc33, loc35	// x3 = mixl tmp1, tmp3
//	;;
//	mix2.r loc32 = loc40, loc41	// tmp0 = mixr y0.2, y1.2
//	mix2.l loc33 = loc40, loc41	// tmp1 = mixl y0.2, y1.2
//	mix2.r loc34 = loc42, loc43	// tmp2 = mixr y2.2, y3.2
//	mix2.l loc35 = loc42, loc43	// tmp3 = mixl y2.2, y3.2
//	;;
//	mix4.r loc20 = loc32, loc34	// x4 = mixr tmp0, tmp2
//	mix4.r loc21 = loc33, loc35	// x5 = mixr tmp1, tmp3
//	mix4.l loc22 = loc32, loc34	// x6 = mixl tmp0, tmp2
//	mix4.l loc23 = loc33, loc35	// x7 = mixl tmp1, tmp3
//	;;
//	mix2.r loc32 = loc52, loc53	// tmp0 = mixr y4, y5
//	mix2.l loc33 = loc52, loc53	// tmp1 = mixl y4, y5
//	mix2.r loc34 = loc54, loc55	// tmp2 = mixr y6, y7
//	mix2.l loc35 = loc54, loc55	// tmp3 = mixl y6, y7
//	;;
//	mix4.r loc24 = loc32, loc34	// x0.2 = mixr tmp0, tmp2
//	mix4.r loc25 = loc33, loc35	// x1.2 = mixr tmp1, tmp3
//	mix4.l loc26 = loc32, loc34	// x2.2 = mixl tmp0, tmp2
//	mix4.l loc27 = loc33, loc35	// x3.2 = mixl tmp1, tmp3
//	;;
//	mix2.r loc32 = loc44, loc45	// tmp0 = mixr y4.2, y5.2
//	mix2.l loc33 = loc44, loc45	// tmp1 = mixl y4.2, y5.2
//	mix2.r loc34 = loc46, loc47	// tmp2 = mixr y6.2, y6.2
//	mix2.l loc35 = loc46, loc47	// tmp3 = mixl y6.2, y6.2
//	;;
//	mix4.r loc28 = loc32, loc34	// x4.2 = mixr tmp0, tmp2
//	mix4.r loc29 = loc33, loc35	// x5.2 = mixr tmp1, tmp3
//	mix4.l loc30 = loc32, loc34	// x6.2 = mixl tmp0, tmp2
//	mix4.l loc31 = loc33, loc35	// x7.2 = mixl tmp1, tmp3
//	;;
//		
//		// *******************
//		// row-DTC 1st half
//		// *******************
//	
//	padd2 loc32 = loc16, loc23	// t0 = x0 + x7
//	padd2 loc33 = loc17, loc22	// t1 = x1 + x6
//	padd2 loc34 = loc18, loc21	// t2 = x2 + x5
//	padd2 loc35 = loc19, loc20	// t3 = x3 + x4
//	psub2 loc36 = loc16, loc23	// t4 = x0 - x7
//	psub2 loc37 = loc17, loc22	// t5 = x1 - x6
//	psub2 loc38 = loc18, loc21	// t6 = x2 - x5
//	psub2 loc39 = loc19, loc20	// t7 = x3 - x4
//	;;
//	padd2 loc40 = loc37, loc38	// buf0 = t5 + t6
//	psub2 loc41 = loc37, loc38	// buf1 = t5 - t6
//	;;
//	pmpyshr2 loc37 = loc40, r28, 16	// t5 = buf0 * g4
//	pmpyshr2 loc38 = loc41, r28, 16	// t6 = buf1 * g4
//	;;
//	padd2 loc37 = loc37, loc40	// t5 = t5 + buf1
//	padd2 loc38 = loc38, loc41	// t6 = t6 + buf2
//	;;
//	padd2 loc16 = loc32, loc35	// x0 = t0 + t3
//	padd2 loc17 = loc33, loc34	// x1 = t1 + t2
//	psub2 loc18 = loc32, loc35	// x2 = t0 - t3
//	psub2 loc19 = loc33, loc34	// x3 = t1 - t2
//	padd2 loc20 = loc36, loc37	// x4 = t4 + t5
//	padd2 loc21 = loc38, loc39	// x5 = t6 + t7
//	psub2 loc22 = loc36, loc37	// x6 = t4 - t5
//	psub2 loc23 = loc38, loc39	// x7 = t6 - t7
//	;;
//	padd2 loc32 = loc16, loc17	// t0 = x0 + x1
//	psub2 loc33 = loc16, loc17	// t1 = x0 - x1
//	pmpyshr2 loc42 = loc18, r30, 16	// buf2 = x2 * c1
//	pmpyshr2 loc43 = loc19, r30, 16	// buf3 = x3 * c1
//	pmpyshr2 loc44 = loc20, r31, 16	// buf4 = x4 * c0
//	pmpyshr2 loc45 = loc21, r31, 16	// buf5 = x5 * c0
//	pmpyshr2 loc46 = loc22, r29, 16	// buf6 = x6 * c2
//	pmpyshr2 loc47 = loc23, r29, 16	// buf7 = x7 * c2
//	;;
//	padd2 loc46 = loc46, loc22	// buf6 = buf6 + x6
//	padd2 loc47 = loc47, loc23	// buf7 = buf7 + x7
//	;;
//	padd2 loc34 = loc18, loc43	// t2 = x2 + (x3 * c1)
//	psub2 loc35 = loc42, loc19	// t3 = (c1 * x2) - x3
//	padd2 loc36 = loc20, loc45	// t4 = x4 + (x5 * c1)
//	psub2 loc37 = loc44, loc21	// t5 = (c1 * x4) - x5
//	padd2 loc38 = loc22, loc47	// t6 = x6 + (x7 * c1)
//	psub2 loc39 = loc46, loc23	// t7 = (c1 * x6) - x7
//	;;
//	pmpyshr2 loc16 = loc32, r28, 16	// x0 = t0 * g4
//	pmpyshr2 loc17 = loc33, r28, 16	// x1 = t1 * g4
//	pmpyshr2 loc18 = loc34, r26, 16	// x2 = t2 * g2
//	pmpyshr2 loc19 = loc35, r26, 16	// x3 = t3 * g2
//	pmpyshr2 loc20 = loc36, r25, 16	// x4 = t4 * g1
//	pmpyshr2 loc21 = loc37, r25, 16	// x5 = t5 * g1
//	pmpyshr2 loc22 = loc38, r27, 16	// x6 = t6 * g3
//	pmpyshr2 loc23 = loc39, r27, 16	// x7 = t7 * g3
//	;;
//	padd2 loc48 = loc16, loc32	// y0 = x0 + t0
//	padd2 loc49 = loc20, loc36	// y1 = x4 + t4
//	padd2 loc50 = loc18, loc34	// y2 = x2 + t2
//	padd2 loc51 = loc22, loc38	// y3 = x6 + t6
//	padd2 loc52 = loc17, loc33	// y4 = x1 + t1
//	padd2 loc53 = loc23, loc39	// y5 = x7 + t7
//	padd2 loc54 = loc19, loc35	// y6 = x3 + t3
//	padd2 loc55 = loc21, loc37	// y7 = x5 + t5
//	;;
//	
//		// *******************
//		// row-DTC 2nd half
//		// *******************
//	
//	padd2 loc32 = loc24, loc31	// t0 = x0.2 + x7.2
//	padd2 loc33 = loc25, loc30	// t1 = x1.2 + x6.2
//	padd2 loc34 = loc26, loc29	// t2 = x2.2 + x5.2
//	padd2 loc35 = loc27, loc28	// t3 = x3.2 + x4.2
//	psub2 loc36 = loc24, loc31	// t4 = x0.2 - x7.2
//	psub2 loc37 = loc25, loc30	// t5 = x1.2 - x6.2
//	psub2 loc38 = loc26, loc29	// t6 = x2.2 - x5.2
//	psub2 loc39 = loc27, loc28	// t7 = x3.2 - x4.2
//	;;
//	padd2 loc40 = loc37, loc38	// buf0 = t5 + t6
//	psub2 loc41 = loc37, loc38	// buf1 = t5 - t6
//	;;
//	pmpyshr2 loc37 = loc40, r28, 16	// t5 = buf0 * g4
//	pmpyshr2 loc38 = loc41, r28, 16	// t6 = buf1 * g4
//	;;
//	padd2 loc37 = loc37, loc40	// t5 = t5 + buf1
//	padd2 loc38 = loc38, loc41	// t6 = t6 + buf2
//	;;
//	padd2 loc16 = loc32, loc35	// x0 = t0 + t3
//	padd2 loc17 = loc33, loc34	// x1 = t1 + t2
//	psub2 loc18 = loc32, loc35	// x2 = t0 - t3
//	psub2 loc19 = loc33, loc34	// x3 = t1 - t2
//	padd2 loc20 = loc36, loc37	// x4 = t4 + t5
//	padd2 loc21 = loc38, loc39	// x5 = t6 + t7
//	psub2 loc22 = loc36, loc37	// x6 = t4 - t5
//	psub2 loc23 = loc38, loc39	// x7 = t6 - t7
//	;;
//	padd2 loc32 = loc16, loc17	// t0 = x0 + x1
//	psub2 loc33 = loc16, loc17	// t1 = x0 - x1
//	pmpyshr2 loc42 = loc18, r30, 16	// buf2 = x2 * c1
//	pmpyshr2 loc43 = loc19, r30, 16	// buf3 = x3 * c1
//	pmpyshr2 loc44 = loc20, r31, 16	// buf4 = x4 * c0
//	pmpyshr2 loc45 = loc21, r31, 16	// buf5 = x5 * c0
//	pmpyshr2 loc46 = loc22, r29, 16	// buf6 = x6 * c2
//	pmpyshr2 loc47 = loc23, r29, 16	// buf7 = x7 * c2
//	;;
//	padd2 loc46 = loc46, loc22	// buf6 = buf6 + x6
//	padd2 loc47 = loc47, loc23	// buf7 = buf7 + x7
//	;;
//	padd2 loc34 = loc18, loc43	// t2 = x2 + (x3 * c1)
//	psub2 loc35 = loc42, loc19	// t3 = (c1 * x2) - x3
//	padd2 loc36 = loc20, loc45	// t4 = x4 + (x5 * c1)
//	psub2 loc37 = loc44, loc21	// t5 = (c1 * x4) - x5
//	padd2 loc38 = loc22, loc47	// t6 = x6 + (x7 * c1)
//	psub2 loc39 = loc46, loc23	// t7 = (c1 * x6) - x7
//	;;
//	pmpyshr2 loc16 = loc32, r28, 16	// x0 = t0 * g4
//	pmpyshr2 loc17 = loc33, r28, 16	// x1 = t1 * g4
//	pmpyshr2 loc18 = loc34, r26, 16	// x2 = t2 * g2
//	pmpyshr2 loc19 = loc35, r26, 16	// x3 = t3 * g2
//	pmpyshr2 loc20 = loc36, r25, 16	// x4 = t4 * g1
//	pmpyshr2 loc21 = loc37, r25, 16	// x5 = t5 * g1
//	pmpyshr2 loc22 = loc38, r27, 16	// x6 = t6 * g3
//	pmpyshr2 loc23 = loc39, r27, 16	// x7 = t7 * g3
//	;;
//	padd2 loc40 = loc16, loc32	// y0.2 = x0 + t0
//	padd2 loc41 = loc20, loc36	// y1.2 = x4 + t4
//	padd2 loc42 = loc18, loc34	// y2.2 = x2 + t2
//	padd2 loc43 = loc22, loc38	// y3.2 = x6 + t6
//	padd2 loc44 = loc17, loc33	// y4.2 = x1 + t1
//	padd2 loc45 = loc23, loc39	// y5.2 = x7 + t7
//	padd2 loc46 = loc19, loc35	// y6.2 = x3 + t3
//	padd2 loc47 = loc21, loc37	// y7.2 = x5 + t5
//	;;
//		// *******************
//		// Transpose matrix
//		// *******************
//
//	mix2.l loc32 = loc49, loc48	// tmp0 = mixr y1, y0
//	mix2.r loc33 = loc49, loc48	// tmp1 = mixl y1, y0
//	mix2.l loc34 = loc51, loc50	// tmp2 = mixr y3, y2
//	mix2.r loc35 = loc51, loc50	// tmp3 = mixl y3, y2
//	;;
//	mix4.l loc16 = loc34, loc32	// x0 = mixr tmp2, tmp0
//	mix4.l loc17 = loc35, loc33	// x1 = mixr tmp3, tmp1
//	mix4.r loc18 = loc34, loc32	// x2 = mixl tmp2, tmp0
//	mix4.r loc19 = loc35, loc33	// x3 = mixl tmp3, tmp1
//	;;
//	mix2.l loc32 = loc41, loc40	// tmp0 = mixr y0.2, y1.2
//	mix2.r loc33 = loc41, loc40	// tmp1 = mixl y0.2, y1.2
//	mix2.l loc34 = loc43, loc42	// tmp2 = mixr y2.2, y3.2
//	mix2.r loc35 = loc43, loc42	// tmp3 = mixl y2.2, y3.2
//	;;
//	mix4.l loc20 = loc34, loc32	// x4 = mixr tmp2, tmp0
//	mix4.l loc21 = loc35, loc33	// x5 = mixr tmp3, tmp1
//	mix4.r loc22 = loc34, loc32	// x6 = mixl tmp2, tmp0
//	mix4.r loc23 = loc35, loc33	// x7 = mixl tmp3, tmp1
//	;;
//	mix2.l loc32 = loc53, loc52	// tmp0 = mixr y5, y4
//	mix2.r loc33 = loc53, loc52	// tmp1 = mixl y5, y4
//	mix2.l loc34 = loc55, loc54	// tmp2 = mixr y7, y6
//	mix2.r loc35 = loc55, loc54	// tmp3 = mixl y7, y6
//	;;
//	mix4.l loc24 = loc34, loc32	// x0.2 = mixr tmp2, tmp0
//	mix4.l loc25 = loc35, loc33	// x1.2 = mixr tmp3, tmp1
//	mix4.r loc26 = loc34, loc32	// x2.2 = mixl tmp2, tmp0
//	mix4.r loc27 = loc35, loc33	// x3.2 = mixl tmp3, tmp1
//	;;
//	mix2.l loc32 = loc45, loc44	// tmp0 = mixr y4.2, y5.2
//	mix2.r loc33 = loc45, loc44	// tmp1 = mixl y4.2, y5.2
//	mix2.l loc34 = loc47, loc46	// tmp2 = mixr y6.2, y6.2
//	mix2.r loc35 = loc47, loc46	// tmp3 = mixl y6.2, y6.2
//	;;
//	mix4.l loc28 = loc34, loc32	// x4.2 = mixr tmp2, tmp0
//	mix4.l loc29 = loc35, loc33	// x5.2 = mixr tmp3, tmp1
//	mix4.r loc30 = loc34, loc32	// x6.2 = mixl tmp2, tmp0
//	mix4.r loc31 = loc35, loc33	// x7.2 = mixl tmp3, tmp1
//	;;
//
//		// ********
//		// descale
//		// ********
//
//	padd2 loc16 = loc16, r23
//	padd2 loc17 = loc17, r23
//	padd2 loc18 = loc18, r23
//	padd2 loc19 = loc19, r23
//	padd2 loc20 = loc20, r23
//	padd2 loc21 = loc21, r23
//	padd2 loc22 = loc22, r23
//	padd2 loc23 = loc23, r23
//	padd2 loc24 = loc24, r23
//	padd2 loc25 = loc25, r23
//	padd2 loc26 = loc26, r23
//	padd2 loc27 = loc27, r23
//	padd2 loc28 = loc28, r23
//	padd2 loc29 = loc29, r23
//	padd2 loc30 = loc30, r23
//	padd2 loc31 = loc31, r23
//	;;
//	pshr2 loc16 = loc16, 3
//	pshr2 loc17 = loc17, 3
//	pshr2 loc18 = loc18, 3
//	pshr2 loc19 = loc19, 3
//	pshr2 loc20 = loc20, 3
//	pshr2 loc21 = loc21, 3
//	pshr2 loc22 = loc22, 3
//	pshr2 loc23 = loc23, 3
//	pshr2 loc24 = loc24, 3
//	pshr2 loc25 = loc25, 3
//	pshr2 loc26 = loc26, 3
//	pshr2 loc27 = loc27, 3
//	pshr2 loc28 = loc28, 3
//	pshr2 loc29 = loc29, 3
//	pshr2 loc30 = loc30, 3
//	pshr2 loc31 = loc31, 3
//	;;
//		// ************
//		// Store Matrix
//		// ************
//	st8 [loc0] = loc16
//	st8 [loc1] = loc24
//	st8 [loc2] = loc17
//	st8 [loc3] = loc25
//	st8 [loc4] = loc18
//	st8 [loc5] = loc26
//	st8 [loc6] = loc19
//	st8 [loc7] = loc27
//	st8 [loc8] = loc20
//	st8 [loc9] = loc28
//	st8 [loc10] = loc21
//	st8 [loc11] = loc29
//	st8 [loc12] = loc22
//	st8 [loc13] = loc30
//	st8 [loc14] = loc23
//	st8 [loc15] = loc31
//
//	mov ar.pfs = r14
//	br.ret.sptk.many b0
//	.endp fdct_ia64#
//	.common	fdct#,8,8
//
