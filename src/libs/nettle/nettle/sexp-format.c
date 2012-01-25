/* sexp-format.c
 *
 * Writing s-expressions.
 */

/* nettle, low-level cryptographics library
 *
 * Copyright (C) 2002 Niels Möller
 *  
 * The nettle library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 * 
 * The nettle library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with the nettle library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 */

#include "config.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sexp.h"
#include "buffer.h"

#if HAVE_LIBGMP
# include "bignum.h"
#endif

/* Code copied from sexp-conv.c: sexp_put_length */
static unsigned
format_prefix(struct nettle_buffer *buffer,
	      unsigned length)
{
  unsigned digit = 1;
  unsigned prefix_length = 1;
  
  for (;;)
    {
      unsigned next = digit * 10;
      if (next > length)
	break;

      prefix_length++;
      digit = next;
    }

  if (buffer)
    {
      for (; digit; length %= digit, digit /= 10)
	if (!NETTLE_BUFFER_PUTC(buffer, '0' + length / digit))
	  return 0;
      
      if (!NETTLE_BUFFER_PUTC(buffer, ':'))
	return 0;
    }

  return prefix_length + 1;
}

static unsigned
format_string(struct nettle_buffer *buffer,
	      unsigned length, const uint8_t *s)
{
  unsigned prefix_length = format_prefix(buffer, length);
  if (!prefix_length)
    return 0;

  if (buffer && !nettle_buffer_write(buffer, length, s))
    return 0;

  return prefix_length + length;
}

unsigned
sexp_vformat(struct nettle_buffer *buffer, const char *format, va_list args)
{
  unsigned nesting = 0;
  unsigned done = 0;

  for (;;)
    switch (*format++)
      {
      default:
	{
	  const char *start = format - 1;
	  unsigned length = 1 + strcspn(format, "()% \t");
	  unsigned output_length = format_string(buffer, length, start);
	  if (!output_length)
	    return 0;
	  
	  done += output_length;
	  format = start + length;

	  break;
	}
      case ' ': case '\t':
	break;
	
      case '\0':
	assert(!nesting);
	    
	return done;

      case '(':
	if (buffer && !NETTLE_BUFFER_PUTC(buffer, '('))
	  return 0;

	done++;
	nesting++;
	break;

      case ')':
	assert (nesting);
	if (buffer && !NETTLE_BUFFER_PUTC(buffer, ')'))
	  return 0;

	done++;
	nesting--;
	break;

      case '%':
	{
	  int nul_flag = 0;

	  if (*format == '0')
	    {
	      format++;
	      nul_flag = 1;
	    }
	  switch (*format++)
	    {
	    default:
	      abort();

	    case '(':
	    case ')':
	      /* Allow unbalanced parenthesis */
	      if (buffer && !NETTLE_BUFFER_PUTC(buffer, format[-1]))
		return 0;
	      done++;
	      break;
	      
	    case 's':
	      {
		const char *s;
		unsigned length;
		unsigned output_length;
		
		if (nul_flag)
		  {
		    s = va_arg(args, const char *);
		    length = strlen(s);
		  }
		else
		  {
		    length = va_arg(args, unsigned);
		    s = va_arg(args, const char *);
		  }
		
		output_length = format_string(buffer, length, s);
		if (!output_length)
		  return 0;

		done += output_length;
		break;
	      }
	    case 't':
	      {
		const char *s;
		unsigned length;
		unsigned output_length;
		
		if (nul_flag)
		  {
		    s = va_arg(args, const char *);
		    if (!s)
		      break;
		    
		    length = strlen(s);
		  }
		else
		  {
		    length = va_arg(args, unsigned);
		    s = va_arg(args, const char *);
		    if (!s)
		      break;
		  }
		
		if (buffer && !NETTLE_BUFFER_PUTC(buffer, '['))
		  return 0;
		done++;
		
		output_length = format_string(buffer, length, s);
	      
		if (!output_length)
		  return 0;

		done += output_length;
		
		if (buffer && !NETTLE_BUFFER_PUTC(buffer, ']'))
		  return 0;
		done++;
		
		break;
	      }
	      
	    case 'l':
	      {
		const char *s;
		unsigned length;
		
		if (nul_flag)
		  {
		    s = va_arg(args, const char *);
		    length = strlen(s);
		  }
		else
		  {
		    length = va_arg(args, unsigned);
		    s = va_arg(args, const char *);
		  }

		if (buffer && !nettle_buffer_write(buffer, length, s))
		  return 0;
	      
		done += length;
		break;
	      }
	    case 'i':
	      {
		uint32_t x = va_arg(args, uint32_t);
		unsigned length;
	      
		if (x < 0x80)
		  length = 1;
		else if (x < 0x8000L)
		  length = 2;
		else if (x < 0x800000L)
		  length = 3;
		else if (x < 0x80000000L)
		  length = 4;
		else
		  length = 5;
	      
		if (buffer && !(NETTLE_BUFFER_PUTC(buffer, '0' + length)
				&& NETTLE_BUFFER_PUTC(buffer, ':')))
		  return 0;

		done += (2 + length);

		if (buffer)
		  switch(length)
		    {
		    case 5:
		      /* Leading byte needed for the sign. */
		      if (!NETTLE_BUFFER_PUTC(buffer, 0))
			return 0;
		      /* Fall through */
		    case 4:
		      if (!NETTLE_BUFFER_PUTC(buffer, x >> 24))
			return 0;
		      /* Fall through */
		    case 3:
		      if (!NETTLE_BUFFER_PUTC(buffer, (x >> 16) & 0xff))
			return 0;
		      /* Fall through */
		    case 2:
		      if (!NETTLE_BUFFER_PUTC(buffer, (x >> 8) & 0xff))
			return 0;
		      /* Fall through */
		    case 1:
		      if (!NETTLE_BUFFER_PUTC(buffer, x & 0xff))
			return 0;
		      break;
		    default:
		      abort();
		    }
		break;
	      }
	    case 'b':
	      {
#if HAVE_LIBGMP
		const MP_INT *n = va_arg(args, const MP_INT *);
		unsigned length;
		unsigned prefix_length;
	      
		length = nettle_mpz_sizeinbase_256_s(n);
		prefix_length = format_prefix(buffer, length);
		if (!prefix_length)
		  return 0;

		done += prefix_length;

		if (buffer)
		  {
		    uint8_t *space = nettle_buffer_space(buffer, length);
		    if (!space)
		      return 0;
		  
		    nettle_mpz_get_str_256(length, space, n);
		  }

		done += length;
	      
#else /* ! HAVE_LIBGMP */
		abort();
#endif /* ! HAVE_LIBGMP */
		break;
	      }
	    }
	}
      }
}

unsigned
sexp_format(struct nettle_buffer *buffer, const char *format, ...)
{
  va_list args;
  unsigned done;
  
  va_start(args, format);
  done = sexp_vformat(buffer, format, args);
  va_end(args);

  return done;
}
