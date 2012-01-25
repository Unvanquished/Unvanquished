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
// * $Id: idct_init.s,v 1.1.14.1 2009/05/25 09:03:47 Isibaar Exp $
// *
// ***************************************************************************/
//
// ****************************************************************************
// *                                                                           
// *  idct_init.s, IA-64 optimized inverse DCT                                 
// *                                                                           
// *  This version was implemented during an IA-64 practical training at 	   
// *  the University of Karlsruhe (http://i44w3.info.uni-karlsruhe.de/)		   
// *                                                                           
// ****************************************************************************
//

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
