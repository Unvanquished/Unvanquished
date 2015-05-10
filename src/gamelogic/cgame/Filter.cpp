/*
===========================================================================

Copyright 2015 Unvanquished Developers

This file is part of Daemon.

Daemon is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Daemon is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon.  If not, see <http://www.gnu.org/licenses/>.

===========================================================================
*/

#include "cg_local.h"

template <class T>
Filter<T>::Filter( int a_width )
{
	this->width = a_width;
}

template <class T>
void Filter<T>::Insert( T sample )
{
	this->samples.remove_if(
		[&]( std::pair<int,T>& sample )
		{
			return cg.time - sample.first > width;
		}
	);
	this->samples.emplace( this->samples.begin( ), cg.time, sample );
}

template <class T>
void Filter<T>::Reset( )
{
	this->samples.clear( );
}


template <class T>
T MAFilter<T>::Get( )
{
	T total = 0;

	for( auto s : this->samples )
		total += s.second;

	return total / this->samples.size( );
}

template <class T>
T CubicMAFilter<T>::Get( )
{
	T total = 0;
	float weight, total_weight = 0;

	for( auto s: this->samples )
	{
		weight = 1.0f - (float)( cg.time - s.first ) / this->width;
		weight = weight * weight * weight;
		total_weight += weight;
		total += s.second * weight;
	}

	return total / total_weight;
}

template <class T>
T GaussianMAFilter<T>::Get( )
{
	T total = 0;
	float weight, total_weight = 0;

	for( auto s: this->samples )
	{
		weight = (float)( cg.time - s.first ) / this->width;
		weight = exp( -8.0f * weight * weight );
		total_weight += weight;
		total += s.second * weight;
	}

	return total / total_weight;
}
