/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2009 Darklegion Development

This file is part of the Unvanquished GPL Source Code (Unvanquished Source Code).

Unvanquished is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Unvanquished is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Unvanquished; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

===========================================================================
*/

#ifndef BG_BOUNDEDVECTOR_H
#define BG_BOUNDEDVECTOR_H

#include "common/Assert.h"
#include "common/Optional.h"

/*
 * This class is a simpler std::vector alternative that doesn't allocate
 * and survives a memset(0) without damage, provided that you don't need to
 * call destructors in such case.
 *
 * The purpose of this is to provide something in-between std::vector, which
 * has fully dynamic and has unbounded size, and std::array that quite
 * impractical as soon as you don't use it at its full capacity (it doesn't
 * support an .append() or a clear() method, you can't use a C++11 style-loop
 * to iterate over only the defined elements, etc.)
 */
template<typename T, size_t maximum_size_>
class BoundedVector
{
private:
	T array[maximum_size_];
	size_t size_; // current size, should be <= capacity
public:
	BoundedVector() : size_(0) {}

	BoundedVector( const BoundedVector<T, maximum_size_> & other )
		: size_(other.size_)
	{
		for (size_t i = 0; i < other.size_; i++)
		{
			array[i] = other.array[i];
		}
	}

	BoundedVector( BoundedVector<T, maximum_size_> && other )
		: BoundedVector()
	{
		std::swap( array, other.array );
		std::swap( size_, other.size_ );
	}

	BoundedVector<T, maximum_size_>& operator=( const BoundedVector<T, maximum_size_> & other )
	{
		for (size_t i = 0; i < other.size_; i++)
		{
			array[i] = other.array[i];
		}
		size_ = other.size_;
		return *this;
	}

	BoundedVector<T, maximum_size_>& operator=( BoundedVector<T, maximum_size_> && other )
	{
		std::swap( array, other.array );
		std::swap( size_, other.size_ );
		return *this;
	}

	inline size_t capacity() const
	{
		return maximum_size_;
	}

	inline bool full() const
	{
		return size_ == maximum_size_;
	}

	T& operator[](size_t n) {
		ASSERT_LT(n, size_);
		return array[n];
	}
	T const& operator[](size_t n) const {
		ASSERT_LT(n, size_);
		return array[n];
	}

	inline bool push_back( T const& elem )
	{
		return append( elem );
	}

	inline bool append( T const& elem )
	{
		if (size_ == maximum_size_)
		{
			return false;
		}
		array[size_] = elem;
		size_++;
		return true;
	}

	inline bool push_back( T && elem )
	{
		return append( std::move( elem ) );
	}

	inline bool append( T && elem )
	{
		if ( size_ == maximum_size_ )
		{
			return false;
		}
		std::swap( array[size_], elem );
		size_++;
		return true;
	}

	Util::optional<T> pop_back() {
		if (size_ == 0)
			return {};
		return std::move(array[--size_]);
	}
	void clear() {
		while (pop_back()) {}
	}

	size_t size() const { return size_; }
	bool empty() const { return size_ == 0; }
	T* begin() { return array; }
	T* end()   { return array+size_; }
	const T* begin() const { return array; }
	const T* end()   const { return array+size_; }
};

#endif
