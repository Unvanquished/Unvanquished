/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 2014 Unvanquished Developers

This file is part of the Unvanquished GPL Source Code (Unvanquished Source Code).

Unvanquished Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Unvanquished Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Unvanquished Source Code.  If not, see <http://www.gnu.org/licenses/>.

===========================================================================
*/

#ifndef SGAME_FREEABLE_COMPONENT_H_
#define SGAME_FREEABLE_COMPONENT_H_

#include "Components.h"

class DeferedFreeingComponent: public DeferedFreeingComponentBase {
public:
	DeferedFreeingComponent(Entity* entity);

	typedef enum {
		DONT_FREE,
		FREE_AFTER_GROUP_THINKING,
		FREE_AFTER_ALL_THINKING,
	} freeTime_t;

	// TODO: Allow messages to take arbitrary types as parameters
	//void OnFreeAt(freeTime_t freeTime);
	void OnFreeAt(int freeTime);

	/**
	 * @return When to free the parent entity.
	 */
	//freeTime_t GetFreeTime();
	int GetFreeTime();

private:
	//freeTime_t freeTime;
	int freeTime;
};

#endif //SGAME_FREEABLE_COMPONENT_H_
