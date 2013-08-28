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

//#include "../engine/qcommon/q_shared.h"
//#include "../engine/qcommon/qcommon.h"
//#include "Command.h"
#include <string>

#ifndef SHARED_CVAR_H_
#define SHARED_CVAR_H_

namespace Cvar {

    class CvarProxy {
        public:
            CvarProxy(std::string name, std::string description, int flags, std::string defaultValue);
            ~CvarProxy();

            virtual bool OnValueChanged(const std::string& newValue) = 0;

        protected:
            void SetValue(std::string newValue) const;

        private:
            std::string name;
    };


    //TODO typed variable must call Register in their code
}

/*
class CvarBool: public CvarProxy {
    void SetInt(int newValue);
    int GetInt() const;
    bool OnValueChanged(const std::string& newValue); // Can reject invalid values, which will undo the Set and revert to previous value, otherwise update proxy value

    int value;
};
*/

#endif // SHARED_CVAR_H_
