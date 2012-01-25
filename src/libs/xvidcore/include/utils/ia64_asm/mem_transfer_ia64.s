// ****************************************************************************
// *
// *  XVID MPEG-4 VIDEO CODEC
// *  - IA64 8bit<->16bit transfer -
// *
// *  Copyright(C) 2002 Sebastian Felis, Max Stengel
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
// * $Id: mem_transfer_ia64.s,v 1.5.10.1 2009/05/25 09:03:47 Isibaar Exp $
// *
// ***************************************************************************/
//
// ****************************************************************************
// *                                                                            
// *  mem_transfer_ia64.s, IA-64 8bit<->16bit transfer                                 
// *                                                                            
// *  This version was implemented during an IA-64 practical training at 	
// *  the University of Karlsruhe (http://i44w3.info.uni-karlsruhe.de/)		
// *                                                                            
// ****************************************************************************

///////////////////////////////////////////////////////////////////////////////
//
// mem_transfer.c optimized for ia-64 by Sebastian Felis and Max Stengel,
// University of Karlsruhe, Germany, 03.06.2002, during the laboratory
// "IA-64 Video Codec Assember Parktikum" at IPD Goos.

///// History /////////////////////////////////////////////////////////////////
//
// - 16.07.2002: several minor changes for ecc-conformity
// - 03.06.2002: initial version
//

///////////////////////////////////////////////////////////////////////////////
//
// Annotations:
// ===========
//
// - All functions work on 8x8-matrices. While the C-code-functions treat each
//   element seperatly, the functions in this assembler-code treat a whole line
//   simultaneously. So one loop is saved.
//   The remaining loop is relized by using softwarepipelining with rotating
//   rregisters.
// - Register renaming is used for better readability
// - To load 8 bytes of missaligned data, two 8-byte-blocks are loaded, both
//   parts are shifted and joined together with an "OR"-Instruction.
// - First parameter is stored in GR 32, next in GR 33, and so on. They must be 
//   saved, as these GRs are used for register-rotation.
// - Some of the orininal, German comments used during development are left in
//   in the code. They shouldn't bother anyone.
//
// Anmerkungen:
// ============
//
// - Alle Funtionen arbeiten mit 8x8-Matrizen. Während die Funktionen im C-Code
//   jedes Element einzeln bearbeiten, bearbeiten die Funtionen dieses Assembler-
//   Codes eine Zeile gleichzeitig. Dadurch kann eine Schleife eingespart werden.
//   Die verbleibende Schleife wird unter Benutzung von Softwarepipelining mit
//   rotierenden Registern realisiert.
// - Umbenennung der Register zwecks besserer Lesbarkeit wird verwendet.
// - Um 8 Bytes falsch ausgerichtete Daten zu laden, werden zwei 8-Byte-Blöcke
//   geladen, beide Teile mit "shift"-Operationen zurechterückt und mit einem
//   logischen Oder zusammenkopiert.
// - Die Parameter werden in den Registern ab GR 32 übergeben. Sie müssen ge-
//   sichert werden, da die Register für die register-Rotation benötigt werden.
// - Einige der ursprünglichen, deutschen Kommentare aus der Entwicklungsphase
//   sind im Code verblieben. Sie sollten niemanden stören.
//
///////////////////////////////////////////////////////////////////////////////


//	***	define Latencies for software pipilines ***

	LL  = 3 // Load
	SL  = 3 // Store
	PL  = 1 // Pack
	SHL = 1 // Shift 
	OL  = 1 // Or
	UL  = 1 // Unpack
	PAL = 1 // Parallel Add
	PSL = 1 // Parallel Subtract
	PAVGL = 1 // Parallel Avarage

	.text	
	

///////////////////////////////////////////////////////////////////////////////
//
// transfer8x8_copy_ia64
//
// SRC is missaligned, to align the source load two 8-bytes-words, shift it,
// join them and store the aligned source into the destination address.
//
///////////////////////////////////////////////////////////////////////////////

	.align 16
	.global transfer8x8_copy_ia64#
	.proc transfer8x8_copy_ia64#
	
transfer8x8_copy_ia64:
	.prologue	

//	*** register renaming ***
	zero = r0

	oldLC = r2
	oldPR = r3
	
	src_1 = r14 // left aligned address of src
	src_2 = r15 // right aligned address of src
	dst = r16  // destination address
	stride = r17
	
	offset = r18 // shift right offset
	aoffset = r19 // shift left offset
	
//	*** Saving old Loop-Counter (LC) and Predicate Registers (PR) ***
	.save ar.lc, oldLC
	mov oldLC = ar.lc
	mov oldPR = pr

	.body

//	*** Allocating new stackframe, initialize LC, Epilogue-Counter and PR ***
	alloc r9 = ar.pfs, 3, 29, 0, 32

//	*** Saving Parameters ***
	mov dst = r32
	mov stride = r34
	
//	*** Misalingment-Treatment ***	
	and src_1 = -8, r33 // Computing adress of first aligned block containing src-values
	dep offset = r33, zero, 3, 3 // Extracting offset for shr from src-adress
	;;
	sub aoffset = 64, offset // Computing counterpart of offset ("anti-offset"), used for shl
	add src_2 = 8, src_1 // Computing adress of second aligned block containing src-values

//	*** init loop: set loop counter, epilog counter, predicates ***
	mov ar.lc = 7 
	mov ar.ec = LL + SHL + OL + 1
	mov pr.rot = 1 << 16
	;;
	
//	*** define register arrays and predicate array for software pipeline ***
	// src_v1 = source value 1, shd_r = shifted right, shd_l = shifted left
	.rotr src_v1[LL+1], src_v2[LL+1], shd_r[SHL+1], shd_l[SHL+1], value[OL+1]
	.rotp ld_stage[LL], sh_stage[SHL], or_stage[OL], st_stage[1]


//	Software pipelined loop:
//	Stage 1: Load two 2 bytes from SRC_1, SRC_2 into SRC_v1 and SRC_v2
//	Stage 2: Shift both values of source to SHD_R and SHD_L
//	Stage 3: Join both parts together with OR
//	Stage 4: Store aligned date to destination and add stride to destination address 


.Loop_8x8copy:
	{.mii
		(ld_stage[0]) ld8 src_v1[0] = [src_1], stride	
		(sh_stage[0]) shr.u shd_r[0] = src_v1[LL], offset
	}
	{.mii
		(ld_stage[0]) ld8 src_v2[0] = [src_2], stride	
		(sh_stage[0]) shl shd_l[0] = src_v2[LL], aoffset
		(or_stage[0]) or value[0] = shd_l[SHL], shd_r[SHL]
	}
	{.mib
		(st_stage[0]) st8 [dst] = value[OL]
		(st_stage[0]) add dst = dst, stride
		br.ctop.sptk.few .Loop_8x8copy
		;;	
	}
	
//	*** Restore old LC and PRs ***
	mov ar.lc = oldLC
	mov pr = oldPR, -1
	
	br.ret.sptk.many b0
	
	.endp transfer8x8_copy_ia64#




///////////////////////////////////////////////////////////////////////////////
//
// transfer_8to16copy_ia64
//
// SRC is aligned. To convert 8 bit unsigned values to 16 bit signed values,
// UNPACK is used. So 8 bytes are loaded from source, unpacked to two 
// 4 x 16 bit values and stored to the destination. Destination is a continuous 
// array of 64 x 16 bit signed data. To store the next line, only 16 must be
// added to the destination address.
///////////////////////////////////////////////////////////////////////////////

	.align 16
	.global transfer_8to16copy_ia64#
	.proc transfer_8to16copy_ia64#
	
	
transfer_8to16copy_ia64:
	.prologue

//	*** register renaming ***
	oldLC = r2
	oldPR = r3

	zero = r0 // damit ist die Zahl "zero" = 0 gemeint
	
	dst_1 = r14 // destination address for first 4 x 16 bit values
	dst_2 = r15 // destination address for second 4 x 16 bit values
	src = r16
	stride = r17

//	*** Saving old Loop-Counter (LC) and Predicate Registers (PR) ***
	.save ar.lc, oldLC
	mov oldLC = ar.lc
	mov oldPR = pr


	.body

//	*** Allocating new stackframe, define rotating registers ***
	alloc r9 = ar.pfs, 4, 92, 0, 96
	
//	*** Saving Paramters ***
	mov dst_1 = r32 // fist 4 x 16 bit values
	add dst_2 = 8, r32 // second 4 x 16 bit values
	mov src = r33
	mov stride = r34

//	*** init loop: set loop counter, epilog counter, predicates ***
	mov ar.lc = 7
	mov ar.ec = LL + UL + 1
	mov pr.rot = 1 << 16
	;;
	
//	*** define register arrays and predicate array for software pipeline ***
	// src_v = source value, dst_v1 = destination value 1
	.rotr src_v[LL+1], dst_v1[UL+1], dst_v2[UL+1]
	.rotp ld_stage[LL], upack_stage[UL], st_stage[1]
	

//	Software pipelined loop:
//	Stage 1: Load value of SRC
//	Stage 2: Unpack the SRC_V to two 4 x 16 bit signed data
//	Stage 3: Store both 8 byte of 16 bit data


.Loop_8to16copy:
	{.mii
		(ld_stage[0]) ld8 src_v[0] = [src], stride
		(upack_stage[0]) unpack1.l dst_v1[0] = zero, src_v[LL]
		(upack_stage[0]) unpack1.h dst_v2[0] = zero, src_v[LL]
	}
	{.mmb
		(st_stage[0]) st8 [dst_1] = dst_v1[UL], 16
		(st_stage[0]) st8 [dst_2] = dst_v2[UL], 16
		br.ctop.sptk.few .Loop_8to16copy
		;;
	}
		
//	*** Restore old LC and PRs ***
	mov ar.lc = oldLC
	mov pr = oldPR, -1

	br.ret.sptk.many b0
	.endp transfer_8to16copy_ia64#


	

///////////////////////////////////////////////////////////////////////////////
//
// transfer_16to8copy_ia64
//
// src is a 64 x 16 bit signed continuous array. To convert the 16 bit 
// values to 8 bit unsigned data, PACK is used. So two 8-bytes-words of 
// 4 x 16 bit signed data are loaded, packed together and stored a 8-byte-word
// of 8 x 8 unsigned data to the destination.
///////////////////////////////////////////////////////////////////////////////

	.align 16
	.global transfer_16to8copy_ia64#
	.proc transfer_16to8copy_ia64#
transfer_16to8copy_ia64:
	.prologue

//	*** register renaming ***
	dst = r14 
	src_1 = r15
	src_2 = r17
	stride = r16

//	*** Saving old Loop-Counter (LC) and Predicate Registers (PR) ***
	.save ar.lc, oldLC
	mov oldLC = ar.lc
	mov oldPR = pr
	

	.body

//	*** Allocating new stackframe, define rotating registers ***
	alloc r9 = ar.pfs, 4, 92, 0, 96
	
//	*** Saving Paramters ***
	mov dst = r32
	mov src_1 = r33
	add src_2 = 8, r33
	mov stride = r34

//	*** init loop: set loop counter, epilog counter, predicates ***
	mov ar.lc = 7
	mov ar.ec = LL + PL + 1
	mov pr.rot = 1 << 16
	;;

//	*** define register arrays and predicate array for software pipeline ***
	// src_v1 = source value 1, dst_v = destination value
	.rotr src_v1[LL+1], src_v2[LL+1], dst_v[PL+1]
	.rotp ld_stage[LL], pack_stage[PL], st_stage[1]
	
	
//	Software pipelined loop:
//	Stage 1: Load two 8-byte-words of 4 x 16 bit signed source data
//	Stage 2: Pack them together to one 8 byte 8 x 8 bit unsigned data
//	Stage 3: Store the 8 byte to the destination address and add stride to
//	         destination address (to get the next 8 byte line of destination)


.Loop_16to8copy:
	{.mmi	
		(ld_stage[0]) ld8 src_v1[0] = [src_1], 16
		(ld_stage[0]) ld8 src_v2[0] = [src_2], 16
		(pack_stage[0]) pack2.uss dst_v[0] = src_v1[LL], src_v2[LL]
	}
	{.mib
		(st_stage[0]) st8 [dst] = dst_v[PL]
		(st_stage[0]) add dst = dst, stride
		br.ctop.sptk.few .Loop_16to8copy
		;;
	}
	
//	*** Restore old LC and PRs ***
	mov ar.lc = oldLC
	mov pr = oldPR, -1

	br.ret.sptk.many b0
	.endp transfer_16to8copy_ia64#



///////////////////////////////////////////////////////////////////////////////
//
// transfer_16to8add_ia64
//
// The 8-Bit-values of dst are "unpacked" into two 8-byte-blocks containing 16-
// bit-values. These are "parallel-added" to the values of src. The result is
// converted into 8-bit-values using "PACK" and stored at the adress of dst. 
// We assume that there is no misalignment.
//
///////////////////////////////////////////////////////////////////////////////

	.align 16
	.global transfer_16to8add_ia64#
	.proc transfer_16to8add_ia64#

transfer_16to8add_ia64:
	.prologue

//	*** register renaming ***
	dst = r14 
	src = r15
	stride = r16
	
	_src = r17

//	*** Saving old Loop-Counter (LC) and Predicate Registers (PR) ***
	.save ar.lc, r2
	mov oldLC = ar.lc
	mov oldPR = pr


	.body

//	*** Allocating new stackframe, initialize LC, Epilogue-Counter and PR ***
	alloc r9 = ar.pfs, 4, 92, 0, 96
	
//	*** Saving Paramters ***
	mov dst = r32
	mov src = r33
	mov stride = r34
	add _src = 8, r33

//	*** init loop: set loop counter, epilog counter, predicates ***
	mov ar.lc = 7
	mov ar.ec = LL + UL + PAL + PL + 1
	mov pr.rot = 1 << 16
	;;

//	*** define register arrays and predicate array for software pipeline ***
	.rotr _dst[LL+UL+PAL+PL+1], dst8[PL+1], pixel_1[PAL+1], pixel_2[PAL+1], w_dst16_1[UL+1], w_src_1[LL+UL+1], w_dst16_2[UL+1], w_src_2[LL+UL+1], w_dst8[LL+1]
	.rotp s1_p[LL], s2_p[UL], s3_p[PAL], s4_p[PL], s5_p[1]
	
	
//	Software pipelined loop:
//	s1_p: The values of src and dst are loaded
//	s2_p: The dst-values are converted to 16-bit-values
//	s3_p: The values of src and dst are added
// 	s4_p: The Results are packed into 8-bit-values
//	s5_p: The 8-bit-values are stored at the dst-adresses


.Loop_16to8add:
	{.mii	
		(s1_p[0]) ld8 w_src_1[0] = [src], 16 // läd die 1. Hälfte der j. Zeile von src (i = 0..3)
		(s1_p[0]) mov _dst[0] = dst // erhöht die Adresse von dst um stride
		(s3_p[0]) padd2.sss pixel_1[0] = w_dst16_1[UL], w_src_1[LL+UL] // parallele Addition von scr und dst
	}
	{.mii
		(s1_p[0]) ld8 w_dst8[0] = [dst], stride // läd die j. Zeile von dst
		(s2_p[0]) unpack1.l w_dst16_1[0] = r0, w_dst8[LL]; // dst wird für i = 0..3 in 16-Bit umgewandelt
		(s2_p[0]) unpack1.h w_dst16_2[0] = r0, w_dst8[LL]; // dst wird für i = 4..7 in 16-Bit umgewandelt
	}
	{.mii
		(s1_p[0]) ld8 w_src_2[0] = [_src], 16 // läd die 2. Hälfte der j. Zeile von src (i = 4..7)
		(s3_p[0]) padd2.sss pixel_2[0] = w_dst16_2[UL], w_src_2[LL+UL] // parallele Addition von scr und dst
		(s4_p[0]) pack2.uss dst8[0] = pixel_1[PAL], pixel_2[PAL] // wandelt die Summen (pixel) in 8-Bit Werte um. Die Überprüfung der Wertebereiche erfolgt automatisch
	}
	{.mmb
		(s5_p[0]) st8 [_dst[LL+UL+PAL+PL]] = dst8[PL] // speichert dst ab
		(s1_p[0]) nop.m 0
		br.ctop.sptk.few .Loop_16to8add
		;;
	}
	
//	*** Restore old LC and PRs ***
	mov ar.lc = oldLC
	mov pr = oldPR, -1

	br.ret.sptk.many b0
	.endp transfer_16to8add_ia64#



///////////////////////////////////////////////////////////////////////////////
//
// transfer_8to16sub_ia64
//
// The 8-bit-values of ref and cur are loaded. cur is converted to 16-bit. The
// Difference of cur and ref ist stored at the dct-adresses and cur is copied
// into the ref-array.
//
// You must assume, that the data adressed by 'ref' are misaligned in memory.
// But you can assume, that the other data are aligned (at least I hope so).
//	
///////////////////////////////////////////////////////////////////////////////

	.align 16
	.global transfer_8to16sub_ia64#
	.proc transfer_8to16sub_ia64#
	
	
transfer_8to16sub_ia64:
	.prologue

//	*** register renaming ***
	oldLC = r2
	oldPR = r3

	zero = r0 // damit ist die Zahl "zero" = 0 gemeint
	
	//Die folgenden Register erhalten die gleichen Namen, wie die Variablen in der C-Vorlage
	dct = r14
	cur = r15
	ref = r34 // muss nicht extra gesichert werden, deswegen bleibt das ÜbergabeRegister in dieser Liste
	stride = r16
	
	offset = r17 // Offset der falsch ausgerichteten Daten zum zurechtrücken
	aoffset = r18 // Gegenstück zum Offset,
	ref_a1 = r19 // Adresse des ersten 64-Bit Blocks von ref
	ref_a2 = r20 // Adresse des zweiten 64-Bit Blocks von ref
	
	_dct = r21 // Register für die Zieladressen des 2. dct-Blocks

//	*** Saving old Loop-Counter (LC) and Predicate Registers (PR) ***
	.save ar.lc, r2
	mov oldLC = ar.lc
	mov oldPR = pr
	

	.body

//	*** Allocating new stackframe, define rotating registers ***
	alloc r9 = ar.pfs, 4, 92, 0, 96
	
//	*** Saving Paramters ***
	mov dct = r32 
	mov cur = r33
	// mov ref = r34: ref is unaligned, get aligned ref below...
	mov stride = r35
	
	and ref_a1 = -8, ref // Die Adresse des ersten 64-Bit Blocks, in dem ref liegt, wird berechnet (entspricht mod 8)
	dep offset = ref, zero, 3, 3
	;;
	add ref_a2 = 8, ref_a1
	sub aoffset = 64, offset // Gegenstück zum Offset wird berechnet
	add _dct = 8, dct // Die Adresse für den 2. dct-Block wird berechnet, um 8 Byte (= 64 Bit) höher als beim 1. Block

//	*** init loop: set loop counter, epilog counter, predicates ***
	mov ar.lc = 7
	mov ar.ec = LL + SHL + OL + UL + PSL + 1
	mov pr.rot = 1 << 16
	;;

//	*** define register arrays and predicate array for software pipeline ***
	.rotr  c[LL+1], ref_v1[LL+1], ref_v2[LL+1], c16_1[SHL+OL+UL+1], c16_2[SHL+OL+UL+1], ref_shdr[SHL+1], ref_shdl[SHL+1], r[OL+1], r16_1[UL+1], r16_2[UL+1],  dct_1[PSL+1], dct_2[PSL+1], _cur[LL+SHL+OL+UL+1]
	.rotp s1_p[LL], s2_p[SHL], s3_p[OL], s4_p[UL], s5_p[PSL], s6_p[1]
	

//	Software pipelined loop:
//	s1_p: The values of ref and cur ale loaded, a copy of cur is made.
//	s2_p: cur is converted to 16-bit and thehe misaligned values of ref are
// 	      shifted...
//	s3_p: ... and copied together.
//	s4_p: This ref-value is converted to 16-bit. The values of cur are stored
//	      at the ref-adresses.
//	s5_p: the ref- abd cur-values are substracted...
//	s6_p: ...and the result is stored at the dct-adresses.

 
loop_8to16sub:
	{.mii
		(s1_p[0]) ld8 ref_v1[0] = [ref_a1], stride // läd den 1. 64-Bit-Block, der einen Teil der ref-Daten enthält
		(s1_p[0]) mov _cur[0] = cur // cur wird für spätere Verwendung gesichert
		(s2_p[0]) shr.u ref_shdr[0] = ref_v1[LL], offset // Die rechte Hälfte wird zurechtgerückt
	}	
	{.mii
		(s1_p[0]) ld8 ref_v2[0] = [ref_a2], stride // läd den 2. 64-Bit-Block
		(s2_p[0]) shl ref_shdl[0] = ref_v2[LL], aoffset // Die linke Hälfte wird zurechtgerückt
		(s3_p[0]) or r[0] = ref_shdr[SHL], ref_shdl[SHL] // Die zurechtgerückten Daten werden in r zusammenkopiert
	}
	{.mii
		(s1_p[0]) ld8 c[0] = [cur], stride //läd die j. Zeile von cur komplett
		(s2_p[0]) unpack1.l c16_1[0] = zero, c[LL]; // c wird für i = 0..3 in 16-Bit umgewandelt
		(s2_p[0]) unpack1.h c16_2[0] = zero, c[LL]; // c wird für i = 4..7 in 16-Bit umgewandelt
	}
	{.mii
		(s4_p[0]) st8 [_cur[LL+SHL+OL]] = r[OL] // cur wird auf den Wert von r gesetzt
		//Umwandeln der 8-Bit r und c -Werte in 16-bit Werte
		(s4_p[0]) unpack1.l r16_1[0] = zero, r[OL]; // r wird für i = 0..3 in 16-Bit umgewandelt
		(s4_p[0]) unpack1.h r16_2[0] = zero, r[OL]; // r wird für i = 4..7 in 16-Bit umgewandelt
	}
	{.mii
		(s5_p[0]) psub2.sss dct_1[0] = c16_1[SHL+OL+UL], r16_1[UL] // Subtraktion der 1. Häfte der j. Zeile
		(s5_p[0]) psub2.sss dct_2[0] = c16_2[SHL+OL+UL], r16_2[UL] // Subtraktion der 2. Hälfte
	}
	{.mmb
		(s6_p[0]) st8 [dct] = dct_1[PSL], 16 // speichert den 1. 64-Bit-Block an der vorgesehenen Adresse, erhöhen der Adresse um 16 Byte für den nächsten Wert
		(s6_p[0]) st8 [_dct] = dct_2[PSL], 16 // speichert den 2. 64-Bit-Block an der vorgesehenen Adresse, erhöhen der Adresse um 16 Byte für den nächsten Wert
		br.ctop.sptk.few loop_8to16sub // Und hopp
		;;
	}
	
//	*** Restore old LC and PRs ***
	mov ar.lc = oldLC
	mov pr = oldPR, -1
	
	br.ret.sptk.many b0
	.endp transfer_8to16sub_ia64#
	




///////////////////////////////////////////////////////////////////////////////
//
// transfer_8to16sub2_ia64
//
// At the time, this function was written, it was not yet in use.
// We assume that the values of ref1/2 are misaligned.
// 
// The values of ref1/2 and cur are loaded, the ref-values need misalignment-
// treatment. The values are converted to 16-bit using unpack. The average of
// ref1 and ref2 is computed with pavg and substacted from cur. The results are
// stored at the dct-adresses.
// pavg1.raz is used to get the same results as the C-code-function. 
// 
///////////////////////////////////////////////////////////////////////////////	

	.text
	.align 16
	.global transfer_8to16sub2_ia64#
	.proc transfer_8to16sub2_ia64#
	
transfer_8to16sub2_ia64:
	.prologue

//	*** register renaming ***
	//	We've tried to keep the C-Code names as often as possible, at least as
	//	part of register-names
	oldLC = r2
	oldPR = r3
	
	zero = r0
	
	dct_al = r14 // dct: adress of left block in one line
	dct_ar = r15 // dct: adress of right block in one line
	cur = r16
	ref1_al = r17 // ref1: aligned adress of lower part
	ref1_ah = r18 // ref1: aligned adress of higher part
	ref2_al = r19 // ref2: aligned adress of lower part
	ref2_ah = r20 // ref2: aligned adress of higher part
	stride = r21
	
	offset_1 = r22
	offset_2 = r23
	aoffset_1 = r24
	aoffset_2 = r25

//	*** Saving old Loop-Counter (LC) and Predicate Registers (PR) ***
	.save ar.lc, r2
	mov oldLC = ar.lc
	mov oldPR = pr


	.body		

//	*** Saving Paramters ***
//	*** (as inputregisters r32 + are needed for register-rotation) ***
	mov dct_ar = r32	
	add dct_al = 8, r32	
	mov cur = r33
	
	and ref1_al = -8, r34	
	and ref2_al = -8, r35	// ref2 aligned adrress of lower part
	
	mov stride = r36
	
//	***	Calculations for Misaligment-Handling ***
	dep offset_1 = r34, zero, 3, 3
	dep offset_2 = r35, zero, 3, 3
	;;
	add ref1_ah = 8, ref1_al
	add ref2_ah = 8, ref2_al
	sub aoffset_1 = 64, offset_1
	sub aoffset_2 = 64, offset_2
	;;

//	*** Allocating new stackframe, define rotating registers ***
	alloc r9 = ar.pfs, 5, 91, 0, 96
	
//	*** init loop: set loop counter, epilog counter, predicates ***
	mov ar.lc = 7
	mov ar.ec = LL + SHL + OL + PAVGL + UL +PSL + 1
	mov pr.rot = 1 << 16
	;;
	
//	*** define register arrays and predicate array for software pipeline ***
	.rotr ref1_vl[LL+1], ref1_vh[LL+1], ref2_vl[LL+1], ref2_vh[LL+1], c[LL+SHL+OL+PAVGL+1], ref1_l[SHL+1], ref1_h[SHL+1], ref2_l[SHL+1], ref2_h[SHL+1], ref1_aligned[OL+1], ref2_aligned[OL+1], r[PAVGL+1], r16_l[UL+1], r16_r[UL+1], c16_l[UL+1], c16_r[UL+1], dct16_l[PSL+1], dct16_r[PSL+1]
	.rotp ld_stage[LL], sh_stage[SHL], or_stage[OL], pavg_stage[PAVGL], up_stage[UL], psub_stage[PSL], st_stage[1]

 
//	software pipelined loop:
//	ld_stage:   The values of ref1, ref2, cur are loaded
//	sh_stage:   The misaligned values of ref1/2 are shifted...
//	or_stage:   ...and copied together. 
//	pavg_stage: The average of ref1 and ref2 is computed.
//	up_stage:   The result and the cur-values are converted to 16-bit.
//	psub_stage: Those values are substracted...
//	st_stage:   ...and stored at the dct-adresses.

 
.Loop_8to16sub2:
	{.mii
		(ld_stage[0])	ld8 c[0] = [cur], stride
		(sh_stage[0])	shr.u ref1_l[0] = ref1_vl[LL], offset_1
		(sh_stage[0])	shl ref1_h[0] = ref1_vh[LL], aoffset_1
	}
	{.mii
		(ld_stage[0])	ld8 ref1_vl[0] = [ref1_al], stride
		(sh_stage[0])	shr.u ref2_l[0] = ref2_vl[LL], offset_2
		(sh_stage[0])	shl ref2_h[0] = ref2_vh[LL], aoffset_2
	}
	{.mii
		(ld_stage[0])	ld8 ref1_vh[0] = [ref1_ah], stride
		(or_stage[0])	or ref1_aligned[0] = ref1_h[SHL], ref1_l[SHL]
		(or_stage[0])	or ref2_aligned[0] = ref2_h[SHL], ref2_l[SHL]
	}
	{.mii
		(ld_stage[0])	ld8 ref2_vl[0] = [ref2_al], stride
		(pavg_stage[0])	pavg1.raz r[0] = ref1_aligned[OL], ref2_aligned[OL]
		(up_stage[0])	unpack1.l r16_r[0] = zero, r[PAVGL]
	}
	{.mii		
		(ld_stage[0])	ld8 ref2_vh[0] = [ref2_ah], stride
		(up_stage[0])	unpack1.h r16_l[0] = zero, r[PAVGL]
		(up_stage[0])	unpack1.l c16_r[0] = zero, c[LL+SHL+OL+PAVGL]
	}
	{.mii			
		(st_stage[0])	st8 [dct_ar] = dct16_r[PSL], 16
		(up_stage[0])	unpack1.h c16_l[0] = zero, c[LL+SHL+OL+PAVGL]
		(psub_stage[0])	psub2.sss dct16_l[0] = c16_l[UL], r16_l[UL]
	}
	{.mib		
		(st_stage[0])	st8 [dct_al] = dct16_l[PSL], 16
		(psub_stage[0])	psub2.sss dct16_r[0] = c16_r[UL], r16_r[UL]		
		br.ctop.sptk.few .Loop_8to16sub2 // Und hopp
		;;
	}
		
//	*** Restore old LC and PRs ***
	mov ar.lc = oldLC
	mov pr = oldPR, -1

	br.ret.sptk.many b0
	.endp transfer_8to16sub2_ia64#
