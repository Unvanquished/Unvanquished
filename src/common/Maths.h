/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2013 Unvanquished Developers

This file is part of the Daemon GPL Source Code (Daemon Source Code).

Daemon Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Daemon Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon Source Code.  If not, see <http://www.gnu.org/licenses/>.

===========================================================================
*/

#include <algorithm>

#ifndef COMMON_MATHS_H_
#define COMMON_MATHS_H_

namespace Maths {

	template<typename T> static inline T clamp(T value, T min, T max)
	{
		// if min > max, use min instead of max
		return std::max(min, std::min(std::max(min, max), value));
	}

	static inline float clampFraction(float value)
	{
		return clamp(value, 0.0f, 1.0f);
	}

	static inline double clampFraction(double value)
	{
		return clamp(value, 0.0, 1.0);
	}
}

#endif //COMMON_MATHS_H_
