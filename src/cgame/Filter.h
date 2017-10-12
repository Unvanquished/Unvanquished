/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 2015 Unvanquished Developers

This file is part of the Unvanquished GPL Source Code (Unvanquished Source Code).

Unvanquished is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Unvanquished is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Unvanquished.  If not, see <http://www.gnu.org/licenses/>.

===========================================================================
*/


template <class T>
class Filter
{
	std::list<std::pair<int,T> > samples;
	int width;

	/**
	 * @brief The Gaussian function used to calculate a Gaussian moving averaege.
	 */
	float Gaussian( float x )
	{
		return exp( -8.0f * x * x );
	}

public:
	/**
	 * @brief Set the filter's width (in units of time).
	 */
	void SetWidth( int a_width )
	{
		width = a_width;
	}

	/**
	 * @brief Add a sample to the filter.
	 */
	void Accumulate( int time, const T& sample )
	{
		samples.remove_if(
			[&]( const std::pair<int,T>& sample )
			{
				return time - sample.first > width;
			}
		);
		samples.emplace( samples.begin( ), time, sample );
	}

	/**
	 * @brief Delete all stored samples.
	 */
	void Reset( )
	{
		samples.clear( );
	}

	/**
	 * @brief Check if there are any stored samples.
	 */
	bool IsEmpty( )
	{
		return samples.empty( );
	}

	/**
	 * @brief Return the most recent sample.
	 */
	std::pair<int,T>& Last( )
	{
		return *samples.begin( );
	}

	/**
	 * @brief Calculate the moving average of the stored samples.
	 */
	T MA( )
	{
		T total{};

		for( auto s : samples )
			total += s.second;

		return total * ( 1.0f / samples.size( ) );
	}

	/**
	 * @brief Calculate the cubic moving average of the stored samples.
	 */
	T CubicMA( int time )
	{
		T total{};
		float total_weight = 0;

		for( auto s: samples )
		{
			float weight;

			weight = pow( 1.0f - (float)( time - s.first ) / width, 3 );
			total_weight += weight;
			total += s.second * weight;
		}

		return total * ( 1.0f / total_weight );
	}

	/**
	 * @brief Calculate the Gaussian moving average of the stored samples.
	 */
	T GaussianMA( int time )
	{
		T total{};
		float total_weight = 0;

		for( auto s: samples )
		{
			float weight;

			weight = Gaussian( (float)( time - s.first ) / width );
			total_weight += weight;
			total += s.second * weight;
		}

		return total * ( 1.0f / total_weight );
	}
};
