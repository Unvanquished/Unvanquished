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


template <class T>
class Filter
{
protected:
	std::list<std::pair<int,T> > samples;
	int width;

public:
	Filter( )
	{
	}

	void SetWidth( int a_width )
	{
		width = a_width;
	}

	Filter& Accumulate( int time, T& sample )
	{
		samples.remove_if(
			[&]( std::pair<int,T>& sample )
			{
				return time - sample.first > width;
			}
		);
		samples.emplace( samples.begin( ), time, sample );

		return *this;
	}

	void Reset( )
	{
		samples.clear( );
	}

	bool IsEmpty( )
	{
		return samples.size( ) == 0;
	}

	std::pair<int,T>& Last( )
	{
		return *samples.begin( );
	}

	T MA( )
	{
		T total = 0;

		for( auto s : this->samples )
			total += s.second;

		return total * ( 1.0f / this->samples.size( ) );
	}

	T CubicMA( int time )
	{
		T total = 0;
		float weight, total_weight = 0;

		for( auto s: this->samples )
		{
			weight = 1.0f - (float)( time - s.first ) / this->width;
			weight = weight * weight * weight;
			total_weight += weight;
			total += s.second * weight;
		}

		return total * ( 1.0f / total_weight );
	}

	T GaussianMA( int time )
	{
		T total;
		float weight, total_weight = 0;

		total = 0;

		for( auto s: this->samples )
		{
			weight = (float)( time - s.first ) / this->width;
			weight = exp( -8.0f * weight * weight );
			total_weight += weight;
			total += s.second * weight;
		}

		return total * ( 1.0f / total_weight );
	}
};
