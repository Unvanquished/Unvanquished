/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggTheora SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE Theora SOURCE CODE IS COPYRIGHT (C) 2002-2009                *
 * by the Xiph.Org Foundation http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

  function: routines for validating codec initialization
  last mod: $Id: noop_theora.c 17819 2011-02-09 01:30:11Z giles $

 ********************************************************************/

#include <theora/theora.h>

#include "tests.h"

static int
noop_test_encode ()
{
  theora_info ti;
  theora_state th;

  INFO ("+ Initializing theora_info struct");
  theora_info_init (&ti);

  INFO ("+ Setting a 16x16 frame");
  ti.width = 16;
  ti.height = 16;

  INFO ("+ Setting a 1:1 frame rate");
  ti.fps_numerator = 1;
  ti.fps_denominator = 1;

  INFO ("+ Initializing theora_state for encoding");
  if (theora_encode_init (&th, &ti) != OC_DISABLED) {
    INFO ("+ Clearing theora_state");
    theora_clear (&th);
  }

  INFO ("+ Clearing theora_info struct");
  theora_info_clear (&ti);

  return 0;
}

static int
noop_test_decode ()
{
  theora_info ti;
  theora_state th;

  INFO ("+ Initializing theora_info struct");
  theora_info_init (&ti);

  INFO ("+ Initializing theora_state for decoding");
  theora_decode_init (&th, &ti);

  INFO ("+ Clearing theora_state");
  theora_clear (&th);

  INFO ("+ Clearing theora_info struct");
  theora_info_clear (&ti);

  return 0;
}

static int
noop_test_comments ()
{
  theora_comment tc;

  theora_comment_init (&tc);
  theora_comment_clear (&tc);

  return 0;
}

int main(int argc, char *argv[])
{
  /*noop_test_decode ();*/

  noop_test_encode ();

  noop_test_comments ();

  exit (0);
}
