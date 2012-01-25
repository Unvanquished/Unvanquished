// ****************************************************************************
// *
// *  XVID MPEG-4 VIDEO CODEC
// *  - IA64 h.263 quantization -
// *
// *  Copyright(C) 2002 Christian Engel, Hans-Joachim Daniels
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
// * $Id: quant_h263_ia64.s,v 1.6.6.1 2009/05/25 09:03:47 Isibaar Exp $
// *
// ***************************************************************************/
//
// ****************************************************************************
// *                                                                            
// *  quant_h263_ia64.s, IA-64 h.263 quantization                                  
// *                                                                            
// *  This version was implemented during an IA-64 practical training at 	
// *  the University of Karlsruhe (http://i44w3.info.uni-karlsruhe.de/)		
// *                                                                            
// ****************************************************************************

// *****************************************************************************
// *									
// *	functions quant_inter and dequant_inter have been softwarepipelined
// *	use was made of the pmpyshr2 instruction				
// *										
// *	by Christian Engel and Hans-Joachim Daniels				
// *	christian.engel@ira.uka.de hans-joachim.daniels@ira.uka.de		
// *										
// *	This was made for the ia64 DivX laboratory (yes, it was really called	
// *	this way, originally OpenDivX was intendet, but died shortly before our	
// *	work started (you will probably already know ...))			
// *	at the Universitat Karlsruhe (TH) held between April and July 2002	
// * 	http://www.info.uni-karlsruhe.de/~rubino/ia64p/				
// *										
// *****************************************************************************/

	.file	"quant_h263_ia64.s"
	.pred.safe_across_calls p1-p5,p16-p63
		.section	.rodata
	.align 4
	.type	 multipliers#,@object
	.size	 multipliers#,128
multipliers:
	data4	0
	data4	32769
	data4	16385
	data4	10923
	data4	8193
	data4	6554
	data4	5462
	data4	4682
	data4	4097
	data4	3641
	data4	3277
	data4	2979
	data4	2731
	data4	2521
	data4	2341
	data4	2185
	data4	2049
	data4	1928
	data4	1821
	data4	1725
	data4	1639
	data4	1561
	data4	1490
	data4	1425
	data4	1366
	data4	1311
	data4	1261
	data4	1214
	data4	1171
	data4	1130
	data4	1093
	data4	1058
	.global __divdi3#
.text
	.align 16
	.global quant_h263_intra_ia64#
	.proc quant_h263_intra_ia64#
quant_h263_intra_ia64:
	.prologue 
	.save ar.pfs, r38
	alloc r38 = ar.pfs, 4, 3, 2, 0
	adds r16 = -8, r12
	.fframe 32
	adds r12 = -32, r12
	mov r17 = ar.lc
	addl r14 = @ltoff(multipliers#), gp
	ld2 r15 = [r33]
	;;
	.savesp ar.lc, 24
	st8 [r16] = r17, 8
	ld8 r14 = [r14]
	sxt2 r15 = r15
	;;
	.save.f 0x1
	stf.spill [r16] = f2
	.save rp, r37
	mov r37 = b0
	.body
	dep.z r36 = r34, 1, 15
	dep.z r16 = r34, 2, 32
	cmp4.ge p6, p7 = 0, r15
	;;
	add r16 = r16, r14
	;;
	ld4 r16 = [r16]
	;;
	setf.sig f2 = r16
	(p6) br.cond.dptk .L8
	extr r39 = r35, 1, 31
	sxt4 r40 = r35
	;;
	add r39 = r39, r15
	br .L21
	;;
.L8:
	extr r39 = r35, 1, 31
	sxt4 r40 = r35
	;;
	sub r39 = r15, r39
	;;
.L21:
	sxt4 r39 = r39
	br.call.sptk.many b0 = __divdi3#
	;;
	addl r14 = 62, r0
	st2 [r32] = r8
	addl r19 = 1, r0
	;;
	mov ar.lc = r14
	;;
.L20:
	dep.z r17 = r19, 1, 32
	;;
	add r15 = r17, r33
	adds r19 = 1, r19
	;;
	ld2 r14 = [r15]
	;;
	sxt2 r14 = r14
	;;
	mov r16 = r14
	mov r18 = r14
	;;
	sub r15 = r0, r16
	cmp4.le p8, p9 = r36, r16
	cmp4.le p6, p7 = r0, r16
	;;
	sxt2 r14 = r15
	(p6) br.cond.dptk .L14
	;;
	mov r16 = r14
	add r18 = r17, r32
	;;
	setf.sig f6 = r16
	cmp4.le p6, p7 = r36, r16
	mov r15 = r18
	;;
	xma.l f6 = f6, f2, f0
	(p7) st2 [r18] = r0
	;;
	getf.sig r14 = f6
	;;
	extr r14 = r14, 16, 16
	;;
	sub r14 = r0, r14
	;;
	(p6) st2 [r15] = r14
	br .L12
.L14:
	.pred.rel "mutex", p8, p9
	setf.sig f6 = r18
	add r16 = r17, r32
	;;
	xma.l f6 = f6, f2, f0
	mov r15 = r16
	(p9) st2 [r16] = r0
	;;
	getf.sig r14 = f6
	;;
	extr r14 = r14, 16, 16
	;;
	(p8) st2 [r15] = r14
.L12:
	br.cloop.sptk.few .L20
	adds r18 = 24, r12
	;;
	ld8 r19 = [r18], 8
	mov ar.pfs = r38
	mov b0 = r37
	;;
	mov ar.lc = r19
	ldf.fill f2 = [r18]
	.restore sp
	adds r12 = 32, r12
	br.ret.sptk.many b0
	.endp quant_h263_intra_ia64#
	.common	quant_h263_intra#,8,8
	.common	dequant_h263_intra#,8,8
	.align 16
	.global dequant_h263_intra_ia64#
	.proc dequant_h263_intra_ia64#
dequant_h263_intra_ia64:
	.prologue
	ld2 r14 = [r33]
	andcm r15 = 1, r34
	setf.sig f8 = r35
	;;
	sxt2 r14 = r14
	sub r15 = r34, r15
	addl r16 = -2048, r0
	;;
	setf.sig f6 = r14
	setf.sig f7 = r15
	shladd r34 = r34, 1, r0
	;;
	xma.l f8 = f6, f8, f0
	.save ar.lc, r2
	mov r2 = ar.lc
	;;
	.body
	getf.sig r14 = f8
	setf.sig f6 = r34
	;;
	sxt2 r15 = r14
	st2 [r32] = r14
	;;
	cmp4.le p6, p7 = r16, r15
	;;
	(p7) st2 [r32] = r16
	(p7) br.cond.dptk .L32
	addl r14 = 2047, r0
	;;
	cmp4.ge p6, p7 = r14, r15
	;;
	(p7) st2 [r32] = r14
.L32:
	addl r14 = 62, r0
	addl r19 = 1, r0
	addl r22 = 2048, r0
	addl r21 = -2048, r0
	addl r20 = 2047, r0
	;;
	mov ar.lc = r14
	;;
.L56:
	dep.z r16 = r19, 1, 32
	;;
	add r14 = r16, r33
	add r17 = r16, r32
	adds r19 = 1, r19
	;;
	ld2 r15 = [r14]
	;;
	sxt2 r15 = r15
	;;
	cmp4.ne p6, p7 = 0, r15
	cmp4.le p8, p9 = r0, r15
	;;
	(p7) st2 [r17] = r0
	(p7) br.cond.dpnt .L36
	add r18 = r16, r32
	sub r17 = r0, r15
	;;
	mov r14 = r18
	(p8) br.cond.dptk .L40
	setf.sig f8 = r17
	;;
	xma.l f8 = f6, f8, f7
	;;
	getf.sig r15 = f8
	;;
	cmp4.lt p6, p7 = r22, r15
	sub r16 = r0, r15
	;;
	(p7) st2 [r14] = r16
	(p6) st2 [r14] = r21
	br .L36
.L40:
	setf.sig f8 = r15
	;;
	xma.l f8 = f6, f8, f7
	;;
	getf.sig r15 = f8
	;;
	cmp4.le p6, p7 = r20, r15
	;;
	(p6) mov r14 = r20
	(p7) mov r14 = r15
	;;
	st2 [r18] = r14
.L36:
	br.cloop.sptk.few .L56
	;;
	mov ar.lc = r2
	br.ret.sptk.many b0
	.endp dequant_h263_intra_ia64#



// uint32_t quant_h263_inter_ia64(int16_t *coeff, const int16_t *data, const uint32_t quant)



	.common	quant_h263_inter#,8,8
	.align 16
	.global quant_h263_inter_ia64#
	.proc quant_h263_inter_ia64#
quant_h263_inter_ia64:


//*******************************************************
//*							*
//*	const uint32_t mult = multipliers[quant];	*
//*	const uint16_t quant_m_2 = quant << 1;		*
//*	const uint16_t quant_d_2 = quant >> 1;		*
//*	int sum = 0;					*
//*	uint32_t i;					*
//*	int16_t acLevel,acL;				*
//*							*
//*******************************************************/



	LL=3		// LL = load latency
			//if LL is changed, you'll also have to change the .pred.rel... parts below!	
	.prologue
	addl r14 = @ltoff(multipliers#), gp
	dep.z r15 = r34, 2, 32
	.save ar.lc, r2
	mov r2 = ar.lc
	;;
	.body
	alloc r9=ar.pfs,0,24,0,24
	mov r17 = ar.ec
	mov r10 = pr
	ld8 r14 = [r14]
	extr.u r16 = r34, 1, 16		//r16 = quant_d_2
	dep.z r20 = r34, 1, 15		//r20 = quant_m_2
	;;
	add r15 = r15, r14
	mov r21 = r16			//r21 = quant_d_2
	mov r8 = r0			//r8  = sum = 0
	mov pr.rot    = 0		//p16-p63 = 0
	;;
	ld4 r15 = [r15]
	addl r14 = 63, r0
	mov pr.rot = 1 << 16		//p16=1	
	;;
	mov ar.lc = r14
	mov ar.ec = LL+9
	mov r29 = r15
	;;
	mov r15 = r33			//r15 = data
	mov r18 = r32			//r18 = coeff
	;;
	
	
	.rotr ac1[LL+3], ac2[8], ac3[2]
	.rotp p[LL+9], cmp1[8], cmp1neg[8],cmp2[5], cmp2neg[2]



//*******************************************************************************
//*										*
//*	for (i = 0; i < 64; i++) {						*
//*		acL=acLevel = data[i];						*
//*		acLevel = ((acLevel < 0)?-acLevel:acLevel) - quant_d_2;		*
//*		if (acLevel < quant_m_2){					*
//*			acLevel = 0;						*
//*		}								*
//*		acLevel = (acLevel * mult) >> SCALEBITS;			*
//*		sum += acLevel;							*
//*		coeff[i] = ((acL < 0)?-acLevel:acLevel);			*
//*	}									*		
//*										*	
//*******************************************************************************/ 



.explicit
.L58:
	.pred.rel "clear", p29, p37
	.pred.rel "mutex", p29, p37

									//pipeline stage
{.mmi
	(p[0]) 		ld2 ac1[0]   = [r15],2				//   0		acL=acLevel = data[i];
	(p[LL+1]) 	sub ac2[0]   = r0, ac1[LL+1]			//   LL+1	ac2=-acLevel
	(p[LL]) 	sxt2 ac1[LL] = ac1[LL]				//   LL
}
{.mmi
	(p[LL+1]) 	cmp4.le cmp1[0], cmp1neg[0] = r0, ac1[LL+1]	//   LL+1	cmp1 = (0<=acLevel)  ;   cmp1neg = !(0<=acLevel)
	(p[LL+4]) 	cmp4.le cmp2[0], cmp2neg[0] = r20, ac2[3]	//   LL+4	cmp2 = (quant_m_2 < acLevel)  ; cmp2neg = !(quant_m_2 < acLevel)
	(cmp1[1])    	sub ac2[1]   = ac1[LL+2], r21			//   LL+2	acLevel = acLevel - quant_d_2;
}
{.mmi
	(cmp2neg[1])	mov ac2[4] = r0					//   LL+5	if (acLevel < quant_m_2) acLevel=0;
	(cmp1neg[1]) 	sub ac2[1]   = ac2[1], r21			//   LL+2	acLevel = ac2 - quant_d_2;
	(p[LL+3]) 	sxt2 ac2[2]   = ac2[2]				//   LL+3
}
{.mmi
	.pred.rel "mutex", p34, p42
	(cmp1[6]) 	mov ac3[0] = ac2[6]				//   LL+7	ac3 = acLevel;
	(cmp1neg[6])	sub ac3[0] = r0, ac2[6]				//   LL+7	ac3 = -acLevel;
	(p[LL+6]) 	pmpyshr2.u ac2[5] = r29, ac2[5], 16		//   LL+6	acLevel = (acLevel * mult) >> SCALEBITS;
}
{.mib
	(p[LL+8]) 	st2 [r18] = ac3[1] , 2				//   LL+8	coeff[i] = ac3;
	(cmp2[4]) 	add r8 = r8, ac2[7]				//   LL+8	sum += acLevel;	
	br.ctop.sptk.few .L58
	;;
}

	.pred.rel "clear", p29, p37
.default
	mov ar.ec = r17
	;;
	mov ar.lc = r2
	mov pr = r10, -1
	mov ar.pfs = r9
	br.ret.sptk.many b0
	.endp quant_h263_inter_ia64#







// void dequant_h263_inter_ia64(int16_t *data, const int16_t *coeff, const uint32_t quant)

	.common	dequant_h263_inter#,8,8
	.align 16
	.global dequant_h263_inter_ia64#
	.proc dequant_h263_inter_ia64#
dequant_h263_inter_ia64:
	
//***********************************************************************
//*									*
//*	const uint16_t quant_m_2 = quant << 1;				*
//*	const uint16_t quant_add = (quant & 1 ? quant : quant - 1);	*
//*	uint32_t i;							*
//*									*		
//***********************************************************************
	
	
	
	
	.prologue
	andcm r14 = 1, r34
	dep.z r29 = r34, 1, 15
	alloc r9=ar.pfs,0,32,0,32
	.save ar.lc, r2
	mov r2 = ar.lc
	;;
	.body
	sub r15 = r34, r14		// r15 = quant
	addl r14 = 63, r0
	addl r21 = -2048, r0
	addl r20 = 2047, r0
	mov r16 = ar.ec
	mov r17 = pr
	;;
	zxt2 r15 = r15
	mov ar.lc = r14
	mov pr.rot = 0
	;;
	adds r14 = 0, r33		// r14 = coeff
	mov r18 = r32			// r18 = data
	mov ar.ec = LL+10
	mov pr.rot = 1 << 16
	;;

//*******************************************************************************
//*										*
//*for (i = 0; i < 64; i++) {							*
//*		int16_t acLevel = coeff[i];					*
//*										*		
//*		if (acLevel == 0)						*
//*		{								*
//*			data[i] = 0;						*
//*		}								*
//*		else if (acLevel < 0)						*
//*		{								*
//*			acLevel = acLevel * quant_m_2 - quant_add;		*
//*			data[i] = (acLevel >= -2048 ? acLevel : -2048);		*
//*		}								*
//*		else // if (acLevel > 0)					*
//*		{								*
//*			acLevel = acLevel * quant_m_2 + quant_add; 		*
//*			data[i] = (acLevel <= 2047 ? acLevel : 2047);		*
//*		}								*		
//*	}									*
//*										*	
//*******************************************************************************/


	
	LL=2	// LL := load latency
		//if LL is changed, you'll also have to change the .pred.rel... parts below!
	
	
	.rotr ac1[LL+10], x[5], y1[3], y2[3]
	.rotp p[LL+10] , cmp1neg[8], cmp2[5], cmp2neg[5],cmp3[2], cmp3neg[2]
	
.explicit	
								//pipeline stage
	
.L60:
	.pred.rel "clear", p36
	.pred.rel "mutex", p47, p49
	.pred.rel "mutex", p46, p48
	.pred.rel "mutex", p40, p45
	.pred.rel "mutex", p39, p44
	.pred.rel "mutex", p38, p43
	.pred.rel "mutex", p37, p42
	.pred.rel "mutex", p36, p41
{.mmi	
	(p[0])ld2 ac1[0] = [r14] ,2				//	0  	acLevel = coeff[i];
	(p[LL+1])cmp4.ne p6, cmp1neg[0] = 0, ac1[LL+1]		//	LL+1
	(p[LL])sxt2 ac1[LL] = ac1[LL]				//	LL

}
{.mmi
	(p[LL+1])cmp4.le cmp2[0], cmp2neg[0] = r0, ac1[LL+1]	//	LL+1
	(cmp2[1]) mov x[0] = r20				//	LL+2
	(p[LL+2])pmpyshr2.u ac1[LL+2] = r29, ac1[LL+2], 0	//	LL+2
}
{.mmi
	(cmp2neg[1]) mov x[0] = r21				//  	LL+2
	(cmp2[2]) add ac1[LL+3] = ac1[LL+3], r15		//	LL+3
	(cmp2neg[2]) sub ac1[LL+3] = ac1[LL+3], r15		//	LL+3

}
{.mmi
	(cmp2neg[4]) mov y1[0] = ac1[LL+5]			//	LL+5
	(cmp2neg[4]) mov y2[0] = x[3]				//	LL+5
	(p[LL+4])sxt2 ac1[LL+4] = ac1[LL+4]			//	LL+4
}
{.mmi
	(cmp2[4]) mov y1[0] = x[3]				//	LL+5
	(cmp2[4]) mov y2[0] = ac1[LL+5]				//	LL+5
	(p[LL+6])cmp4.le cmp3[0], cmp3neg[0] = x[4], ac1[LL+6]	//	LL+6
}
{.mmi
	(cmp3[1]) mov ac1[LL+7] = y1[2]				//	LL+7
	(cmp3neg[1]) mov ac1[LL+7] = y2[2]			//	LL+7
	(cmp1neg[7])  mov ac1[LL+8] = r0			//	LL+8
}
{.mbb
	(p[LL+9])st2 [r18] = ac1[LL+9] ,2			//	LL+9
	nop.b 0x0
	br.ctop.sptk.few .L60
	;;
}
	.pred.rel "clear", p36
.default
	mov ar.lc = r2
	mov ar.pfs = r9
	mov ar.ec  = r16
	mov pr = r17, -1
	;;
	mov ar.lc = r2
	br.ret.sptk.many b0
	.endp dequant_h263_inter_ia64#
	.ident	"GCC: (GNU) 2.96 20000731 (Red Hat Linux 7.1 2.96-85)"
