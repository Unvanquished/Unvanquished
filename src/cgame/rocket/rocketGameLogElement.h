/*
This file is part of Unvanquished.

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
*/

#ifndef ROCKETINFOBOXELEMENT_H
#define ROCKETINFOBOXELEMENT_H

#include "rocket.h"
#include <Rocket/Core.h>
#include <Rocket/Core/Element.h>
#include <Rocket/Core/FontFaceHandle.h>
#include "../cg_local.h"

typedef enum {
	OBITUARY,
	BEACON,

	COUNT
} gameLogEventType_t;

typedef struct {
	int time;
	Rocket::Core::Element *element;
} gameLogEntry_t;

class RocketGameLogElement : public Rocket::Core::Element
{
public:
	RocketGameLogElement(const Rocket::Core::String &tag)
	: Rocket::Core::Element(tag)
	{
	}

	~RocketGameLogElement();
	
	void OnUpdate(void);

private:
	void Initialize(void);

	bool initialized = false;
	Rocket::Core::Element *templates[gameLogEventType_t::COUNT] = {NULL};
	std::list<gameLogEntry_t> entries;
};

#endif


