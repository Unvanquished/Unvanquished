/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggTheora SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE Theora SOURCE CODE IS COPYRIGHT (C) 2002-2009                *
 * by the Xiph.Org Foundation and contributors http://www.xiph.org/ *
 *                                                                  *
 ********************************************************************

  function:
    last mod: $Id$

 ********************************************************************/
#include <string.h>
#include "c64xint.h"
#include "dct.h"

#define OC_C1S7D ((OC_C1S7<<16)|(OC_C1S7&0xFFFF))
#define OC_C2S6D ((OC_C2S6<<16)|(OC_C2S6&0xFFFF))
#define OC_C3S5D ((OC_C3S5<<16)|(OC_C3S5&0xFFFF))
#define OC_C4S4D ((OC_C4S4<<16)|(OC_C4S4&0xFFFF))
#define OC_C5S3D ((OC_C5S3<<16)|(OC_C5S3&0xFFFF))
#define OC_C6S2D ((OC_C6S2<<16)|(OC_C6S2&0xFFFF))
#define OC_C7S1D ((OC_C7S1<<16)|(OC_C7S1&0xFFFF))

/*Various building blocks for the iDCT implementations.
  These are done in macros instead of functions so that we can use all local
   variables, which avoids leaving the compiler to try to sort out memory
   reference dependencies.*/

/*Load two rows into x0...x7.*/
#define OC_IDCT8x2_LOAD8(_x) \
  do{ \
    long long ll; \
    ll=_dpack2(_amem4_const((_x)+8),_amem4_const((_x)+0)); \
    x0=_loll(ll); \
    x1=_hill(ll); \
    ll=_dpack2(_amem4_const((_x)+10),_amem4_const((_x)+2)); \
    x2=_loll(ll); \
    x3=_hill(ll); \
    ll=_dpack2(_amem4_const((_x)+12),_amem4_const((_x)+4)); \
    x4=_loll(ll); \
    x5=_hill(ll); \
    ll=_dpack2(_amem4_const((_x)+14),_amem4_const((_x)+6)); \
    x6=_loll(ll); \
    x7=_hill(ll); \
  } \
  while(0)

/*Load two rows into x0...x3.
  Uses ll as a temporary.*/
#define OC_IDCT8x2_LOAD4(_x) \
  do{ \
    long long ll; \
    ll=_dpack2(_amem4_const((_x)+8),_amem4_const((_x)+0)); \
    x0=_loll(ll); \
    x1=_hill(ll); \
    ll=_dpack2(_amem4_const((_x)+10),_amem4_const((_x)+2)); \
    x2=_loll(ll); \
    x3=_hill(ll); \
  } \
  while(0)

/*Load two rows into x0...x1.*/
#define OC_IDCT8x2_LOAD2(_x) \
  do{ \
    long long ll; \
    ll=_dpack2(_amem4_const((_x)+8),_amem4_const((_x)+0)); \
    x0=_loll(ll); \
    x1=_hill(ll); \
  } \
  while(0)

/*Load two columns into x0...x1.*/
#define OC_IDCT8x2_LOAD2T(_x) \
  do{ \
    x0=_amem4_const((_x)+(0<<3)); \
    x1=_amem4_const((_x)+(1<<3)); \
  } \
  while(0)

/*Transform x0...x7 into t0...t7.*/
#define OC_IDCT8x2() \
  do{ \
    long long ll; \
    int       a; \
    int       b; \
    /*Stage 1:*/ \
    ll=_addsub2(x0,x4); \
    a=_hill(ll); \
    b=_loll(ll); \
    t0=_packh2(_mpyhus(OC_C4S4D,a),_mpyus(OC_C4S4D,a)); \
    t1=_packh2(_mpyhus(OC_C4S4D,b),_mpyus(OC_C4S4D,b)); \
    ll=_mpy2ll(OC_C6S2D,x2); \
    a=_packh2(_hill(ll),_loll(ll)); \
    ll=_mpy2ll(OC_C2S6D,x6); \
    b=_add2(_packh2(_hill(ll),_loll(ll)),x6); \
    t2=_sub2(a,b); \
    ll=_mpy2ll(OC_C2S6D,x2); \
    a=_add2(_packh2(_hill(ll),_loll(ll)),x2); \
    ll=_mpy2ll(OC_C6S2D,x6); \
    b=_packh2(_hill(ll),_loll(ll)); \
    t3=_add2(a,b); \
    ll=_mpy2ll(OC_C7S1D,x1); \
    a=_packh2(_hill(ll),_loll(ll)); \
    ll=_mpy2ll(OC_C1S7D,x7); \
    b=_add2(_packh2(_hill(ll),_loll(ll)),x7); \
    t4=_sub2(a,b); \
    ll=_mpy2ll(OC_C3S5D,x5); \
    a=_add2(_packh2(_hill(ll),_loll(ll)),x5); \
    ll=_mpy2ll(OC_C5S3D,x3); \
    b=_add2(_packh2(_hill(ll),_loll(ll)),x3); \
    t5=_sub2(a,b); \
    ll=_mpy2ll(OC_C5S3D,x5); \
    a=_add2(_packh2(_hill(ll),_loll(ll)),x5); \
    ll=_mpy2ll(OC_C3S5D,x3); \
    b=_add2(_packh2(_hill(ll),_loll(ll)),x3); \
    t6=_add2(a,b); \
    ll=_mpy2ll(OC_C1S7D,x1); \
    a=_add2(_packh2(_hill(ll),_loll(ll)),x1); \
    ll=_mpy2ll(OC_C7S1D,x7); \
    b=_packh2(_hill(ll),_loll(ll)); \
    t7=_add2(a,b); \
    /*Stage 2:*/ \
    ll=_addsub2(t4,t5); \
    t4=_hill(ll); \
    b=_loll(ll); \
    ll=_mpy2ll(OC_C4S4D,b); \
    t5=_add2(_packh2(_hill(ll),_loll(ll)),b); \
    ll=_addsub2(t7,t6); \
    t7=_hill(ll); \
    b=_loll(ll); \
    ll=_mpy2ll(OC_C4S4D,b); \
    t6=_add2(_packh2(_hill(ll),_loll(ll)),b); \
    /*Stage 3:*/ \
    ll=_addsub2(t0,t3); \
    t0=_hill(ll); \
    t3=_loll(ll); \
    ll=_addsub2(t1,t2); \
    t1=_hill(ll); \
    t2=_loll(ll); \
    ll=_addsub2(t6,t5); \
    t6=_hill(ll); \
    t5=_loll(ll); \
  } \
  while(0)

/*Transform x0...x3 into t0...t7, assuming x4...x7 are zero.*/
#define OC_IDCT8x2_4() \
  do{ \
    long long ll; \
    int       a; \
    /*Stage 1:*/ \
    ll=_mpy2ll(OC_C4S4D,x0); \
    t0=_add2(_packh2(_hill(ll),_loll(ll)),x0); \
    t1=t0; \
    ll=_mpy2ll(OC_C6S2D,x2); \
    t2=_packh2(_hill(ll),_loll(ll)); \
    ll=_mpy2ll(OC_C2S6D,x2); \
    t3=_add2(_packh2(_hill(ll),_loll(ll)),x2); \
    ll=_mpy2ll(OC_C7S1D,x1); \
    t4=_packh2(_hill(ll),_loll(ll)); \
    ll=_mpy2ll(OC_C5S3D,x3); \
    t5=_add2(_packh2(_hill(ll),_loll(ll)),x3); \
    ll=_mpy2ll(OC_C3S5D,x3); \
    t6=_add2(_packh2(_hill(ll),_loll(ll)),x3); \
    ll=_mpy2ll(OC_C1S7D,x1); \
    t7=_add2(_packh2(_hill(ll),_loll(ll)),x1); \
    /*Stage 2:*/ \
    ll=_addsub2(t4,t5); \
    t4=_loll(ll); \
    a=_hill(ll); \
    ll=_mpy2ll(OC_C4S4D,a); \
    t5=_add2(_packh2(_hill(ll),_loll(ll)),a); \
    ll=_addsub2(t7,t6); \
    t7=_hill(ll); \
    a=_loll(ll); \
    ll=_mpy2ll(OC_C4S4D,a); \
    t6=_add2(_packh2(_hill(ll),_loll(ll)),a); \
    /*Stage 3:*/ \
    ll=_addsub2(t0,t3); \
    t0=_hill(ll); \
    t3=_loll(ll); \
    ll=_addsub2(t1,t2); \
    t1=_hill(ll); \
    t2=_loll(ll); \
    ll=_addsub2(t6,t5); \
    t6=_hill(ll); \
    t5=_loll(ll); \
  } \
  while(0)

/*Transform x0...x1 into t0...t7, assuming x2...x7 are zero.*/
#define OC_IDCT8x2_2() \
  do{ \
    long long ll; \
    /*Stage 1:*/ \
    ll=_mpy2ll(OC_C4S4D,x0); \
    t0=_add2(_packh2(_hill(ll),_loll(ll)),x0); \
    t1=t0; \
    ll=_mpy2ll(OC_C7S1D,x1); \
    t4=_packh2(_hill(ll),_loll(ll)); \
    ll=_mpy2ll(OC_C1S7D,x1); \
    t7=_add2(_packh2(_hill(ll),_loll(ll)),x1); \
    /*Stage 2:*/ \
    ll=_mpy2ll(OC_C4S4D,t4); \
    t5=_add2(_packh2(_hill(ll),_loll(ll)),t4); \
    ll=_mpy2ll(OC_C4S4D,t7); \
    t6=_add2(_packh2(_hill(ll),_loll(ll)),t7); \
    /*Stage 3:*/ \
    t3=t0; \
    t2=t1; \
    ll=_addsub2(t6,t5); \
    t6=_hill(ll); \
    t5=_loll(ll); \
  } \
  while(0)

/*Finish transforming t0...t7 and store two rows.*/
#define OC_IDCT8x2_STORE(_y) \
  do{ \
    long long ll; \
    int       a; \
    int       b; \
    int       c; \
    int       d; \
    /*Stage 4:*/ \
    ll=_addsub2(t0,t7); \
    a=_hill(ll); \
    c=_loll(ll); \
    ll=_addsub2(t1,t6); \
    b=_hill(ll); \
    d=_loll(ll); \
    ll=_dpack2(b,a); \
    _amem4((_y)+0)=_loll(ll); \
    _amem4((_y)+8)=_hill(ll); \
    ll=_dpack2(c,d); \
    _amem4((_y)+6)=_loll(ll); \
    _amem4((_y)+14)=_hill(ll); \
    ll=_addsub2(t2,t5); \
    a=_hill(ll); \
    c=_loll(ll); \
    ll=_addsub2(t3,t4); \
    b=_hill(ll); \
    d=_loll(ll); \
    ll=_dpack2(b,a); \
    _amem4((_y)+2)=_loll(ll); \
    _amem4((_y)+10)=_hill(ll); \
    ll=_dpack2(c,d); \
    _amem4((_y)+4)=_loll(ll); \
    _amem4((_y)+12)=_hill(ll); \
  } \
  while(0)

/*Finish transforming t0...t7 and store two columns.*/
#define OC_IDCT8x2_STORET(_y) \
  do{ \
    long long ll; \
    /*Stage 4:*/ \
    ll=_addsub2(t0,t7); \
    _amem4((_y)+(0<<3))=_hill(ll); \
    _amem4((_y)+(7<<3))=_loll(ll); \
    ll=_addsub2(t1,t6); \
    _amem4((_y)+(1<<3))=_hill(ll); \
    _amem4((_y)+(6<<3))=_loll(ll); \
    ll=_addsub2(t2,t5); \
    _amem4((_y)+(2<<3))=_hill(ll); \
    _amem4((_y)+(5<<3))=_loll(ll); \
    ll=_addsub2(t3,t4); \
    _amem4((_y)+(3<<3))=_hill(ll); \
    _amem4((_y)+(4<<3))=_loll(ll); \
  } \
  while(0)

/*Finish transforming t0...t7, round and scale, and store two columns.*/
#define OC_IDCT8x2_ROUND_STORET(_y) \
  do{ \
    long long ll; \
    /*Stage 4:*/ \
    /*Adjust for the scale factor.*/ \
    ll=_addsub2(t0,t7); \
    _amem4((_y)+(0<<3))=_shr2(_add2(_hill(ll),0x00080008),4); \
    _amem4((_y)+(7<<3))=_shr2(_add2(_loll(ll),0x00080008),4); \
    ll=_addsub2(t1,t6); \
    _amem4((_y)+(1<<3))=_shr2(_add2(_hill(ll),0x00080008),4); \
    _amem4((_y)+(6<<3))=_shr2(_add2(_loll(ll),0x00080008),4); \
    ll=_addsub2(t2,t5); \
    _amem4((_y)+(2<<3))=_shr2(_add2(_hill(ll),0x00080008),4); \
    _amem4((_y)+(5<<3))=_shr2(_add2(_loll(ll),0x00080008),4); \
    ll=_addsub2(t3,t4); \
    _amem4((_y)+(3<<3))=_shr2(_add2(_hill(ll),0x00080008),4); \
    _amem4((_y)+(4<<3))=_shr2(_add2(_loll(ll),0x00080008),4); \
  } \
  while(0)

/*196 cycles.*/
static void oc_idct8x8_slow_c64x(ogg_int16_t _y[64],ogg_int16_t _x[64]){
  ogg_int16_t w[64];
  int         x0;
  int         x1;
  int         x2;
  int         x3;
  int         x4;
  int         x5;
  int         x6;
  int         x7;
  int         t0;
  int         t1;
  int         t2;
  int         t3;
  int         t4;
  int         t5;
  int         t6;
  int         t7;
  int         i;
  /*Transform rows of x into columns of w.*/
  for(i=0;i<8;i+=2){
    OC_IDCT8x2_LOAD8(_x+i*8);
    _amem8(_x+i*8)=0LL;
    _amem8(_x+i*8+4)=0LL;
    _amem8(_x+i*8+8)=0LL;
    _amem8(_x+i*8+12)=0LL;
    OC_IDCT8x2();
    OC_IDCT8x2_STORET(w+i);
  }
  /*Transform rows of w into columns of y.*/
  for(i=0;i<8;i+=2){
    OC_IDCT8x2_LOAD8(w+i*8);
    OC_IDCT8x2();
    OC_IDCT8x2_ROUND_STORET(_y+i);
  }
}

/*106 cycles.*/
static void oc_idct8x8_10_c64x(ogg_int16_t _y[64],ogg_int16_t _x[64]){
  ogg_int16_t w[64];
  int         t0;
  int         t1;
  int         t2;
  int         t3;
  int         t4;
  int         t5;
  int         t6;
  int         t7;
  int         x0;
  int         x1;
  int         x2;
  int         x3;
  int         i;
  /*Transform rows of x into columns of w.*/
  OC_IDCT8x2_LOAD4(_x);
  OC_IDCT8x2_4();
  OC_IDCT8x2_STORET(w);
  OC_IDCT8x2_LOAD2(_x+16);
  _amem8(_x)=0LL;
  _amem8(_x+8)=0LL;
  _amem4(_x+16)=0;
  _amem4(_x+24)=0;
  OC_IDCT8x2_2();
  OC_IDCT8x2_STORET(w+2);
  /*Transform rows of w into columns of y.*/
  for(i=0;i<8;i+=2){
    OC_IDCT8x2_LOAD4(w+i*8);
    OC_IDCT8x2_4();
    OC_IDCT8x2_ROUND_STORET(_y+i);
  }
}

#if 0
/*This used to compile to something faster (88 cycles), but no longer, and I'm
   not sure what changed to cause this.
  In any case, it's barely an advantage over the 10-coefficient version, and is
   now hardly worth the icache space.*/
/*95 cycles.*/
static inline void oc_idct8x8_3_c64x(ogg_int16_t _y[64],ogg_int16_t _x[64]){
  ogg_int16_t w[64];
  int         t0;
  int         t1;
  int         t2;
  int         t3;
  int         t4;
  int         t5;
  int         t6;
  int         t7;
  int         x0;
  int         x1;
  int         i;
  /*Transform rows of x into rows of w.*/
  for(i=0;i<2;i+=2){
    OC_IDCT8x2_LOAD2(_x+i*8);
    OC_IDCT8x2_2();
    OC_IDCT8x2_STORE(w+i*8);
  }
  _amem4(_x)=0;
  _amem4(_x+8)=0;
  /*Transform columns of w into columns of y.*/
  for(i=0;i<8;i+=2){
    OC_IDCT8x2_LOAD2T(w+i);
    OC_IDCT8x2_2();
    OC_IDCT8x2_ROUND_STORET(_y+i);
  }
}
#endif

/*Performs an inverse 8x8 Type-II DCT transform.
  The input is assumed to be scaled by a factor of 4 relative to orthonormal
   version of the transform.*/
void oc_idct8x8_c64x(ogg_int16_t _y[64],ogg_int16_t _x[64],int _last_zzi){
  /*if(_last_zzi<=3)oc_idct8x8_3_c64x(_y,_x);
  else*/ if(_last_zzi<=10)oc_idct8x8_10_c64x(_y,_x);
  else oc_idct8x8_slow_c64x(_y,_x);
}
