/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2012-2013 Unvanquished Developers

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

#ifndef GAME_IGNITABLE_COMPONENT_H_
#define GAME_IGNITABLE_COMPONENT_H_

#include "g_local.h"
#include "Components.h"

class IgnitableComponent: public IgnitableComponentBase {
public:
	IgnitableComponent(Entity* entity, bool alwaysOnFire);
	~IgnitableComponent();

	void OnDoNetCode();

	void Ignite(gentity_t* igniter);
	void PutOut(int immunityTime);
	bool Think();

private:
	float DistanceToIgnitable(IgnitableComponent* other);

	gentity_t* igniter = nullptr;
	bool onFire;
	int fireImmunityUntil;
	int nextBurnDamage;
	int nextBurnSplashDamage;
	int nextBurnAction;
};

#endif //GAME_IGNITABLE_COMPONENT_H_
