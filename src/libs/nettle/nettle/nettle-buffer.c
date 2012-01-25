/* buffer.c
 *
 * A bare-bones string stream.
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
#include <stdlib.h>
#include <string.h>

#include "buffer.h"

int
nettle_buffer_grow(struct nettle_buffer *buffer,
		   unsigned length)
{
  assert(buffer->size <= buffer->alloc);
  
  if (buffer->size + length > buffer->alloc)
    {
      unsigned alloc;
      uint8_t *p;
      
      if (!buffer->realloc)
	return 0;
      
      alloc = buffer->alloc * 2 + length + 100;
      p = buffer->realloc(buffer->realloc_ctx, buffer->contents, alloc);
      if (!p)
	return 0;
      
      buffer->contents = p;
      buffer->alloc = alloc;
    }
  return 1;
}

void
nettle_buffer_init_realloc(struct nettle_buffer *buffer,
			   void *realloc_ctx,
			   nettle_realloc_func realloc)
{
  buffer->contents = NULL;
  buffer->alloc = 0;
  buffer->realloc = realloc;
  buffer->realloc_ctx = realloc_ctx;
  buffer->size = 0;
}

void
nettle_buffer_init_size(struct nettle_buffer *buffer,
			unsigned length, uint8_t *space)
{
  buffer->contents = space;
  buffer->alloc = length;
  buffer->realloc = NULL;
  buffer->realloc_ctx = NULL;
  buffer->size = 0;
}

void
nettle_buffer_clear(struct nettle_buffer *buffer)
{
  if (buffer->realloc)
    buffer->realloc(buffer->realloc_ctx, buffer->contents, 0);

  buffer->contents = NULL;
  buffer->alloc = 0;
  buffer->size = 0;
}

void
nettle_buffer_reset(struct nettle_buffer *buffer)
{
  buffer->size = 0;
}

uint8_t *
nettle_buffer_space(struct nettle_buffer *buffer,
		    unsigned length)
{
  uint8_t *p;

  if (!nettle_buffer_grow(buffer, length))
    return NULL;

  p = buffer->contents + buffer->size;
  buffer->size += length;
  return p;
}
     
int
nettle_buffer_write(struct nettle_buffer *buffer,
		    unsigned length, const uint8_t *data)
{
  uint8_t *p = nettle_buffer_space(buffer, length);
  if (p)
    {
      memcpy(p, data, length);
      return 1;
    }
  else
    return 0;
}

int
nettle_buffer_copy(struct nettle_buffer *dst,
		   const struct nettle_buffer *src)
{
  return nettle_buffer_write(dst, src->size, src->contents);
}
