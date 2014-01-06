/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2013, Daemon Developers
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

    bool ParseCvarValue(Str::StringRef value, bool& result) {
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

    template<>
    std::string GetCvarTypeName<bool>() {
        return "bool";
    }

    bool ParseCvarValue(Str::StringRef value, int& result) {
        //TODO: this accepts "1a" as a valid int
        return Str::ParseInt(result, value);
    }


    std::string SerializeCvarValue(int value) {
        return std::to_string(value);
    }

    template<>
    std::string GetCvarTypeName<int>() {
        return "int";
    }

    bool ParseCvarValue(Str::StringRef value, float& result) {
        return Str::ToFloat(std::move(value), result);
    }


    std::string SerializeCvarValue(float value) {
        return std::to_string(value);
    }

    template<>
    std::string GetCvarTypeName<float>() {
        return "float";
    }

    bool ParseCvarValue(std::string value, std::string& result) {
		result = std::move(value);
		return true;
    }

    std::string SerializeCvarValue(std::string value) {
		return std::move(value);
    }

    template<>
    std::string GetCvarTypeName<std::string>() {
        return "text";
    }
}
