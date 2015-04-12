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

#include "DeferredFreeingComponent.h"

DeferredFreeingComponent::DeferredFreeingComponent(Entity &entity)
	: DeferredFreeingComponentBase(entity), freeTime(DONT_FREE)
{}

void DeferredFreeingComponent::HandleFreeAt(int freeTime) {
	this->freeTime = freeTime;
}

int DeferredFreeingComponent::GetFreeTime() {
	return freeTime;
}
