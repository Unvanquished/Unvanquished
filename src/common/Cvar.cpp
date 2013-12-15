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

#include "Cvar.h"
#include "../engine/framework/CvarSystem.h"

#include "../engine/qcommon/q_shared.h"
#include "../engine/qcommon/qcommon.h"
#include "./String.h"

namespace Cvar {

    CvarProxy::CvarProxy(std::string name_, std::string description, int flags, std::string defaultValue) : name(std::move(name_)) {
        ::Cvar::Register(this, name, std::move(description), flags, std::move(defaultValue));
    }

    void CvarProxy::SetValue(std::string value) {
        ::Cvar::SetValue(name, std::move(value));
    }

    bool ParseCvarValue(std::string value, bool& result) {
        if (value == "1" or value == "on" or value == "yes" or value == "true" or value == "enable") {
            result = true;
            return true;
        } else if (value == "0" or value == "off" or value == "no" or value == "false" or value == "disable") {
            result = false;
            return true;
        }
        return false;
    }

    std::string SerializeCvarValue(bool value) {
        //TODO: we keep compatibility with integer-style booleans for now but we should use "on" and "off" once we are merged
        return value ? "1" : "0";
    }

    bool ParseCvarValue(std::string value, int& result) {
        //TODO: this accepts "1a" as a valid int
        return Str::ToInt(std::move(value), result);
    }

    template<>
    std::string GetCvarTypeName<bool>() {
        return "bool";
    }


    std::string SerializeCvarValue(int value) {
        return std::to_string(value);
    }

    bool ParseCvarValue(std::string value, std::string& result) {
		result = std::move(value);
		return true;
    }

    template<>
    std::string GetCvarTypeName<int>() {
        return "int";
    }


    std::string SerializeCvarValue(std::string value) {
		return std::move(value);
    }

    template<>
    std::string GetCvarTypeName<std::string>() {
        return "text";
    }
}
