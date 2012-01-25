// ****************************************************************************
// *
// *  XVID MPEG-4 VIDEO CODEC
// *  - IA64 inverse discrete cosine transform -
// *
// *  Copyright(C) 2002 Christian Schwarz, Haiko Gaisser, Sebastian Hack
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
// * $Id: idct_ia64_ecc.s,v 1.1.14.1 2009/05/25 09:03:47 Isibaar Exp $
// *
// ***************************************************************************/
//
// ****************************************************************************
// *                                                                           
// *  idct_ia64_ecc.s, IA-64 optimized inverse DCT                             
// *                                                                           
// *  This version was implemented during an IA-64 practical training at 	   
// *  the University of Karlsruhe (http://i44w3.info.uni-karlsruhe.de/)		   
// *                                                                           
// ****************************************************************************
//

addreg1 = r14
addreg2 = r15
c0 = f32
c1 = f33
c2 = f34
c3 = f35
c4 = f36
c5 = f37
c6 = f38
c7 = f39
c8 = f40
c9 = f41
c10 = f42
c11 = f43
c12 = f44
c13 = f45
c14 = f46
c15 = f47
.sdata
.align 16
.data_c0:
real4 0.353553390593273730857504233427, 0.353553390593273730857504233427
.data_c1:
real4 -2.414213562373094923430016933708, -2.414213562373094923430016933708
.align 16
.data_c2:
real4 -0.414213562373095034452319396223, -0.414213562373095034452319396223
.data_c3:
real4 0.198912367379658006072418174881, 0.198912367379658006072418174881
.align 16
.data_c4:
real4 5.027339492125848074977056967327, 5.027339492125848074977056967327
.data_c5:
real4 0.668178637919298878955487452913, 0.668178637919298878955487452913
.align 16
.data_c6:
real4 1.496605762665489169904731170391, 1.496605762665489169904731170391
.data_c7:
real4 0.461939766255643369241568052530, 0.461939766255643369241568052530
.align 16
.data_c8:
real4 0.191341716182544890889616340246, 0.191341716182544890889616340246
.data_c9:
real4 0.847759065022573476966272210120, 0.847759065022573476966272210120
.align 16
.data_c10:
real4 2.847759065022573476966272210120, 2.847759065022573476966272210120
.data_c11:
real4 5.027339492125848074977056967327, 5.027339492125848074977056967327
.align 16
.data_c12:
real4 0.490392640201615215289621119155, 0.490392640201615215289621119155
.data_c13:
real4 0.068974844820735750627882509889, 0.068974844820735750627882509889
.align 16
.data_c14:
real4 0.097545161008064124041894160655, 0.097545161008064124041894160655
.data_c15:
real4 1.000000000000000000000000000000, 1.000000000000000000000000000000

.text
.global idct_ia64
.global idct_ia64_init
.align 16
.proc idct_ia64_init
idct_ia64_init:
br.ret.sptk.few b0
.endp
.align 16
.proc idct_ia64
idct_ia64:

	addreg3 = r20
	addreg4 = r21
	addreg5 = r22
	addreg6 = r23
	
	one = f30
	alloc   r16 = ar.pfs, 1, 71, 0, 0
	addl	addreg1 = @gprel(.data_c0#), gp
	addl	addreg2 = @gprel(.data_c2#), gp
	;;
	add	addreg3 = 32, addreg1
	add	addreg4 = 32, addreg2
	add	addreg5 = 64, addreg1
	add	addreg6 = 64, addreg2
	;; 
	ldfp8	c0, c1 = [addreg1]
	ldfp8	c2, c3 = [addreg2]
	;; 
	ldfp8	c4, c5 = [addreg3], 16
	ldfp8	c6, c7 = [addreg4], 16
	add	addreg1 = 96, addreg1
	add	addreg2 = 96, addreg2
	;;
	ldfp8	c8, c9 = [addreg5], 16
	ldfp8	c10, c11 = [addreg6], 16
	;; 
	ldfp8	c12, c13 = [addreg1]
	ldfp8	c14, c15 = [addreg2]
	;;
	mov	addreg1 = in0
	fpack	one = f1, f1
	add	addreg2 = 2, in0
	;; 

	ld2  r33 = [addreg1], 4
	ld2  r34 = [addreg2], 4
	;;
	ld2  r35 = [addreg1], 4
	ld2  r36 = [addreg2], 4
	;;
	ld2  r37 = [addreg1], 4
	ld2  r38 = [addreg2], 4
	;;
	ld2  r39 = [addreg1], 4
	ld2  r40 = [addreg2], 4
	;;
	ld2  r41 = [addreg1], 4
	ld2  r42 = [addreg2], 4
	;;
	ld2  r43 = [addreg1], 4
	ld2  r44 = [addreg2], 4
	;;
	ld2  r45 = [addreg1], 4
	ld2  r46 = [addreg2], 4
	;;
	ld2  r47 = [addreg1], 4
	ld2  r48 = [addreg2], 4
	;;
	ld2  r49 = [addreg1], 4
	ld2  r50 = [addreg2], 4
	;;
	ld2  r51 = [addreg1], 4
	ld2  r52 = [addreg2], 4
	;;
	ld2  r53 = [addreg1], 4
	ld2  r54 = [addreg2], 4
	;;
	ld2  r55 = [addreg1], 4
	ld2  r56 = [addreg2], 4
	;;
	ld2  r57 = [addreg1], 4
	ld2  r58 = [addreg2], 4
	;;
	ld2  r59 = [addreg1], 4
	ld2  r60 = [addreg2], 4
	;;
	ld2  r61 = [addreg1], 4
	ld2  r62 = [addreg2], 4
	;;
	ld2  r63 = [addreg1], 4
	ld2  r64 = [addreg2], 4
	;;
	ld2  r65 = [addreg1], 4
	ld2  r66 = [addreg2], 4
	;;
	ld2  r67 = [addreg1], 4
	ld2  r68 = [addreg2], 4
	;;
	ld2  r69 = [addreg1], 4
	ld2  r70 = [addreg2], 4
	;;
	ld2  r71 = [addreg1], 4
	ld2  r72 = [addreg2], 4
	;;
	ld2  r73 = [addreg1], 4
	ld2  r74 = [addreg2], 4
	;;
	ld2  r75 = [addreg1], 4
	ld2  r76 = [addreg2], 4
	;;
	ld2  r77 = [addreg1], 4
	ld2  r78 = [addreg2], 4
	;;
	ld2  r79 = [addreg1], 4
	ld2  r80 = [addreg2], 4
	;;
	ld2  r81 = [addreg1], 4
	ld2  r82 = [addreg2], 4
	;;
	ld2  r83 = [addreg1], 4
	ld2  r84 = [addreg2], 4
	;;
	ld2  r85 = [addreg1], 4
	ld2  r86 = [addreg2], 4
	;;
	ld2  r87 = [addreg1], 4
	ld2  r88 = [addreg2], 4
	;;
	ld2  r89 = [addreg1], 4
	ld2  r90 = [addreg2], 4
	;;
	ld2  r91 = [addreg1], 4
	ld2  r92 = [addreg2], 4
	;;
	ld2  r93 = [addreg1], 4
	ld2  r94 = [addreg2], 4
	;;
	ld2  r95 = [addreg1], 4
	ld2  r96 = [addreg2], 4
	;;
	sxt2  r33 = r33
	sxt2  r34 = r34
	sxt2  r35 = r35
	sxt2  r36 = r36
	sxt2  r37 = r37
	sxt2  r38 = r38
	sxt2  r39 = r39
	sxt2  r40 = r40
	sxt2  r41 = r41
	sxt2  r42 = r42
	sxt2  r43 = r43
	sxt2  r44 = r44
	sxt2  r45 = r45
	sxt2  r46 = r46
	sxt2  r47 = r47
	sxt2  r48 = r48
	sxt2  r49 = r49
	sxt2  r50 = r50
	sxt2  r51 = r51
	sxt2  r52 = r52
	sxt2  r53 = r53
	sxt2  r54 = r54
	sxt2  r55 = r55
	sxt2  r56 = r56
	sxt2  r57 = r57
	sxt2  r58 = r58
	sxt2  r59 = r59
	sxt2  r60 = r60
	sxt2  r61 = r61
	sxt2  r62 = r62
	sxt2  r63 = r63
	sxt2  r64 = r64
	sxt2  r65 = r65
	sxt2  r66 = r66
	sxt2  r67 = r67
	sxt2  r68 = r68
	sxt2  r69 = r69
	sxt2  r70 = r70
	sxt2  r71 = r71
	sxt2  r72 = r72
	sxt2  r73 = r73
	sxt2  r74 = r74
	sxt2  r75 = r75
	sxt2  r76 = r76
	sxt2  r77 = r77
	sxt2  r78 = r78
	sxt2  r79 = r79
	sxt2  r80 = r80
	sxt2  r81 = r81
	sxt2  r82 = r82
	sxt2  r83 = r83
	sxt2  r84 = r84
	sxt2  r85 = r85
	sxt2  r86 = r86
	sxt2  r87 = r87
	sxt2  r88 = r88
	sxt2  r89 = r89
	sxt2  r90 = r90
	sxt2  r91 = r91
	sxt2  r92 = r92
	sxt2  r93 = r93
	sxt2  r94 = r94
	sxt2  r95 = r95
	sxt2  r96 = r96
	;;
	setf.sig  f48 = r33
	setf.sig  f49 = r34
	setf.sig  f50 = r35
	setf.sig  f51 = r36
	setf.sig  f52 = r37
	setf.sig  f53 = r38
	setf.sig  f54 = r39
	setf.sig  f55 = r40
	setf.sig  f56 = r41
	setf.sig  f57 = r42
	setf.sig  f58 = r43
	setf.sig  f59 = r44
	setf.sig  f60 = r45
	setf.sig  f61 = r46
	setf.sig  f62 = r47
	setf.sig  f63 = r48
	setf.sig  f64 = r49
	setf.sig  f65 = r50
	setf.sig  f66 = r51
	setf.sig  f67 = r52
	setf.sig  f68 = r53
	setf.sig  f69 = r54
	setf.sig  f70 = r55
	setf.sig  f71 = r56
	setf.sig  f72 = r57
	setf.sig  f73 = r58
	setf.sig  f74 = r59
	setf.sig  f75 = r60
	setf.sig  f76 = r61
	setf.sig  f77 = r62
	setf.sig  f78 = r63
	setf.sig  f79 = r64
	setf.sig  f80 = r65
	setf.sig  f81 = r66
	setf.sig  f82 = r67
	setf.sig  f83 = r68
	setf.sig  f84 = r69
	setf.sig  f85 = r70
	setf.sig  f86 = r71
	setf.sig  f87 = r72
	setf.sig  f88 = r73
	setf.sig  f89 = r74
	setf.sig  f90 = r75
	setf.sig  f91 = r76
	setf.sig  f92 = r77
	setf.sig  f93 = r78
	setf.sig  f94 = r79
	setf.sig  f95 = r80
	setf.sig  f96 = r81
	setf.sig  f97 = r82
	setf.sig  f98 = r83
	setf.sig  f99 = r84
	setf.sig  f100 = r85
	setf.sig  f101 = r86
	setf.sig  f102 = r87
	setf.sig  f103 = r88
	setf.sig  f104 = r89
	setf.sig  f105 = r90
	setf.sig  f106 = r91
	setf.sig  f107 = r92
	setf.sig  f108 = r93
	setf.sig  f109 = r94
	setf.sig  f110 = r95
	setf.sig  f111 = r96
	;;
	fcvt.xf  f48 = f48
	fcvt.xf  f49 = f49
	fcvt.xf  f50 = f50
	fcvt.xf  f51 = f51
	fcvt.xf  f52 = f52
	fcvt.xf  f53 = f53
	fcvt.xf  f54 = f54
	fcvt.xf  f55 = f55
	fcvt.xf  f56 = f56
	fcvt.xf  f57 = f57
	fcvt.xf  f58 = f58
	fcvt.xf  f59 = f59
	fcvt.xf  f60 = f60
	fcvt.xf  f61 = f61
	fcvt.xf  f62 = f62
	fcvt.xf  f63 = f63
	fcvt.xf  f64 = f64
	fcvt.xf  f65 = f65
	fcvt.xf  f66 = f66
	fcvt.xf  f67 = f67
	fcvt.xf  f68 = f68
	fcvt.xf  f69 = f69
	fcvt.xf  f70 = f70
	fcvt.xf  f71 = f71
	fcvt.xf  f72 = f72
	fcvt.xf  f73 = f73
	fcvt.xf  f74 = f74
	fcvt.xf  f75 = f75
	fcvt.xf  f76 = f76
	fcvt.xf  f77 = f77
	fcvt.xf  f78 = f78
	fcvt.xf  f79 = f79
	fcvt.xf  f80 = f80
	fcvt.xf  f81 = f81
	fcvt.xf  f82 = f82
	fcvt.xf  f83 = f83
	fcvt.xf  f84 = f84
	fcvt.xf  f85 = f85
	fcvt.xf  f86 = f86
	fcvt.xf  f87 = f87
	fcvt.xf  f88 = f88
	fcvt.xf  f89 = f89
	fcvt.xf  f90 = f90
	fcvt.xf  f91 = f91
	fcvt.xf  f92 = f92
	fcvt.xf  f93 = f93
	fcvt.xf  f94 = f94
	fcvt.xf  f95 = f95
	fcvt.xf  f96 = f96
	fcvt.xf  f97 = f97
	fcvt.xf  f98 = f98
	fcvt.xf  f99 = f99
	fcvt.xf  f100 = f100
	fcvt.xf  f101 = f101
	fcvt.xf  f102 = f102
	fcvt.xf  f103 = f103
	fcvt.xf  f104 = f104
	fcvt.xf  f105 = f105
	fcvt.xf  f106 = f106
	fcvt.xf  f107 = f107
	fcvt.xf  f108 = f108
	fcvt.xf  f109 = f109
	fcvt.xf  f110 = f110
	fcvt.xf  f111 = f111
	;;
	fpack    f48 = f48, f49
	;;
	fpack    f49 = f50, f51
	;;
	fpack    f50 = f52, f53
	;;
	fpack    f51 = f54, f55
	;;
	fpack    f52 = f56, f57
	;;
	fpack    f53 = f58, f59
	;;
	fpack    f54 = f60, f61
	;;
	fpack    f55 = f62, f63
	;;
	fpack    f56 = f64, f65
	;;
	fpack    f57 = f66, f67
	;;
	fpack    f58 = f68, f69
	;;
	fpack    f59 = f70, f71
	;;
	fpack    f60 = f72, f73
	;;
	fpack    f61 = f74, f75
	;;
	fpack    f62 = f76, f77
	;;
	fpack    f63 = f78, f79
	;;
	fpack    f64 = f80, f81
	;;
	fpack    f65 = f82, f83
	;;
	fpack    f66 = f84, f85
	;;
	fpack    f67 = f86, f87
	;;
	fpack    f68 = f88, f89
	;;
	fpack    f69 = f90, f91
	;;
	fpack    f70 = f92, f93
	;;
	fpack    f71 = f94, f95
	;;
	fpack    f72 = f96, f97
	;;
	fpack    f73 = f98, f99
	;;
	fpack    f74 = f100, f101
	;;
	fpack    f75 = f102, f103
	;;
	fpack    f76 = f104, f105
	;;
	fpack    f77 = f106, f107
	;;
	fpack    f78 = f108, f109
	;;
	fpack    f79 = f110, f111
	;;
	fpma    f48 = f48, c0, f0
	fpma    f49 = f49, c0, f0
	fpma    f50 = f50, c0, f0
	fpma    f51 = f51, c0, f0
	;;

	// before pre shuffle
	//  48 49 50 51 
	//  52 53 54 55 
	//  56 57 58 59 
	//  60 61 62 63 
	//  64 65 66 67 
	//  68 69 70 71 
	//  72 73 74 75 
	//  76 77 78 79 

	// after pre shuffle
	//  48 49 50 51 
	//  64 53 54 55 
	//  56 57 58 59 
	//  72 61 62 63 
	//  52 65 66 67 
	//  76 69 70 71 
	//  60 73 74 75 
	//  68 77 78 79 
	// (f80, f64) = (f48, f64) $ (c0, c0), (line 0, 1)
	fpma    f80 = f64, c0, f48
	fpnma   f64 = f64, c0, f48
	;;
	// (f48, f72) = (f56, f72) $ (c1, c2), (line 2, 3)
	fpma    f48 = f72, c1, f56
	fpnma   f72 = f72, c2, f56
	;;
	// (f56, f76) = (f52, f76) $ (c3, c4), (line 4, 5)
	fpma    f56 = f76, c3, f52
	fpnma   f76 = f76, c4, f52
	;;
	// (f52, f68) = (f60, f68) $ (c5, c6), (line 6, 7)
	fpma    f52 = f68, c5, f60
	fpnma   f68 = f68, c6, f60
	;;
	;;
	// (f60, f72) = (f80, f72) $ (c7, c7), (line 0, 3)
	fpma    f60 = f72, c7, f80
	fpnma   f72 = f72, c7, f80
	;;
	// (f80, f48) = (f64, f48) $ (c8, c8), (line 1, 2)
	fpma    f80 = f48, c8, f64
	fpnma   f48 = f48, c8, f64
	;;
	// (f64, f52) = (f56, f52) $ (c9, c9), (line 4, 6)
	fpma    f64 = f52, c9, f56
	fpnma   f52 = f52, c9, f56
	;;
	// (f56, f68) = (f76, f68) $ (c10, c10), (line 5, 7)
	fpma    f56 = f68, c10, f76
	fpnma   f68 = f68, c10, f76
	;;
	;;
	// (f76, f52) = (f56, f52) $ (c11, c11), (line 5, 6)
	fpma    f76 = f52, c11, f56
	fpnma   f52 = f52, c11, f56
	;;
	// (f56, f64) = (f60, f64) $ (c12, c12), (line 0, 4)
	fpma    f56 = f64, c12, f60
	fpnma   f64 = f64, c12, f60
	;;
	// (f60, f68) = (f72, f68) $ (c14, c14), (line 3, 7)
	fpma    f60 = f68, c14, f72
	fpnma   f68 = f68, c14, f72
	;;
	;;
	// (f72, f76) = (f80, f76) $ (c13, c13), (line 1, 5)
	fpma    f72 = f76, c13, f80
	fpnma   f76 = f76, c13, f80
	;;
	// (f80, f52) = (f48, f52) $ (c13, c13), (line 2, 6)
	fpma    f80 = f52, c13, f48
	fpnma   f52 = f52, c13, f48
	;;

	// before post shuffle
	//  56 49 50 51 
	//  72 53 54 55 
	//  80 57 58 59 
	//  60 61 62 63 
	//  64 65 66 67 
	//  76 69 70 71 
	//  52 73 74 75 
	//  68 77 78 79 

	// after post shuffle
	//  56 49 50 51 
	//  72 53 54 55 
	//  52 57 58 59 
	//  60 61 62 63 
	//  68 65 66 67 
	//  80 69 70 71 
	//  76 73 74 75 
	//  64 77 78 79 

	// before pre shuffle
	//  56 49 50 51 
	//  72 53 54 55 
	//  52 57 58 59 
	//  60 61 62 63 
	//  68 65 66 67 
	//  80 69 70 71 
	//  76 73 74 75 
	//  64 77 78 79 

	// after pre shuffle
	//  56 49 50 51 
	//  72 65 54 55 
	//  52 57 58 59 
	//  60 73 62 63 
	//  68 53 66 67 
	//  80 77 70 71 
	//  76 61 74 75 
	//  64 69 78 79 
	// (f48, f65) = (f49, f65) $ (c0, c0), (line 0, 1)
	fpma    f48 = f65, c0, f49
	fpnma   f65 = f65, c0, f49
	;;
	// (f49, f73) = (f57, f73) $ (c1, c2), (line 2, 3)
	fpma    f49 = f73, c1, f57
	fpnma   f73 = f73, c2, f57
	;;
	// (f57, f77) = (f53, f77) $ (c3, c4), (line 4, 5)
	fpma    f57 = f77, c3, f53
	fpnma   f77 = f77, c4, f53
	;;
	// (f53, f69) = (f61, f69) $ (c5, c6), (line 6, 7)
	fpma    f53 = f69, c5, f61
	fpnma   f69 = f69, c6, f61
	;;
	;;
	// (f61, f73) = (f48, f73) $ (c7, c7), (line 0, 3)
	fpma    f61 = f73, c7, f48
	fpnma   f73 = f73, c7, f48
	;;
	// (f48, f49) = (f65, f49) $ (c8, c8), (line 1, 2)
	fpma    f48 = f49, c8, f65
	fpnma   f49 = f49, c8, f65
	;;
	// (f65, f53) = (f57, f53) $ (c9, c9), (line 4, 6)
	fpma    f65 = f53, c9, f57
	fpnma   f53 = f53, c9, f57
	;;
	// (f57, f69) = (f77, f69) $ (c10, c10), (line 5, 7)
	fpma    f57 = f69, c10, f77
	fpnma   f69 = f69, c10, f77
	;;
	;;
	// (f77, f53) = (f57, f53) $ (c11, c11), (line 5, 6)
	fpma    f77 = f53, c11, f57
	fpnma   f53 = f53, c11, f57
	;;
	// (f57, f65) = (f61, f65) $ (c12, c12), (line 0, 4)
	fpma    f57 = f65, c12, f61
	fpnma   f65 = f65, c12, f61
	;;
	// (f61, f69) = (f73, f69) $ (c14, c14), (line 3, 7)
	fpma    f61 = f69, c14, f73
	fpnma   f69 = f69, c14, f73
	;;
	;;
	// (f73, f77) = (f48, f77) $ (c13, c13), (line 1, 5)
	fpma    f73 = f77, c13, f48
	fpnma   f77 = f77, c13, f48
	;;
	// (f48, f53) = (f49, f53) $ (c13, c13), (line 2, 6)
	fpma    f48 = f53, c13, f49
	fpnma   f53 = f53, c13, f49
	;;

	// before post shuffle
	//  56 57 50 51 
	//  72 73 54 55 
	//  52 48 58 59 
	//  60 61 62 63 
	//  68 65 66 67 
	//  80 77 70 71 
	//  76 53 74 75 
	//  64 69 78 79 

	// after post shuffle
	//  56 57 50 51 
	//  72 73 54 55 
	//  52 53 58 59 
	//  60 61 62 63 
	//  68 69 66 67 
	//  80 48 70 71 
	//  76 77 74 75 
	//  64 65 78 79 

	// before pre shuffle
	//  56 57 50 51 
	//  72 73 54 55 
	//  52 53 58 59 
	//  60 61 62 63 
	//  68 69 66 67 
	//  80 48 70 71 
	//  76 77 74 75 
	//  64 65 78 79 

	// after pre shuffle
	//  56 57 50 51 
	//  72 73 66 55 
	//  52 53 58 59 
	//  60 61 74 63 
	//  68 69 54 67 
	//  80 48 78 71 
	//  76 77 62 75 
	//  64 65 70 79 
	// (f49, f66) = (f50, f66) $ (c0, c0), (line 0, 1)
	fpma    f49 = f66, c0, f50
	fpnma   f66 = f66, c0, f50
	;;
	// (f50, f74) = (f58, f74) $ (c1, c2), (line 2, 3)
	fpma    f50 = f74, c1, f58
	fpnma   f74 = f74, c2, f58
	;;
	// (f58, f78) = (f54, f78) $ (c3, c4), (line 4, 5)
	fpma    f58 = f78, c3, f54
	fpnma   f78 = f78, c4, f54
	;;
	// (f54, f70) = (f62, f70) $ (c5, c6), (line 6, 7)
	fpma    f54 = f70, c5, f62
	fpnma   f70 = f70, c6, f62
	;;
	;;
	// (f62, f74) = (f49, f74) $ (c7, c7), (line 0, 3)
	fpma    f62 = f74, c7, f49
	fpnma   f74 = f74, c7, f49
	;;
	// (f49, f50) = (f66, f50) $ (c8, c8), (line 1, 2)
	fpma    f49 = f50, c8, f66
	fpnma   f50 = f50, c8, f66
	;;
	// (f66, f54) = (f58, f54) $ (c9, c9), (line 4, 6)
	fpma    f66 = f54, c9, f58
	fpnma   f54 = f54, c9, f58
	;;
	// (f58, f70) = (f78, f70) $ (c10, c10), (line 5, 7)
	fpma    f58 = f70, c10, f78
	fpnma   f70 = f70, c10, f78
	;;
	;;
	// (f78, f54) = (f58, f54) $ (c11, c11), (line 5, 6)
	fpma    f78 = f54, c11, f58
	fpnma   f54 = f54, c11, f58
	;;
	// (f58, f66) = (f62, f66) $ (c12, c12), (line 0, 4)
	fpma    f58 = f66, c12, f62
	fpnma   f66 = f66, c12, f62
	;;
	// (f62, f70) = (f74, f70) $ (c14, c14), (line 3, 7)
	fpma    f62 = f70, c14, f74
	fpnma   f70 = f70, c14, f74
	;;
	;;
	// (f74, f78) = (f49, f78) $ (c13, c13), (line 1, 5)
	fpma    f74 = f78, c13, f49
	fpnma   f78 = f78, c13, f49
	;;
	// (f49, f54) = (f50, f54) $ (c13, c13), (line 2, 6)
	fpma    f49 = f54, c13, f50
	fpnma   f54 = f54, c13, f50
	;;

	// before post shuffle
	//  56 57 58 51 
	//  72 73 74 55 
	//  52 53 49 59 
	//  60 61 62 63 
	//  68 69 66 67 
	//  80 48 78 71 
	//  76 77 54 75 
	//  64 65 70 79 

	// after post shuffle
	//  56 57 58 51 
	//  72 73 74 55 
	//  52 53 54 59 
	//  60 61 62 63 
	//  68 69 70 67 
	//  80 48 49 71 
	//  76 77 78 75 
	//  64 65 66 79 

	// before pre shuffle
	//  56 57 58 51 
	//  72 73 74 55 
	//  52 53 54 59 
	//  60 61 62 63 
	//  68 69 70 67 
	//  80 48 49 71 
	//  76 77 78 75 
	//  64 65 66 79 

	// after pre shuffle
	//  56 57 58 51 
	//  72 73 74 67 
	//  52 53 54 59 
	//  60 61 62 75 
	//  68 69 70 55 
	//  80 48 49 79 
	//  76 77 78 63 
	//  64 65 66 71 
	// (f50, f67) = (f51, f67) $ (c0, c0), (line 0, 1)
	fpma    f50 = f67, c0, f51
	fpnma   f67 = f67, c0, f51
	;;
	// (f51, f75) = (f59, f75) $ (c1, c2), (line 2, 3)
	fpma    f51 = f75, c1, f59
	fpnma   f75 = f75, c2, f59
	;;
	// (f59, f79) = (f55, f79) $ (c3, c4), (line 4, 5)
	fpma    f59 = f79, c3, f55
	fpnma   f79 = f79, c4, f55
	;;
	// (f55, f71) = (f63, f71) $ (c5, c6), (line 6, 7)
	fpma    f55 = f71, c5, f63
	fpnma   f71 = f71, c6, f63
	;;
	;;
	// (f63, f75) = (f50, f75) $ (c7, c7), (line 0, 3)
	fpma    f63 = f75, c7, f50
	fpnma   f75 = f75, c7, f50
	;;
	// (f50, f51) = (f67, f51) $ (c8, c8), (line 1, 2)
	fpma    f50 = f51, c8, f67
	fpnma   f51 = f51, c8, f67
	;;
	// (f67, f55) = (f59, f55) $ (c9, c9), (line 4, 6)
	fpma    f67 = f55, c9, f59
	fpnma   f55 = f55, c9, f59
	;;
	// (f59, f71) = (f79, f71) $ (c10, c10), (line 5, 7)
	fpma    f59 = f71, c10, f79
	fpnma   f71 = f71, c10, f79
	;;
	;;
	// (f79, f55) = (f59, f55) $ (c11, c11), (line 5, 6)
	fpma    f79 = f55, c11, f59
	fpnma   f55 = f55, c11, f59
	;;
	// (f59, f67) = (f63, f67) $ (c12, c12), (line 0, 4)
	fpma    f59 = f67, c12, f63
	fpnma   f67 = f67, c12, f63
	;;
	// (f63, f71) = (f75, f71) $ (c14, c14), (line 3, 7)
	fpma    f63 = f71, c14, f75
	fpnma   f71 = f71, c14, f75
	;;
	;;
	// (f75, f79) = (f50, f79) $ (c13, c13), (line 1, 5)
	fpma    f75 = f79, c13, f50
	fpnma   f79 = f79, c13, f50
	;;
	// (f50, f55) = (f51, f55) $ (c13, c13), (line 2, 6)
	fpma    f50 = f55, c13, f51
	fpnma   f55 = f55, c13, f51
	;;

	// before post shuffle
	//  56 57 58 59 
	//  72 73 74 75 
	//  52 53 54 50 
	//  60 61 62 63 
	//  68 69 70 67 
	//  80 48 49 79 
	//  76 77 78 55 
	//  64 65 66 71 

	// after post shuffle
	//  56 57 58 59 
	//  72 73 74 75 
	//  52 53 54 55 
	//  60 61 62 63 
	//  68 69 70 71 
	//  80 48 49 50 
	//  76 77 78 79 
	//  64 65 66 67 
	;;
	fmix.r  f51 = f56, f72
	fmix.r  f81 = f57, f73
	fmix.r  f82 = f58, f74
	fmix.r  f83 = f59, f75
	fmix.r  f84 = f52, f60
	fmix.r  f85 = f53, f61
	fmix.r  f86 = f54, f62
	fmix.r  f87 = f55, f63
	fmix.r  f88 = f68, f80
	fmix.r  f89 = f69, f48
	fmix.r  f90 = f70, f49
	fmix.r  f91 = f71, f50
	fmix.r  f92 = f76, f64
	fmix.r  f93 = f77, f65
	fmix.r  f94 = f78, f66
	fmix.r  f95 = f79, f67
	;;
	fmix.l  f56 = f56, f72
	fmix.l  f57 = f57, f73
	fmix.l  f58 = f58, f74
	fmix.l  f59 = f59, f75
	fmix.l  f52 = f52, f60
	fmix.l  f53 = f53, f61
	fmix.l  f54 = f54, f62
	fmix.l  f55 = f55, f63
	fmix.l  f68 = f68, f80
	fmix.l  f69 = f69, f48
	fmix.l  f70 = f70, f49
	fmix.l  f71 = f71, f50
	fmix.l  f76 = f76, f64
	fmix.l  f77 = f77, f65
	fmix.l  f78 = f78, f66
	fmix.l  f79 = f79, f67
	;;
	fpma    f56 = f56, c0, f0
	fpma    f52 = f52, c0, f0
	fpma    f68 = f68, c0, f0
	fpma    f76 = f76, c0, f0
	;;

	// before pre shuffle
	//  56 52 68 76 
	//  51 84 88 92 
	//  57 53 69 77 
	//  81 85 89 93 
	//  58 54 70 78 
	//  82 86 90 94 
	//  59 55 71 79 
	//  83 87 91 95 

	// after pre shuffle
	//  56 52 68 76 
	//  58 84 88 92 
	//  57 53 69 77 
	//  59 85 89 93 
	//  51 54 70 78 
	//  83 86 90 94 
	//  81 55 71 79 
	//  82 87 91 95 
	// (f48, f58) = (f56, f58) $ (c0, c0), (line 0, 1)
	fpma    f48 = f58, c0, f56
	fpnma   f58 = f58, c0, f56
	;;
	// (f49, f59) = (f57, f59) $ (c1, c2), (line 2, 3)
	fpma    f49 = f59, c1, f57
	fpnma   f59 = f59, c2, f57
	;;
	// (f50, f83) = (f51, f83) $ (c3, c4), (line 4, 5)
	fpma    f50 = f83, c3, f51
	fpnma   f83 = f83, c4, f51
	;;
	// (f51, f82) = (f81, f82) $ (c5, c6), (line 6, 7)
	fpma    f51 = f82, c5, f81
	fpnma   f82 = f82, c6, f81
	;;
	;;
	// (f56, f59) = (f48, f59) $ (c7, c7), (line 0, 3)
	fpma    f56 = f59, c7, f48
	fpnma   f59 = f59, c7, f48
	;;
	// (f48, f49) = (f58, f49) $ (c8, c8), (line 1, 2)
	fpma    f48 = f49, c8, f58
	fpnma   f49 = f49, c8, f58
	;;
	// (f57, f51) = (f50, f51) $ (c9, c9), (line 4, 6)
	fpma    f57 = f51, c9, f50
	fpnma   f51 = f51, c9, f50
	;;
	// (f50, f82) = (f83, f82) $ (c10, c10), (line 5, 7)
	fpma    f50 = f82, c10, f83
	fpnma   f82 = f82, c10, f83
	;;
	;;
	// (f58, f51) = (f50, f51) $ (c11, c11), (line 5, 6)
	fpma    f58 = f51, c11, f50
	fpnma   f51 = f51, c11, f50
	;;
	// (f50, f57) = (f56, f57) $ (c12, c12), (line 0, 4)
	fpma    f50 = f57, c12, f56
	fpnma   f57 = f57, c12, f56
	;;
	// (f56, f82) = (f59, f82) $ (c14, c14), (line 3, 7)
	fpma    f56 = f82, c14, f59
	fpnma   f82 = f82, c14, f59
	;;
	;;
	// (f59, f58) = (f48, f58) $ (c13, c13), (line 1, 5)
	fpma    f59 = f58, c13, f48
	fpnma   f58 = f58, c13, f48
	;;
	// (f48, f51) = (f49, f51) $ (c13, c13), (line 2, 6)
	fpma    f48 = f51, c13, f49
	fpnma   f51 = f51, c13, f49
	;;

	// before post shuffle
	//  50 52 68 76 
	//  59 84 88 92 
	//  48 53 69 77 
	//  56 85 89 93 
	//  57 54 70 78 
	//  58 86 90 94 
	//  51 55 71 79 
	//  82 87 91 95 

	// after post shuffle
	//  50 52 68 76 
	//  59 84 88 92 
	//  51 53 69 77 
	//  56 85 89 93 
	//  82 54 70 78 
	//  48 86 90 94 
	//  58 55 71 79 
	//  57 87 91 95 

	// before pre shuffle
	//  50 52 68 76 
	//  59 84 88 92 
	//  51 53 69 77 
	//  56 85 89 93 
	//  82 54 70 78 
	//  48 86 90 94 
	//  58 55 71 79 
	//  57 87 91 95 

	// after pre shuffle
	//  50 52 68 76 
	//  59 54 88 92 
	//  51 53 69 77 
	//  56 55 89 93 
	//  82 84 70 78 
	//  48 87 90 94 
	//  58 85 71 79 
	//  57 86 91 95 
	// (f49, f54) = (f52, f54) $ (c0, c0), (line 0, 1)
	fpma    f49 = f54, c0, f52
	fpnma   f54 = f54, c0, f52
	;;
	// (f52, f55) = (f53, f55) $ (c1, c2), (line 2, 3)
	fpma    f52 = f55, c1, f53
	fpnma   f55 = f55, c2, f53
	;;
	// (f53, f87) = (f84, f87) $ (c3, c4), (line 4, 5)
	fpma    f53 = f87, c3, f84
	fpnma   f87 = f87, c4, f84
	;;
	// (f60, f86) = (f85, f86) $ (c5, c6), (line 6, 7)
	fpma    f60 = f86, c5, f85
	fpnma   f86 = f86, c6, f85
	;;
	;;
	// (f61, f55) = (f49, f55) $ (c7, c7), (line 0, 3)
	fpma    f61 = f55, c7, f49
	fpnma   f55 = f55, c7, f49
	;;
	// (f49, f52) = (f54, f52) $ (c8, c8), (line 1, 2)
	fpma    f49 = f52, c8, f54
	fpnma   f52 = f52, c8, f54
	;;
	// (f54, f60) = (f53, f60) $ (c9, c9), (line 4, 6)
	fpma    f54 = f60, c9, f53
	fpnma   f60 = f60, c9, f53
	;;
	// (f53, f86) = (f87, f86) $ (c10, c10), (line 5, 7)
	fpma    f53 = f86, c10, f87
	fpnma   f86 = f86, c10, f87
	;;
	;;
	// (f62, f60) = (f53, f60) $ (c11, c11), (line 5, 6)
	fpma    f62 = f60, c11, f53
	fpnma   f60 = f60, c11, f53
	;;
	// (f53, f54) = (f61, f54) $ (c12, c12), (line 0, 4)
	fpma    f53 = f54, c12, f61
	fpnma   f54 = f54, c12, f61
	;;
	// (f61, f86) = (f55, f86) $ (c14, c14), (line 3, 7)
	fpma    f61 = f86, c14, f55
	fpnma   f86 = f86, c14, f55
	;;
	;;
	// (f55, f62) = (f49, f62) $ (c13, c13), (line 1, 5)
	fpma    f55 = f62, c13, f49
	fpnma   f62 = f62, c13, f49
	;;
	// (f49, f60) = (f52, f60) $ (c13, c13), (line 2, 6)
	fpma    f49 = f60, c13, f52
	fpnma   f60 = f60, c13, f52
	;;

	// before post shuffle
	//  50 53 68 76 
	//  59 55 88 92 
	//  51 49 69 77 
	//  56 61 89 93 
	//  82 54 70 78 
	//  48 62 90 94 
	//  58 60 71 79 
	//  57 86 91 95 

	// after post shuffle
	//  50 53 68 76 
	//  59 55 88 92 
	//  51 60 69 77 
	//  56 61 89 93 
	//  82 86 70 78 
	//  48 49 90 94 
	//  58 62 71 79 
	//  57 54 91 95 

	// before pre shuffle
	//  50 53 68 76 
	//  59 55 88 92 
	//  51 60 69 77 
	//  56 61 89 93 
	//  82 86 70 78 
	//  48 49 90 94 
	//  58 62 71 79 
	//  57 54 91 95 

	// after pre shuffle
	//  50 53 68 76 
	//  59 55 70 92 
	//  51 60 69 77 
	//  56 61 71 93 
	//  82 86 88 78 
	//  48 49 91 94 
	//  58 62 89 79 
	//  57 54 90 95 
	// (f52, f70) = (f68, f70) $ (c0, c0), (line 0, 1)
	fpma    f52 = f70, c0, f68
	fpnma   f70 = f70, c0, f68
	;;
	// (f63, f71) = (f69, f71) $ (c1, c2), (line 2, 3)
	fpma    f63 = f71, c1, f69
	fpnma   f71 = f71, c2, f69
	;;
	// (f64, f91) = (f88, f91) $ (c3, c4), (line 4, 5)
	fpma    f64 = f91, c3, f88
	fpnma   f91 = f91, c4, f88
	;;
	// (f65, f90) = (f89, f90) $ (c5, c6), (line 6, 7)
	fpma    f65 = f90, c5, f89
	fpnma   f90 = f90, c6, f89
	;;
	;;
	// (f66, f71) = (f52, f71) $ (c7, c7), (line 0, 3)
	fpma    f66 = f71, c7, f52
	fpnma   f71 = f71, c7, f52
	;;
	// (f52, f63) = (f70, f63) $ (c8, c8), (line 1, 2)
	fpma    f52 = f63, c8, f70
	fpnma   f63 = f63, c8, f70
	;;
	// (f67, f65) = (f64, f65) $ (c9, c9), (line 4, 6)
	fpma    f67 = f65, c9, f64
	fpnma   f65 = f65, c9, f64
	;;
	// (f64, f90) = (f91, f90) $ (c10, c10), (line 5, 7)
	fpma    f64 = f90, c10, f91
	fpnma   f90 = f90, c10, f91
	;;
	;;
	// (f68, f65) = (f64, f65) $ (c11, c11), (line 5, 6)
	fpma    f68 = f65, c11, f64
	fpnma   f65 = f65, c11, f64
	;;
	// (f64, f67) = (f66, f67) $ (c12, c12), (line 0, 4)
	fpma    f64 = f67, c12, f66
	fpnma   f67 = f67, c12, f66
	;;
	// (f66, f90) = (f71, f90) $ (c14, c14), (line 3, 7)
	fpma    f66 = f90, c14, f71
	fpnma   f90 = f90, c14, f71
	;;
	;;
	// (f69, f68) = (f52, f68) $ (c13, c13), (line 1, 5)
	fpma    f69 = f68, c13, f52
	fpnma   f68 = f68, c13, f52
	;;
	// (f52, f65) = (f63, f65) $ (c13, c13), (line 2, 6)
	fpma    f52 = f65, c13, f63
	fpnma   f65 = f65, c13, f63
	;;

	// before post shuffle
	//  50 53 64 76 
	//  59 55 69 92 
	//  51 60 52 77 
	//  56 61 66 93 
	//  82 86 67 78 
	//  48 49 68 94 
	//  58 62 65 79 
	//  57 54 90 95 

	// after post shuffle
	//  50 53 64 76 
	//  59 55 69 92 
	//  51 60 65 77 
	//  56 61 66 93 
	//  82 86 90 78 
	//  48 49 52 94 
	//  58 62 68 79 
	//  57 54 67 95 

	// before pre shuffle
	//  50 53 64 76 
	//  59 55 69 92 
	//  51 60 65 77 
	//  56 61 66 93 
	//  82 86 90 78 
	//  48 49 52 94 
	//  58 62 68 79 
	//  57 54 67 95 

	// after pre shuffle
	//  50 53 64 76 
	//  59 55 69 78 
	//  51 60 65 77 
	//  56 61 66 79 
	//  82 86 90 92 
	//  48 49 52 95 
	//  58 62 68 93 
	//  57 54 67 94 
	// (f63, f78) = (f76, f78) $ (c0, c0), (line 0, 1)
	fpma    f63 = f78, c0, f76
	fpnma   f78 = f78, c0, f76
	;;
	// (f70, f79) = (f77, f79) $ (c1, c2), (line 2, 3)
	fpma    f70 = f79, c1, f77
	fpnma   f79 = f79, c2, f77
	;;
	// (f71, f95) = (f92, f95) $ (c3, c4), (line 4, 5)
	fpma    f71 = f95, c3, f92
	fpnma   f95 = f95, c4, f92
	;;
	// (f72, f94) = (f93, f94) $ (c5, c6), (line 6, 7)
	fpma    f72 = f94, c5, f93
	fpnma   f94 = f94, c6, f93
	;;
	;;
	// (f73, f79) = (f63, f79) $ (c7, c7), (line 0, 3)
	fpma    f73 = f79, c7, f63
	fpnma   f79 = f79, c7, f63
	;;
	// (f63, f70) = (f78, f70) $ (c8, c8), (line 1, 2)
	fpma    f63 = f70, c8, f78
	fpnma   f70 = f70, c8, f78
	;;
	// (f74, f72) = (f71, f72) $ (c9, c9), (line 4, 6)
	fpma    f74 = f72, c9, f71
	fpnma   f72 = f72, c9, f71
	;;
	// (f71, f94) = (f95, f94) $ (c10, c10), (line 5, 7)
	fpma    f71 = f94, c10, f95
	fpnma   f94 = f94, c10, f95
	;;
	;;
	// (f75, f72) = (f71, f72) $ (c11, c11), (line 5, 6)
	fpma    f75 = f72, c11, f71
	fpnma   f72 = f72, c11, f71
	;;
	// (f71, f74) = (f73, f74) $ (c12, c12), (line 0, 4)
	fpma    f71 = f74, c12, f73
	fpnma   f74 = f74, c12, f73
	;;
	// (f73, f94) = (f79, f94) $ (c14, c14), (line 3, 7)
	fpma    f73 = f94, c14, f79
	fpnma   f94 = f94, c14, f79
	;;
	;;
	// (f76, f75) = (f63, f75) $ (c13, c13), (line 1, 5)
	fpma    f76 = f75, c13, f63
	fpnma   f75 = f75, c13, f63
	;;
	// (f63, f72) = (f70, f72) $ (c13, c13), (line 2, 6)
	fpma    f63 = f72, c13, f70
	fpnma   f72 = f72, c13, f70
	;;

	// before post shuffle
	//  50 53 64 71 
	//  59 55 69 76 
	//  51 60 65 63 
	//  56 61 66 73 
	//  82 86 90 74 
	//  48 49 52 75 
	//  58 62 68 72 
	//  57 54 67 94 

	// after post shuffle
	//  50 53 64 71 
	//  59 55 69 76 
	//  51 60 65 72 
	//  56 61 66 73 
	//  82 86 90 94 
	//  48 49 52 63 
	//  58 62 68 75 
	//  57 54 67 74 
	;;
	fmix.r  f70 = f50, f59
	fmix.r  f77 = f53, f55
	fmix.r  f78 = f64, f69
	fmix.r  f79 = f71, f76
	fmix.r  f80 = f51, f56
	fmix.r  f81 = f60, f61
	fmix.r  f83 = f65, f66
	fmix.r  f84 = f72, f73
	fmix.r  f85 = f82, f48
	fmix.r  f87 = f86, f49
	fmix.r  f88 = f90, f52
	fmix.r  f89 = f94, f63
	fmix.r  f91 = f58, f57
	fmix.r  f92 = f62, f54
	fmix.r  f93 = f68, f67
	fmix.r  f95 = f75, f74
	;;
	fmix.l  f50 = f50, f59
	fmix.l  f53 = f53, f55
	fmix.l  f64 = f64, f69
	fmix.l  f71 = f71, f76
	fmix.l  f51 = f51, f56
	fmix.l  f60 = f60, f61
	fmix.l  f65 = f65, f66
	fmix.l  f72 = f72, f73
	fmix.l  f82 = f82, f48
	fmix.l  f86 = f86, f49
	fmix.l  f90 = f90, f52
	fmix.l  f94 = f94, f63
	fmix.l  f58 = f58, f57
	fmix.l  f62 = f62, f54
	fmix.l  f68 = f68, f67
	fmix.l  f75 = f75, f74
	;;
	//  50 51 82 58 
	//  70 80 85 91 
	//  53 60 86 62 
	//  77 81 87 92 
	//  64 65 90 68 
	//  78 83 88 93 
	//  71 72 94 75 
	//  79 84 89 95 
	mov   addreg1 = in0
	add   addreg2 = 4, in0
	;;
	fpcvt.fx f50 = f50
	fpcvt.fx f51 = f51
	fpcvt.fx f82 = f82
	fpcvt.fx f58 = f58
	fpcvt.fx f70 = f70
	fpcvt.fx f80 = f80
	fpcvt.fx f85 = f85
	fpcvt.fx f91 = f91
	fpcvt.fx f53 = f53
	fpcvt.fx f60 = f60
	fpcvt.fx f86 = f86
	fpcvt.fx f62 = f62
	fpcvt.fx f77 = f77
	fpcvt.fx f81 = f81
	fpcvt.fx f87 = f87
	fpcvt.fx f92 = f92
	fpcvt.fx f64 = f64
	fpcvt.fx f65 = f65
	fpcvt.fx f90 = f90
	fpcvt.fx f68 = f68
	fpcvt.fx f78 = f78
	fpcvt.fx f83 = f83
	fpcvt.fx f88 = f88
	fpcvt.fx f93 = f93
	fpcvt.fx f71 = f71
	fpcvt.fx f72 = f72
	fpcvt.fx f94 = f94
	fpcvt.fx f75 = f75
	fpcvt.fx f79 = f79
	fpcvt.fx f84 = f84
	fpcvt.fx f89 = f89
	fpcvt.fx f95 = f95
	;;
	getf.sig r33 = f50
	getf.sig r34 = f51
	getf.sig r35 = f82
	getf.sig r36 = f58
	getf.sig r37 = f70
	getf.sig r38 = f80
	getf.sig r39 = f85
	getf.sig r40 = f91
	getf.sig r41 = f53
	getf.sig r42 = f60
	getf.sig r43 = f86
	getf.sig r44 = f62
	getf.sig r45 = f77
	getf.sig r46 = f81
	getf.sig r47 = f87
	getf.sig r48 = f92
	getf.sig r49 = f64
	getf.sig r50 = f65
	getf.sig r51 = f90
	getf.sig r52 = f68
	getf.sig r53 = f78
	getf.sig r54 = f83
	getf.sig r55 = f88
	getf.sig r56 = f93
	getf.sig r57 = f71
	getf.sig r58 = f72
	getf.sig r59 = f94
	getf.sig r60 = f75
	getf.sig r61 = f79
	getf.sig r62 = f84
	getf.sig r63 = f89
	getf.sig r64 = f95
	;;
	shl      r33 = r33, 7
	shl      r34 = r34, 7
	shl      r35 = r35, 7
	shl      r36 = r36, 7
	shl      r37 = r37, 7
	shl      r38 = r38, 7
	shl      r39 = r39, 7
	shl      r40 = r40, 7
	shl      r41 = r41, 7
	shl      r42 = r42, 7
	shl      r43 = r43, 7
	shl      r44 = r44, 7
	shl      r45 = r45, 7
	shl      r46 = r46, 7
	shl      r47 = r47, 7
	shl      r48 = r48, 7
	shl      r49 = r49, 7
	shl      r50 = r50, 7
	shl      r51 = r51, 7
	shl      r52 = r52, 7
	shl      r53 = r53, 7
	shl      r54 = r54, 7
	shl      r55 = r55, 7
	shl      r56 = r56, 7
	shl      r57 = r57, 7
	shl      r58 = r58, 7
	shl      r59 = r59, 7
	shl      r60 = r60, 7
	shl      r61 = r61, 7
	shl      r62 = r62, 7
	shl      r63 = r63, 7
	shl      r64 = r64, 7
	;;
	pack4.sss r33 = r33, r0
	pack4.sss r34 = r34, r0
	pack4.sss r35 = r35, r0
	pack4.sss r36 = r36, r0
	pack4.sss r37 = r37, r0
	pack4.sss r38 = r38, r0
	pack4.sss r39 = r39, r0
	pack4.sss r40 = r40, r0
	pack4.sss r41 = r41, r0
	pack4.sss r42 = r42, r0
	pack4.sss r43 = r43, r0
	pack4.sss r44 = r44, r0
	pack4.sss r45 = r45, r0
	pack4.sss r46 = r46, r0
	pack4.sss r47 = r47, r0
	pack4.sss r48 = r48, r0
	pack4.sss r49 = r49, r0
	pack4.sss r50 = r50, r0
	pack4.sss r51 = r51, r0
	pack4.sss r52 = r52, r0
	pack4.sss r53 = r53, r0
	pack4.sss r54 = r54, r0
	pack4.sss r55 = r55, r0
	pack4.sss r56 = r56, r0
	pack4.sss r57 = r57, r0
	pack4.sss r58 = r58, r0
	pack4.sss r59 = r59, r0
	pack4.sss r60 = r60, r0
	pack4.sss r61 = r61, r0
	pack4.sss r62 = r62, r0
	pack4.sss r63 = r63, r0
	pack4.sss r64 = r64, r0
	;;
	pshr2    r33 = r33, 7
	pshr2    r34 = r34, 7
	pshr2    r35 = r35, 7
	pshr2    r36 = r36, 7
	pshr2    r37 = r37, 7
	pshr2    r38 = r38, 7
	pshr2    r39 = r39, 7
	pshr2    r40 = r40, 7
	pshr2    r41 = r41, 7
	pshr2    r42 = r42, 7
	pshr2    r43 = r43, 7
	pshr2    r44 = r44, 7
	pshr2    r45 = r45, 7
	pshr2    r46 = r46, 7
	pshr2    r47 = r47, 7
	pshr2    r48 = r48, 7
	pshr2    r49 = r49, 7
	pshr2    r50 = r50, 7
	pshr2    r51 = r51, 7
	pshr2    r52 = r52, 7
	pshr2    r53 = r53, 7
	pshr2    r54 = r54, 7
	pshr2    r55 = r55, 7
	pshr2    r56 = r56, 7
	pshr2    r57 = r57, 7
	pshr2    r58 = r58, 7
	pshr2    r59 = r59, 7
	pshr2    r60 = r60, 7
	pshr2    r61 = r61, 7
	pshr2    r62 = r62, 7
	pshr2    r63 = r63, 7
	pshr2    r64 = r64, 7
	;;
	mux2     r33 = r33, 0xe1
	mux2     r34 = r34, 0xe1
	mux2     r35 = r35, 0xe1
	mux2     r36 = r36, 0xe1
	mux2     r37 = r37, 0xe1
	mux2     r38 = r38, 0xe1
	mux2     r39 = r39, 0xe1
	mux2     r40 = r40, 0xe1
	mux2     r41 = r41, 0xe1
	mux2     r42 = r42, 0xe1
	mux2     r43 = r43, 0xe1
	mux2     r44 = r44, 0xe1
	mux2     r45 = r45, 0xe1
	mux2     r46 = r46, 0xe1
	mux2     r47 = r47, 0xe1
	mux2     r48 = r48, 0xe1
	mux2     r49 = r49, 0xe1
	mux2     r50 = r50, 0xe1
	mux2     r51 = r51, 0xe1
	mux2     r52 = r52, 0xe1
	mux2     r53 = r53, 0xe1
	mux2     r54 = r54, 0xe1
	mux2     r55 = r55, 0xe1
	mux2     r56 = r56, 0xe1
	mux2     r57 = r57, 0xe1
	mux2     r58 = r58, 0xe1
	mux2     r59 = r59, 0xe1
	mux2     r60 = r60, 0xe1
	mux2     r61 = r61, 0xe1
	mux2     r62 = r62, 0xe1
	mux2     r63 = r63, 0xe1
	mux2     r64 = r64, 0xe1
	;;
	st4   [addreg1] = r33, 8
	st4   [addreg2] = r34, 8
	;;
	st4   [addreg1] = r35, 8
	st4   [addreg2] = r36, 8
	;;
	st4   [addreg1] = r37, 8
	st4   [addreg2] = r38, 8
	;;
	st4   [addreg1] = r39, 8
	st4   [addreg2] = r40, 8
	;;
	st4   [addreg1] = r41, 8
	st4   [addreg2] = r42, 8
	;;
	st4   [addreg1] = r43, 8
	st4   [addreg2] = r44, 8
	;;
	st4   [addreg1] = r45, 8
	st4   [addreg2] = r46, 8
	;;
	st4   [addreg1] = r47, 8
	st4   [addreg2] = r48, 8
	;;
	st4   [addreg1] = r49, 8
	st4   [addreg2] = r50, 8
	;;
	st4   [addreg1] = r51, 8
	st4   [addreg2] = r52, 8
	;;
	st4   [addreg1] = r53, 8
	st4   [addreg2] = r54, 8
	;;
	st4   [addreg1] = r55, 8
	st4   [addreg2] = r56, 8
	;;
	st4   [addreg1] = r57, 8
	st4   [addreg2] = r58, 8
	;;
	st4   [addreg1] = r59, 8
	st4   [addreg2] = r60, 8
	;;
	st4   [addreg1] = r61, 8
	st4   [addreg2] = r62, 8
	;;
	st4   [addreg1] = r63, 8
	st4   [addreg2] = r64, 8
	;;

	mov	ar.pfs = r16
	br.ret.sptk.few b0

.endp
