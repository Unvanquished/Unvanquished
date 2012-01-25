// ****************************************************************************
// *
// *  XVID MPEG-4 VIDEO CODEC
// *  - IA64 sum of absolute differences -
// *
// *  Copyright(C) 2002 Hannes Jütting, Christopher Özbek
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
// * $Id: sad_ia64.s,v 1.7.10.1 2009/05/25 09:03:47 Isibaar Exp $
// *
// ***************************************************************************/
//
// ****************************************************************************
// *                                                                            
// *  sad_ia64.s, IA-64 sum of absolute differences                                
// *                                                                            
// *  This version was implemented during an IA-64 practical training at 	
// *  the University of Karlsruhe (http://i44w3.info.uni-karlsruhe.de/)		
// *                                                                            
// ****************************************************************************

//   ------------------------------------------------------------------------------
//   *
//   * Optimized Assembler Versions of sad8 and sad16
//   *
//   ------------------------------------------------------------------------------
//   *
//   * Hannes Jütting and Christopher Özbek 
//   * {s_juetti,s_oezbek}@ira.uka.de
//   *
//   * Programmed for the IA64 laboratory held at University Karlsruhe 2002
//   * http://www.info.uni-karlsruhe.de/~rubino/ia64p/
//   *
//   ------------------------------------------------------------------------------
//   *
//   * These are the optimized assembler versions of sad8 and sad16, which calculate 
//   * the sum of absolute differences between two 8x8/16x16 block matrices. 
//   *
//   * Our approach uses:
//   *   - The Itanium command psad1, which solves the problem in hardware. 
//   *   - Modulo-Scheduled Loops as the best way to loop unrolling on the IA64 
//   *     EPIC architecture
//   *   - Alignment resolving to avoid memory faults
//   *
//   ------------------------------------------------------------------------------

		
	
		
	.common	sad16bi#,8,8
	.align 16
	.global sad16bi_ia64#
	.proc sad16bi_ia64#
sad16bi_ia64:
	.prologue
	.save ar.lc, r2
	mov r2 = ar.lc
	.body
	zxt4 r35 = r35
	mov r8 = r0
	mov r23 = r0
	addl r22 = 255, r0
.L21:
	addl r14 = 7, r0
	mov r19 = r32
	mov r21 = r34
	mov r20 = r33
	;;
	mov ar.lc = r14
	;;
.L105:
	mov r17 = r20
	mov r18 = r21
	;;
	ld1 r14 = [r17], 1
	ld1 r15 = [r18], 1
	;;
	add r14 = r14, r15
	;;
	adds r14 = 1, r14
	;;
	shr.u r16 = r14, 1
	;;
	cmp4.le p6, p7 = r0, r16
	;;
	(p7) mov r16 = r0
	(p7) br.cond.dpnt .L96
	;;
	cmp4.ge p6, p7 = r22, r16
	;;
	(p7) addl r16 = 255, r0
.L96:
	ld1 r14 = [r19]
	adds r20 = 2, r20
	adds r21 = 2, r21
	;;
	sub r15 = r14, r16
	;;
	cmp4.ge p6, p7 = 0, r15
	;;
	(p6) sub r14 = r16, r14
	(p7) add r8 = r8, r15
	;;
	(p6) add r8 = r8, r14
	ld1 r15 = [r18]
	ld1 r14 = [r17]
	;;
	add r14 = r14, r15
	adds r17 = 1, r19
	;;
	adds r14 = 1, r14
	;;
	shr.u r16 = r14, 1
	;;
	cmp4.le p6, p7 = r0, r16
	;;
	(p7) mov r16 = r0
	(p7) br.cond.dpnt .L102
	;;
	cmp4.ge p6, p7 = r22, r16
	;;
	(p7) addl r16 = 255, r0
.L102:
	ld1 r14 = [r17]
	adds r19 = 2, r19
	;;
	sub r15 = r14, r16
	;;
	cmp4.ge p6, p7 = 0, r15
	;;
	(p7) add r8 = r8, r15
	(p6) sub r14 = r16, r14
	;;
	(p6) add r8 = r8, r14
	br.cloop.sptk.few .L105
	adds r23 = 1, r23
	add r32 = r32, r35
	add r33 = r33, r35
	add r34 = r34, r35
	;;
	cmp4.geu p6, p7 = 15, r23
	(p6) br.cond.dptk .L21
	mov ar.lc = r2
	br.ret.sptk.many b0
	.endp sad16bi_ia64#



	
	
	
	
	
.text
	.align 16
	.global dev16_ia64#
	.proc dev16_ia64#
.auto
dev16_ia64:
	// renamings for better readability
	stride = r18
	pfs = r19			//for saving previous function state
	cura0 = r20			//address of first 8-byte block of cur
	cura1 = r21			//address of second 8-byte block of cur
	mean0 = r22			//registers for calculating the sum in parallel
	mean1 = r23
	mean2 = r24
	mean3 = r25
	dev0 = r26			//same for the deviation
	dev1 = r27			
	dev2 = r28
	dev3 = r29
	
	.body
	alloc pfs = ar.pfs, 2, 38, 0, 40

	mov cura0  = in0
	mov stride = in1
	add cura1 = 8, cura0
	
	.rotr c[32], psad[8] 		// just using rotating registers to get an array ;-)

.explicit
{.mmi
	ld8 c[0] = [cura0], stride	// load them ...
	ld8 c[1] = [cura1], stride
	;; 
}	
{.mmi	
	ld8 c[2] = [cura0], stride
	ld8 c[3] = [cura1], stride
	;; 
}
{.mmi	
	ld8 c[4] = [cura0], stride
	ld8 c[5] = [cura1], stride
	;;
}
{.mmi
	ld8 c[6] = [cura0], stride
	ld8 c[7] = [cura1], stride
	;;
}
{.mmi	
	ld8 c[8] = [cura0], stride
	ld8 c[9] = [cura1], stride
	;;
}
{.mmi
	ld8 c[10] = [cura0], stride
	ld8 c[11] = [cura1], stride
	;;
}
{.mii
	ld8 c[12] = [cura0], stride
	psad1 mean0 = c[0], r0		// get the sum of them ...
	psad1 mean1 = c[1], r0
}
{.mmi
	ld8 c[13] = [cura1], stride
	;; 
	ld8 c[14] = [cura0], stride
	psad1 mean2 = c[2], r0
}
{.mii
	ld8 c[15] = [cura1], stride
	psad1 mean3 = c[3], r0
	;; 
	psad1 psad[0] = c[4], r0
}
{.mmi
	ld8 c[16] = [cura0], stride
	ld8 c[17] = [cura1], stride
	psad1 psad[1] = c[5], r0
	;;
}
{.mii	
	ld8 c[18] = [cura0], stride
	psad1 psad[2] = c[6], r0
	psad1 psad[3] = c[7], r0
}
{.mmi	
	ld8 c[19] = [cura1], stride
	;; 
	ld8 c[20] = [cura0], stride
	psad1 psad[4] = c[8], r0
}
{.mii
	ld8 c[21] = [cura1], stride
	psad1 psad[5] = c[9], r0
	;;
	add mean0 = mean0, psad[0]
}
{.mmi
	ld8 c[22] = [cura0], stride
	ld8 c[23] = [cura1], stride
	add mean1 = mean1, psad[1]
	;; 
}
{.mii
	ld8 c[24] = [cura0], stride
	psad1 psad[0] = c[10], r0
	psad1 psad[1] = c[11], r0
}
{.mmi
	ld8 c[25] = [cura1], stride
	;; 
	ld8 c[26] = [cura0], stride
	add mean2 = mean2, psad[2]
}
{.mii
	ld8 c[27] = [cura1], stride
	add mean3 = mean3, psad[3]
	;; 
	psad1 psad[2] = c[12], r0
}
{.mmi
	ld8 c[28] = [cura0], stride
	ld8 c[29] = [cura1], stride
	psad1 psad[3] = c[13], r0
	;; 
}
{.mii
	ld8 c[30] = [cura0]
	psad1 psad[6] = c[14], r0
	psad1 psad[7] = c[15], r0
}
{.mmi
	ld8 c[31] = [cura1]
	;; 
	add mean0 = mean0, psad[0]
	add mean1 = mean1, psad[1]
}
{.mii
	add mean2 = mean2, psad[4]
	add mean3 = mean3, psad[5]
	;;
	psad1 psad[0] = c[16], r0
}
{.mmi
	add mean0 = mean0, psad[2]
	add mean1 = mean1, psad[3]
	psad1 psad[1] = c[17], r0
	;;
}
{.mii
	add mean2 = mean2, psad[6]
	psad1 psad[2] = c[18], r0
	psad1 psad[3] = c[19], r0
}
{.mmi
	add mean3 = mean3, psad[7]
	;; 
	add mean0 = mean0, psad[0]
	psad1 psad[4] = c[20], r0
}
{.mii
	add mean1 = mean1, psad[1]
	psad1 psad[5] = c[21], r0
	;;
	psad1 psad[6] = c[22], r0
}
{.mmi
	add mean2 = mean2, psad[2]
	add mean3 = mean3, psad[3]
	psad1 psad[7] = c[23], r0
	;;
}
{.mii
	add mean0 = mean0, psad[4]
	psad1 psad[0] = c[24], r0
	psad1 psad[1] = c[25], r0
}
{.mmi
	add mean1 = mean1, psad[5]
	;;
	add mean2 = mean2, psad[6]
	psad1 psad[2] = c[26], r0
}
{.mii
	add mean3 = mean3, psad[7]
	psad1 psad[3] = c[27], r0
	;; 
	psad1 psad[4] = c[28], r0
}
{.mmi
	add mean0 = mean0, psad[0]
	add mean1 = mean1, psad[1]
	psad1 psad[5] = c[29], r0
	;;
}
{.mii
	add mean2 = mean2, psad[2]
	psad1 psad[6] = c[30], r0
	psad1 psad[7] = c[31], r0
}
{.mmi
	add mean3 = mean3, psad[3]
	;;
	add mean0 = mean0, psad[4]
	add mean1 = mean1, psad[5]
}
{.mbb
	add mean2 = mean2, mean3
	nop.b 1
	nop.b 1
	;;
}
{.mib
	add mean0 = mean0, psad[6]
	add mean1 = mean1, psad[7]
	nop.b 1
	;;
}
{.mib
	add mean0 = mean0, mean1
	// add mean2 = 127, mean2	// this could make our division more exactly, but does not help much
	;;
}
{.mib
	add mean0 = mean0, mean2
	;;
}

{.mib
	shr.u mean0 = mean0, 8		// divide them ...
	;;
}
{.mib
	mux1 mean0 = mean0, @brcst
	;; 
}	
{.mii
	nop.m 0
	psad1 dev0 = c[0], mean0	// and do a sad again ...
	psad1 dev1 = c[1], mean0
}
{.mii
	nop.m 0
	psad1 dev2 = c[2], mean0
	psad1 dev3 = c[3], mean0
}
{.mii
	nop.m 0
	psad1 psad[0] = c[4], mean0
	psad1 psad[1] = c[5], mean0
}
{.mii
	nop.m 0
	psad1 psad[2] = c[6], mean0
	psad1 psad[3] = c[7], mean0
}
{.mii
	nop.m 0
	psad1 psad[4] = c[8], mean0
	psad1 psad[5] = c[9], mean0
	;; 
}
{.mii
	add dev0 = dev0, psad[0]
	psad1 psad[6] = c[10], mean0
	psad1 psad[7] = c[11], mean0
}
{.mmi
	add dev1 = dev1, psad[1]

	add dev2 = dev2, psad[2]
	psad1 psad[0] = c[12], mean0
}
{.mii
	add dev3 = dev3, psad[3]
	psad1 psad[1] = c[13], mean0
	;; 
	psad1 psad[2] = c[14], mean0
}
{.mmi
	add dev0 = dev0, psad[4]
	add dev1 = dev1, psad[5]
	psad1 psad[3] = c[15], mean0
}
{.mii
	add dev2 = dev2, psad[6]
	psad1 psad[4] = c[16], mean0
	psad1 psad[5] = c[17], mean0
}
{.mmi
	add dev3 = dev3, psad[7]
	;; 
	add dev0 = dev0, psad[0]
	psad1 psad[6] = c[18], mean0
}
{.mii
	add dev1 = dev1, psad[1]
	psad1 psad[7] = c[19], mean0
	
	psad1 psad[0] = c[20], mean0
}
{.mmi	
	add dev2 = dev2, psad[2]
	add dev3 = dev3, psad[3]
	psad1 psad[1] = c[21], mean0
	;;
}
{.mii
	add dev0 = dev0, psad[4]
	psad1 psad[2] = c[22], mean0
	psad1 psad[3] = c[23], mean0
}
{.mmi
	add dev1 = dev1, psad[5]

	add dev2 = dev2, psad[6]
	psad1 psad[4] = c[24], mean0
}
{.mii
	add dev3 = dev3, psad[7]
	psad1 psad[5] = c[25], mean0
	;; 
	psad1 psad[6] = c[26], mean0
}
{.mmi
	add dev0 = dev0, psad[0]
	add dev1 = dev1, psad[1]
	psad1 psad[7] = c[27], mean0
}
{.mii
	add dev2 = dev2, psad[2]
	psad1 psad[0] = c[28], mean0
	psad1 psad[1] = c[29], mean0
}
{.mmi
	add dev3 = dev3, psad[3]
	;;
	add dev0 = dev0, psad[4]
	psad1 psad[2] = c[30], mean0
}
{.mii
	add dev1 = dev1, psad[5]
	psad1 psad[3] = c[31], mean0
	;; 
	add dev2 = dev2, psad[6]
}
{.mmi
	add dev3 = dev3, psad[7]
	add dev0 = dev0, psad[0]
	add dev1 = dev1, psad[1]
	;;
}
{.mii
	add dev2 = dev2, psad[2]
	add dev3 = dev3, psad[3]
	add ret0 = dev0, dev1
	;; 
}
{.mib
	add dev2 = dev2, dev3
	nop.i 1
	nop.b 1
	;; 
}
{.mib
	add ret0 = ret0, dev2
	nop.i 1
	br.ret.sptk.many b0
}
	.endp dev16_ia64#


// ###########################################################
// ###########################################################
// Neue version von gruppe 01 ################################
// ###########################################################
// ###########################################################

		
	
	.text
	.align 16
	.global sad16_ia64#
	.proc sad16_ia64#
sad16_ia64:
	alloc r1 = ar.pfs, 4, 76, 0, 0
	mov r2 = pr
	dep r14 = r0, r33, 0, 3		// r14 = (r33 div 8)*8 (aligned version of ref)
	dep.z r31 = r33, 0, 3		// r31 = r33 mod 8 (misalignment of ref)
	;;
	mov r64 = r34			//(1) calculate multiples of stride
	shl r65 = r34, 1		//(2) for being able to load all the
	shladd r66 = r34, 1, r34	//(3) data at once
	shl r67 = r34, 2		//(4)
	shladd r68 = r34, 2, r34	//(5)
	shl r71 = r34, 3		//(8)
	shladd r72 = r34, 3, r34	//(9)
	;;
	shl r69 = r66, 1		//(6)
	shladd r70 = r66, 1, r34	//(7)
	shl r73 = r68, 1		//(10)
	shladd r74 = r68, 1, r34	//(11)
	shl r75 = r66, 2		//(12)
	shladd r76 = r66, 2, r34	//(13)
	shladd r77 = r66, 2, r65	//(14)
	shladd r78 = r66, 2, r66	//(15)
	;;
	cmp.eq p16, p17 = 0, r31	// prepare predicates according to the misalignment
	cmp.eq p18, p19 = 2, r31	// ref
	cmp.eq p20, p21 = 4, r31
	cmp.eq p22, p23 = 6, r31
	cmp.eq p24, p25 = 1, r31
	cmp.eq p26, p27 = 3, r31
	cmp.eq p28, p29 = 5, r31
	mov r96 = r14			// and calculate all the adresses where we have
	mov r33 = r32			// to load from
	add r97 = r14, r64
	add r35 = r32, r64
	add r98 = r14, r65
	add r37 = r32, r65
	add r99 = r14, r66
	add r39 = r32, r66
	add r100 = r14, r67
	add r41 = r32, r67
	add r101 = r14, r68
	add r43 = r32, r68
	add r102 = r14, r69
	add r45 = r32, r69
	add r103 = r14, r70
	add r47 = r32, r70
	add r104 = r14, r71
	add r49 = r32, r71
	add r105 = r14, r72
	add r51 = r32, r72
	add r106 = r14, r73
	add r53 = r32, r73
	add r107 = r14, r74
	add r55 = r32, r74
	add r108 = r14, r75
	add r57 = r32, r75
	add r109 = r14, r76
	add r59 = r32, r76
	add r110 = r14, r77
	add r61 = r32, r77
	add r111 = r14, r78
	add r63 = r32, r78
	;;
	ld8 r32 = [r33], 8		// Load all the data which is needed for the sad
	ld8 r34 = [r35], 8		// in the registers. the goal is to have the array
	ld8 r36 = [r37], 8		// adressed by cur in the registers r32 - r63 and
	ld8 r38 = [r39], 8		// the aray adressed by ref in the registers
	ld8 r40 = [r41], 8		// r64 - r95. The registers r96 - r111 are needed
	ld8 r42 = [r43], 8		// to load the aligned 24 bits in which the
	ld8 r44 = [r45], 8		// needed misaligned 16 bits must be.
	ld8 r46 = [r47], 8		// After loading we start a preprocessing which
	ld8 r48 = [r49], 8		// guarantees that the data adressed by ref is in
	ld8 r50 = [r51], 8		// the registers r64 - r95.
	ld8 r52 = [r53], 8
	ld8 r54 = [r55], 8
	ld8 r56 = [r57], 8
	ld8 r58 = [r59], 8
	ld8 r60 = [r61], 8
	ld8 r62 = [r63], 8
	ld8 r64 = [r96], 8
	ld8 r66 = [r97], 8
	ld8 r68 = [r98], 8
	ld8 r70 = [r99], 8
	ld8 r72 = [r100], 8
	ld8 r74 = [r101], 8
	ld8 r76 = [r102], 8
	ld8 r78 = [r103], 8
	ld8 r80 = [r104], 8
	ld8 r82 = [r105], 8
	ld8 r84 = [r106], 8
	ld8 r86 = [r107], 8
	ld8 r88 = [r108], 8
	ld8 r90 = [r109], 8
	ld8 r92 = [r110], 8
	ld8 r94 = [r111], 8
	;;
	ld8 r33 = [r33]
	ld8 r35 = [r35]
	ld8 r37 = [r37]
	ld8 r39 = [r39]
	ld8 r41 = [r41]
	ld8 r43 = [r43]
	ld8 r45 = [r45]
	ld8 r47 = [r47]
	ld8 r49 = [r49]
	ld8 r51 = [r51]
	ld8 r53 = [r53]
	ld8 r55 = [r55]
	ld8 r57 = [r57]
	ld8 r59 = [r59]
	ld8 r61 = [r61]
	ld8 r63 = [r63]
	ld8 r65 = [r96], 8
	ld8 r67 = [r97], 8
	ld8 r69 = [r98], 8
	ld8 r71 = [r99], 8
	ld8 r73 = [r100], 8
	ld8 r75 = [r101], 8
	ld8 r77 = [r102], 8
	ld8 r79 = [r103], 8
	ld8 r81 = [r104], 8
	ld8 r83 = [r105], 8
	ld8 r85 = [r106], 8
	ld8 r87 = [r107], 8
	ld8 r89 = [r108], 8
	ld8 r91 = [r109], 8
	ld8 r93 = [r110], 8
	ld8 r95 = [r111], 8
	(p16) br.cond.dptk.many .Lber	// If ref is aligned, everything is loaded and we can start the calculation
	;;
	ld8 r96 = [r96]			// If not, we have to load a bit more
	ld8 r97 = [r97]
	ld8 r98 = [r98]
	ld8 r99 = [r99]
	ld8 r100 = [r100]
	ld8 r101 = [r101]
	ld8 r102 = [r102]
	ld8 r103 = [r103]
	ld8 r104 = [r104]
	ld8 r105 = [r105]
	ld8 r106 = [r106]
	ld8 r107 = [r107]
	ld8 r108 = [r108]
	ld8 r109 = [r109]
	ld8 r110 = [r110]
	ld8 r111 = [r111]
	(p24) br.cond.dptk.many .Lmod1	// according to the misalignment, we have
	(p18) br.cond.dpnt.many .Lmod2	// to jump to different preprocessing routines
	(p26) br.cond.dpnt.many .Lmod3
	(p20) br.cond.dpnt.many .Lmod4
	(p28) br.cond.dpnt.many .Lmod5
	(p22) br.cond.dpnt.many .Lmod6
	;;
.Lmod7:					// this jump point is not needed
	shrp r64 = r65, r64, 56		// in these blocks, we do the preprocessing
	shrp r65 = r96, r65, 56
	shrp r66 = r67, r66, 56
	shrp r67 = r97, r67, 56
	shrp r68 = r69, r68, 56
	shrp r69 = r98, r69, 56
	shrp r70 = r71, r70, 56
	shrp r71 = r99, r71, 56
	shrp r72 = r73, r72, 56
	shrp r73 = r100, r73, 56
	shrp r74 = r75, r74, 56
	shrp r75 = r101, r75, 56
	shrp r76 = r77, r76, 56
	shrp r77 = r102, r77, 56
	shrp r78 = r79, r78, 56
	shrp r79 = r103, r79, 56
	shrp r80 = r81, r80, 56
	shrp r81 = r104, r81, 56
	shrp r82 = r83, r82, 56
	shrp r83 = r105, r83, 56
	shrp r84 = r85, r84, 56
	shrp r85 = r106, r85, 56
	shrp r86 = r87, r86, 56
	shrp r87 = r107, r87, 56
	shrp r88 = r89, r88, 56
	shrp r89 = r108, r89, 56
	shrp r90 = r91, r90, 56
	shrp r91 = r109, r91, 56
	shrp r92 = r93, r92, 56
	shrp r93 = r110, r93, 56
	shrp r94 = r95, r94, 56
	shrp r95 = r111, r95, 56
	br.cond.sptk.many .Lber		// and then we jump to the calculation
	;;
.Lmod6:
	shrp r64 = r65, r64, 48
	shrp r65 = r96, r65, 48
	shrp r66 = r67, r66, 48
	shrp r67 = r97, r67, 48
	shrp r68 = r69, r68, 48
	shrp r69 = r98, r69, 48
	shrp r70 = r71, r70, 48
	shrp r71 = r99, r71, 48
	shrp r72 = r73, r72, 48
	shrp r73 = r100, r73, 48
	shrp r74 = r75, r74, 48
	shrp r75 = r101, r75, 48
	shrp r76 = r77, r76, 48
	shrp r77 = r102, r77, 48
	shrp r78 = r79, r78, 48
	shrp r79 = r103, r79, 48
	shrp r80 = r81, r80, 48
	shrp r81 = r104, r81, 48
	shrp r82 = r83, r82, 48
	shrp r83 = r105, r83, 48
	shrp r84 = r85, r84, 48
	shrp r85 = r106, r85, 48
	shrp r86 = r87, r86, 48
	shrp r87 = r107, r87, 48
	shrp r88 = r89, r88, 48
	shrp r89 = r108, r89, 48
	shrp r90 = r91, r90, 48
	shrp r91 = r109, r91, 48
	shrp r92 = r93, r92, 48
	shrp r93 = r110, r93, 48
	shrp r94 = r95, r94, 48
	shrp r95 = r111, r95, 48
	br.cond.sptk.many .Lber
	;;
.Lmod5:
	shrp r64 = r65, r64, 40
	shrp r65 = r96, r65, 40
	shrp r66 = r67, r66, 40
	shrp r67 = r97, r67, 40
	shrp r68 = r69, r68, 40
	shrp r69 = r98, r69, 40
	shrp r70 = r71, r70, 40
	shrp r71 = r99, r71, 40
	shrp r72 = r73, r72, 40
	shrp r73 = r100, r73, 40
	shrp r74 = r75, r74, 40
	shrp r75 = r101, r75, 40
	shrp r76 = r77, r76, 40
	shrp r77 = r102, r77, 40
	shrp r78 = r79, r78, 40
	shrp r79 = r103, r79, 40
	shrp r80 = r81, r80, 40
	shrp r81 = r104, r81, 40
	shrp r82 = r83, r82, 40
	shrp r83 = r105, r83, 40
	shrp r84 = r85, r84, 40
	shrp r85 = r106, r85, 40
	shrp r86 = r87, r86, 40
	shrp r87 = r107, r87, 40
	shrp r88 = r89, r88, 40
	shrp r89 = r108, r89, 40
	shrp r90 = r91, r90, 40
	shrp r91 = r109, r91, 40
	shrp r92 = r93, r92, 40
	shrp r93 = r110, r93, 40
	shrp r94 = r95, r94, 40
	shrp r95 = r111, r95, 40
	br.cond.sptk.many .Lber
	;;
.Lmod4:
	shrp r64 = r65, r64, 32
	shrp r65 = r96, r65, 32
	shrp r66 = r67, r66, 32
	shrp r67 = r97, r67, 32
	shrp r68 = r69, r68, 32
	shrp r69 = r98, r69, 32
	shrp r70 = r71, r70, 32
	shrp r71 = r99, r71, 32
	shrp r72 = r73, r72, 32
	shrp r73 = r100, r73, 32
	shrp r74 = r75, r74, 32
	shrp r75 = r101, r75, 32
	shrp r76 = r77, r76, 32
	shrp r77 = r102, r77, 32
	shrp r78 = r79, r78, 32
	shrp r79 = r103, r79, 32
	shrp r80 = r81, r80, 32
	shrp r81 = r104, r81, 32
	shrp r82 = r83, r82, 32
	shrp r83 = r105, r83, 32
	shrp r84 = r85, r84, 32
	shrp r85 = r106, r85, 32
	shrp r86 = r87, r86, 32
	shrp r87 = r107, r87, 32
	shrp r88 = r89, r88, 32
	shrp r89 = r108, r89, 32
	shrp r90 = r91, r90, 32
	shrp r91 = r109, r91, 32
	shrp r92 = r93, r92, 32
	shrp r93 = r110, r93, 32
	shrp r94 = r95, r94, 32
	shrp r95 = r111, r95, 32
	br.cond.sptk.many .Lber
	;;
.Lmod3:
	shrp r64 = r65, r64, 24
	shrp r65 = r96, r65, 24
	shrp r66 = r67, r66, 24
	shrp r67 = r97, r67, 24
	shrp r68 = r69, r68, 24
	shrp r69 = r98, r69, 24
	shrp r70 = r71, r70, 24
	shrp r71 = r99, r71, 24
	shrp r72 = r73, r72, 24
	shrp r73 = r100, r73, 24
	shrp r74 = r75, r74, 24
	shrp r75 = r101, r75, 24
	shrp r76 = r77, r76, 24
	shrp r77 = r102, r77, 24
	shrp r78 = r79, r78, 24
	shrp r79 = r103, r79, 24
	shrp r80 = r81, r80, 24
	shrp r81 = r104, r81, 24
	shrp r82 = r83, r82, 24
	shrp r83 = r105, r83, 24
	shrp r84 = r85, r84, 24
	shrp r85 = r106, r85, 24
	shrp r86 = r87, r86, 24
	shrp r87 = r107, r87, 24
	shrp r88 = r89, r88, 24
	shrp r89 = r108, r89, 24
	shrp r90 = r91, r90, 24
	shrp r91 = r109, r91, 24
	shrp r92 = r93, r92, 24
	shrp r93 = r110, r93, 24
	shrp r94 = r95, r94, 24
	shrp r95 = r111, r95, 24
	br.cond.sptk.many .Lber
	;;
.Lmod2:
	shrp r64 = r65, r64, 16
	shrp r65 = r96, r65, 16
	shrp r66 = r67, r66, 16
	shrp r67 = r97, r67, 16
	shrp r68 = r69, r68, 16
	shrp r69 = r98, r69, 16
	shrp r70 = r71, r70, 16
	shrp r71 = r99, r71, 16
	shrp r72 = r73, r72, 16
	shrp r73 = r100, r73, 16
	shrp r74 = r75, r74, 16
	shrp r75 = r101, r75, 16
	shrp r76 = r77, r76, 16
	shrp r77 = r102, r77, 16
	shrp r78 = r79, r78, 16
	shrp r79 = r103, r79, 16
	shrp r80 = r81, r80, 16
	shrp r81 = r104, r81, 16
	shrp r82 = r83, r82, 16
	shrp r83 = r105, r83, 16
	shrp r84 = r85, r84, 16
	shrp r85 = r106, r85, 16
	shrp r86 = r87, r86, 16
	shrp r87 = r107, r87, 16
	shrp r88 = r89, r88, 16
	shrp r89 = r108, r89, 16
	shrp r90 = r91, r90, 16
	shrp r91 = r109, r91, 16
	shrp r92 = r93, r92, 16
	shrp r93 = r110, r93, 16
	shrp r94 = r95, r94, 16
	shrp r95 = r111, r95, 16
	br.cond.sptk.many .Lber
	;;
.Lmod1:
	shrp r64 = r65, r64, 8
	shrp r65 = r96, r65, 8
	shrp r66 = r67, r66, 8
	shrp r67 = r97, r67, 8
	shrp r68 = r69, r68, 8
	shrp r69 = r98, r69, 8
	shrp r70 = r71, r70, 8
	shrp r71 = r99, r71, 8
	shrp r72 = r73, r72, 8
	shrp r73 = r100, r73, 8
	shrp r74 = r75, r74, 8
	shrp r75 = r101, r75, 8
	shrp r76 = r77, r76, 8
	shrp r77 = r102, r77, 8
	shrp r78 = r79, r78, 8
	shrp r79 = r103, r79, 8
	shrp r80 = r81, r80, 8
	shrp r81 = r104, r81, 8
	shrp r82 = r83, r82, 8
	shrp r83 = r105, r83, 8
	shrp r84 = r85, r84, 8
	shrp r85 = r106, r85, 8
	shrp r86 = r87, r86, 8
	shrp r87 = r107, r87, 8
	shrp r88 = r89, r88, 8
	shrp r89 = r108, r89, 8
	shrp r90 = r91, r90, 8
	shrp r91 = r109, r91, 8
	shrp r92 = r93, r92, 8
	shrp r93 = r110, r93, 8
	shrp r94 = r95, r94, 8
	shrp r95 = r111, r95, 8
.Lber:
	;;
	psad1 r32 = r32, r64		// Here we do the calculation.
	psad1 r33 = r33, r65		// The machine is providing a fast method
	psad1 r34 = r34, r66		// for calculating sad, so we use it
	psad1 r35 = r35, r67
	psad1 r36 = r36, r68
	psad1 r37 = r37, r69
	psad1 r38 = r38, r70
	psad1 r39 = r39, r71
	psad1 r40 = r40, r72
	psad1 r41 = r41, r73
	psad1 r42 = r42, r74
	psad1 r43 = r43, r75
	psad1 r44 = r44, r76
	psad1 r45 = r45, r77
	psad1 r46 = r46, r78
	psad1 r47 = r47, r79
	psad1 r48 = r48, r80
	psad1 r49 = r49, r81
	psad1 r50 = r50, r82
	psad1 r51 = r51, r83
	psad1 r52 = r52, r84
	psad1 r53 = r53, r85
	psad1 r54 = r54, r86
	psad1 r55 = r55, r87
	psad1 r56 = r56, r88
	psad1 r57 = r57, r89
	psad1 r58 = r58, r90
	psad1 r59 = r59, r91
	psad1 r60 = r60, r92
	psad1 r61 = r61, r93
	psad1 r62 = r62, r94
	psad1 r63 = r63, r95
	;;
	add r32 = r32, r63		// at last, we have to sum up
	add r33 = r33, r62		// in 5 stages
	add r34 = r34, r61
	add r35 = r35, r60
	add r36 = r36, r59
	add r37 = r37, r58
	add r38 = r38, r57
	add r39 = r39, r56
	add r40 = r40, r55
	add r41 = r41, r54
	add r42 = r42, r53
	add r43 = r43, r52
	add r44 = r44, r51
	add r45 = r45, r50
	add r46 = r46, r49
	add r47 = r47, r48
	;;
	add r32 = r32, r47
	add r33 = r33, r46
	add r34 = r34, r45
	add r35 = r35, r44
	add r36 = r36, r43
	add r37 = r37, r42
	add r38 = r38, r41
	add r39 = r39, r40
	;;
	add r32 = r32, r39
	add r33 = r33, r38
	add r34 = r34, r37
	add r35 = r35, r36
	;;
	add r32 = r32, r35
	add r33 = r33, r34
	;;
	add r8 = r32, r33		// and store the result in r8
	mov pr = r2, -1
	mov ar.pfs = r1
	br.ret.sptk.many b0
	.endp sad16_ia64#



	
	.align 16
	.global sad8_ia64#
	.proc sad8_ia64#
sad8_ia64:
	alloc r1 = ar.pfs, 3, 21, 0, 0
	mov r2 = pr
	dep r14 = r0, r33, 0, 3	// calculate aligned version of ref
	dep.z r31 = r33, 0, 3		// calculate misalignment of ref
	;;
	mov r40 = r34		//(1) calculate multiples of stride
	shl r41 = r34, 1		//(2)
	shladd r42 = r34, 1, r34	//(3)
	shl r43 = r34, 2		//(4)
	shladd r44 = r34, 2, r34	//(5)
	;;
	cmp.eq p16, p17 = 0, r31	// set predicates according to the misalignment of ref
	cmp.eq p18, p19 = 2, r31
	shl r45 = r42, 1		//(6)
	cmp.eq p20, p21 = 4, r31
	cmp.eq p22, p23 = 6, r31
	shladd r46 = r42, 1, r34	//(7)
	cmp.eq p24, p25 = 1, r31
	cmp.eq p26, p27 = 3, r31
	cmp.eq p28, p29 = 5, r31
	;;
	mov r48 = r14		// calculate memory adresses of data
	add r33 = r32, r40
	add r49 = r14, r40
	add r34 = r32, r41
	add r50 = r14, r41
	add r35 = r32, r42
	add r51 = r14, r42
	add r36 = r32, r43
	add r52 = r14, r43
	add r37 = r32, r44
	add r53 = r14, r44
	add r38 = r32, r45
	add r54 = r14, r45
	add r39 = r32, r46
	add r55 = r14, r46
	;;
	ld8 r32 = [r32]		// load everythingund alles wird geladen
	ld8 r33 = [r33]		// cur is located in r32 - r39
	ld8 r34 = [r34]		// ref in r40 - r47
	ld8 r35 = [r35]
	ld8 r36 = [r36]
	ld8 r37 = [r37]
	ld8 r38 = [r38]
	ld8 r39 = [r39]
	ld8 r40 = [r48] ,8
	ld8 r41 = [r49] ,8
	ld8 r42 = [r50] ,8
	ld8 r43 = [r51] ,8
	ld8 r44 = [r52] ,8
	ld8 r45 = [r53] ,8
	ld8 r46 = [r54] ,8
	ld8 r47 = [r55] ,8
	(p16) br.cond.dptk.many .Lber2	// if ref is aligned, we can start the calculation
	;;
	ld8 r48 = [r48]		// if not, we have to load some more
	ld8 r49 = [r49]		// because of the alignment of ld8
	ld8 r50 = [r50]
	ld8 r51 = [r51]
	ld8 r52 = [r52]
	ld8 r53 = [r53]
	ld8 r54 = [r54]
	ld8 r55 = [r55]
	(p24) br.cond.dptk.many .Lmode1
	(p18) br.cond.dpnt.many .Lmode2
	(p26) br.cond.dpnt.many .Lmode3
	(p20) br.cond.dpnt.many .Lmode4
	(p28) br.cond.dpnt.many .Lmode5
	(p22) br.cond.dpnt.many .Lmode6
	;;
.Lmode7:				// this jump piont is not needed, it is for better understandment
	shrp r40 = r48, r40, 56	// here we do some preprocessing on the data
	shrp r41 = r49, r41, 56	// this is because of the alignment problem of ref
	shrp r42 = r50, r42, 56
	shrp r43 = r51, r43, 56
	shrp r44 = r52, r44, 56
	shrp r45 = r53, r45, 56
	shrp r46 = r54, r46, 56
	shrp r47 = r55, r47, 56
	br.cond.sptk.many .Lber2
	;;
.Lmode6:
	shrp r40 = r48, r40, 48
	shrp r41 = r49, r41, 48
	shrp r42 = r50, r42, 48
	shrp r43 = r51, r43, 48
	shrp r44 = r52, r44, 48
	shrp r45 = r53, r45, 48
	shrp r46 = r54, r46, 48
	shrp r47 = r55, r47, 48
	br.cond.sptk.many .Lber2
	;;
.Lmode5:
	shrp r40 = r48, r40, 40
	shrp r41 = r49, r41, 40
	shrp r42 = r50, r42, 40
	shrp r43 = r51, r43, 40
	shrp r44 = r52, r44, 40
	shrp r45 = r53, r45, 40
	shrp r46 = r54, r46, 40
	shrp r47 = r55, r47, 40
	br.cond.sptk.many .Lber2
	;;
.Lmode4:
	shrp r40 = r48, r40, 32
	shrp r41 = r49, r41, 32
	shrp r42 = r50, r42, 32
	shrp r43 = r51, r43, 32
	shrp r44 = r52, r44, 32
	shrp r45 = r53, r45, 32
	shrp r46 = r54, r46, 32
	shrp r47 = r55, r47, 32
	br.cond.sptk.many .Lber2
	;;
.Lmode3:
	shrp r40 = r48, r40, 24
	shrp r41 = r49, r41, 24
	shrp r42 = r50, r42, 24
	shrp r43 = r51, r43, 24
	shrp r44 = r52, r44, 24
	shrp r45 = r53, r45, 24
	shrp r46 = r54, r46, 24
	shrp r47 = r55, r47, 24
	br.cond.sptk.many .Lber2
	;;
.Lmode2:
	shrp r40 = r48, r40, 16
	shrp r41 = r49, r41, 16
	shrp r42 = r50, r42, 16
	shrp r43 = r51, r43, 16
	shrp r44 = r52, r44, 16
	shrp r45 = r53, r45, 16
	shrp r46 = r54, r46, 16
	shrp r47 = r55, r47, 16
	br.cond.sptk.many .Lber2
	;;
.Lmode1:
	shrp r40 = r48, r40, 8
	shrp r41 = r49, r41, 8
	shrp r42 = r50, r42, 8
	shrp r43 = r51, r43, 8
	shrp r44 = r52, r44, 8
	shrp r45 = r53, r45, 8
	shrp r46 = r54, r46, 8
	shrp r47 = r55, r47, 8
.Lber2:
	;;
	psad1 r32 = r32, r40	// we start calculating sad
	psad1 r33 = r33, r41	// using th psad1 command of IA64
	psad1 r34 = r34, r42
	psad1 r35 = r35, r43
	psad1 r36 = r36, r44
	psad1 r37 = r37, r45
	psad1 r38 = r38, r46
	psad1 r39 = r39, r47
	;;
	add r32 = r32, r33		// then we sum up everything
	add r33 = r34, r35
	add r34 = r36, r37
	add r35 = r38, r39
	;;
	add r32 = r32, r33
	add r33 = r34, r35
	;;
	add r8 = r32, r33		// and store the result un r8
	mov pr = r2, -1
	mov ar.pfs = r1
	br.ret.sptk.many b0
	.endp sad8_ia64#
